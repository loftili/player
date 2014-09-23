#include <iostream>
#include <sstream>
#include <vector>
#include <mpg123.h>
#include <curl/curl.h>
#include <ao/ao.h>
#include <Portaudio.h>
using namespace std;
#define FRAME_PER_BUFFER 4096
#define BITS 8
typedef PaStreamCallbackTimeInfo TimeInfo;
typedef PaStreamCallbackFlags StreamFlags;
typedef unsigned long FrameCount;

// http://www.soundjay.com/button/button-09.mp3
typedef unsigned long FrameCount;

struct AudioBuffer {
  mpg123_handle* mhandle;
  pthread_mutex_t wait_sig;
  pthread_cond_t wait_cond;

  size_t realsize;
  int length;

  void* data;
  void* full_data;
  int current_frame;
  bool safe;
  bool finished;

  int channels;
  int encoding;
  long rate;
};

PaStream* p_stream;
AudioBuffer* audio_buffer;
pthread_t play_thread;
pthread_t watch_thread;
ao_device *dev = NULL;

int playback(const void* in, void* out, FrameCount fpb, const TimeInfo* ti, StreamFlags f, void* d) {
  AudioBuffer* audio_buffer = (AudioBuffer*) d;

  pthread_mutex_lock(&audio_buffer->wait_sig);

  /*
  cout << "Rate: " << audio_buffer->rate << endl;
  cout << "Encoding: " << audio_buffer->encoding << endl;
  cout << "Channels: " << audio_buffer->channels << endl;
  cout << "Size: " << audio_buffer->length << endl;
  cout << "Frames Per Buffer: " << fpb << endl;
  */

  off_t frame_offset;
  unsigned char* audio;
  size_t done;

  int err = mpg123_decode_frame(audio_buffer->mhandle, &frame_offset, &audio, &done);

  switch(err) {
    case MPG123_NEW_FORMAT:
      //cout << "-> new format" << endl;
      break;
    case MPG123_OK:
      ao_play(dev, (char*)audio, done);
      break;
    case MPG123_NEED_MORE:
      //cout << "-> bad 1" << endl;
      audio_buffer->finished = true;
      break;
    default:
      //cout << "-> bad 2" << endl;
      break;
  }

  pthread_cond_broadcast(&audio_buffer->wait_cond);
  pthread_mutex_unlock(&audio_buffer->wait_sig);
  return 0;
}

size_t load_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  AudioBuffer* audio_buffer = (AudioBuffer*) userp;
  size_t realsize = size * nmemb;
  audio_buffer->full_data = realloc(audio_buffer->full_data, audio_buffer->length + realsize + 1);
  void* end = &((char*)audio_buffer->full_data)[audio_buffer->length];
  memcpy(end, buffer, realsize);
  audio_buffer->length += realsize;
  ((char*)audio_buffer->full_data)[audio_buffer->length] = 0;
  return realsize;
}

void* watch_stream(void* audio_buffer_data) {
  AudioBuffer* audio_buffer = (AudioBuffer*) audio_buffer_data;
  while(true) {
    if(audio_buffer->finished) {
      break;
    }
    pthread_cond_wait(&audio_buffer->wait_cond, &audio_buffer->wait_sig);
  }

  cout << "Finished!" << endl;
  Pa_StopStream(p_stream);
  Pa_CloseStream(p_stream);
}

void* fetch_and_play(void* url_buffer) {
  ao_initialize();
  cout << "initializing curl fetch for: " << (char*) url_buffer << endl;
  CURL* curl_handle;
  ao_initialize();
  
  mpg123_init();
  mpg123_handle* mhandle = NULL;
  mhandle = mpg123_new(NULL, NULL);
  mpg123_open_feed(mhandle);

  audio_buffer = new AudioBuffer;
  audio_buffer->length = 0;
  audio_buffer->realsize = 0;
  audio_buffer->data = NULL;
  audio_buffer->safe = false;
  audio_buffer->full_data = malloc(0);
  audio_buffer->current_frame = 0;
  audio_buffer->mhandle = mhandle;

  pthread_mutex_init(&audio_buffer->wait_sig, NULL);
  pthread_cond_init(&audio_buffer->wait_cond, NULL);

  int success = Pa_Initialize();
  int devices = Pa_GetDeviceCount();
  if(success != paNoError || devices < 1) {
    cout << "portaudio initialize fail" << endl;
    return NULL;
  }

  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char*) url_buffer);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, load_data);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, audio_buffer);

  curl_easy_perform(curl_handle);
  curl_easy_cleanup(curl_handle);

  cout << "finished with audio buffer, parsing data" << endl;
  mpg123_feed(mhandle, (const unsigned char*) audio_buffer->full_data, audio_buffer->length);

  off_t frame_offset;
  unsigned char *audio;
  size_t done;
  int err = mpg123_decode_frame(mhandle, &frame_offset, &audio, &done);
  if(err != MPG123_NEW_FORMAT) {
    cout << "BAD FORMAT FROM FULL DATA BUFFER!!" << endl;
    audio_buffer->safe = false;
  } else {
    mpg123_getformat(mhandle, &audio_buffer->rate, &audio_buffer->channels, &audio_buffer->encoding);
    ao_sample_format format;
    format.bits = mpg123_encsize(audio_buffer->encoding) * BITS;
    format.rate = audio_buffer->rate;
    format.channels = audio_buffer->channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(ao_default_driver_id(), &format, NULL);
    audio_buffer->safe = true;
  }

  if(audio_buffer->safe) {
    do {
      err = mpg123_decode_frame(audio_buffer->mhandle, &frame_offset, &audio, &done);
      switch(err) {
        case MPG123_OK:
          ao_play(dev, (char*)audio, done);
          break;
        default:
          break;
      }
    } while(done > 0);
  }

  /*
  if(audio_buffer->safe) {
    cout << "safe audio file, continuing to decoding frames" << endl;
    err = Pa_OpenDefaultStream(&p_stream, 0, 2, paFloat32, 44100, FRAME_PER_BUFFER, &playback, (void*)audio_buffer);
    err = Pa_StartStream(p_stream);
    pthread_create(&watch_thread, NULL, watch_stream, (void*) audio_buffer);
  }
  */
}

void play(const char* url) {
  pthread_create(&play_thread, NULL, fetch_and_play, (void*) url);
}

int main(int argc, char* argv[]) {
  string url;
  cout << "enter a url for us to play: " << endl;
  cin >> url;
  std::cout << url << endl;
  play(url.c_str());

  cout << "type exit to close" << endl;
  string exit;
  do {
    cin >> exit;
  } while(exit != "exit");

  ao_close(dev);
  ao_shutdown();

  free(audio_buffer);

}
