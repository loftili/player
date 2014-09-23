#include <iostream>
#include <sstream>
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

//typedef PaStreamCallbackTimeInfo TimeInfo;
//typedef PaStreamCallbackFlags StreamFlags;
typedef unsigned long FrameCount;

struct AudioBuffer {
  string name;
  int length;
  size_t realsize;
  void* data;
  mpg123_handle* mhandle;
  int channels;
  int encoding;
  long rate;
};

PaStream* p_stream;
AudioBuffer* audio_buffer;
ao_device *dev = NULL;

int playback(const void* in, void* out, FrameCount fpb, const TimeInfo* ti, StreamFlags f, void* d) {
  cout << "playing!" << endl;
  return 0;
}

size_t load_data(void *buffer, size_t size, size_t nmemb, void *userp) {
  AudioBuffer* audio_buffer = (AudioBuffer*) userp;

  size_t realsize = size * nmemb;
  int err;
  off_t frame_offset;
  char *audio;
  size_t done;
  ao_sample_format format;
  int channels, encoding;
  long rate;
  void* end;

  mpg123_feed(audio_buffer->mhandle, (const unsigned char*) buffer, realsize);

  do {
    err = mpg123_decode_frame(audio_buffer->mhandle, &frame_offset, (unsigned char**)&audio, &done);
    switch(err) {
      case MPG123_NEW_FORMAT:
        mpg123_getformat(audio_buffer->mhandle, &rate, &channels, &encoding);
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(ao_default_driver_id(), &format, NULL);
        break;
      case MPG123_OK:
        ao_play(dev, audio, done);
        break;
      case MPG123_NEED_MORE:
        break;
      default:
        break;
    }
  } while(done > 0);

  return realsize;
}

void* fetch_and_play(void* url_buffer) {
  ao_initialize();
  cout << "initializing curl fetch for: " << (char*) url_buffer << endl;
  CURL* curl_handle;
  
  mpg123_init();
  mpg123_handle* mhandle = NULL;
  mhandle = mpg123_new(NULL, NULL);
  mpg123_open_feed(mhandle);

  audio_buffer = (AudioBuffer*) malloc(sizeof(AudioBuffer));
  audio_buffer->length = 0;
  audio_buffer->realsize = 0;
  audio_buffer->data = NULL;
  audio_buffer->mhandle = mhandle;
  audio_buffer->name = "hello world";

  int success = Pa_Initialize();
  int devices = Pa_GetDeviceCount();
  if(success != paNoError || devices < 1) {
    cout << "portaudio initialize fail" << endl;
    return NULL;
  }

  mpg123_feed(mhandle, (const unsigned char*)audio_buffer->data, audio_buffer->realsize);

  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char*) url_buffer);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, load_data);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, audio_buffer);

  int m_error = Pa_OpenDefaultStream(&p_stream, 0, 2, paFloat32, 44100, FRAME_PER_BUFFER, &playback, (void*)audio_buffer);
  m_error = Pa_StartStream(p_stream);
  curl_easy_perform(curl_handle);
  curl_easy_cleanup(curl_handle);

  mpg123_close(mhandle);
  mpg123_delete(mhandle);

  free(audio_buffer);

  ao_close(dev);
  ao_shutdown();

  Pa_StopStream(p_stream);
  Pa_CloseStream(p_stream);
}

void play(const char* url) {
  pthread_t play_thread;
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

  free(audio_buffer);

}
