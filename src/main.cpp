#include <iostream>
#include <sstream>
#include <mpg123.h>
#include <portaudio.h>
#include <curl/curl.h>
using namespace std;
#define FRAME_PER_BUFFER 4096

typedef PaStreamCallbackTimeInfo TimeInfo;
typedef PaStreamCallbackFlags StreamFlags;
typedef unsigned long FrameCount;

struct AudioBuffer {
  int length;
  size_t realsize;
  void* data;
  mpg123_handle* mhandle;
  int channels;
  int encoding;
  long rate;
};

PaStream* pstream;
AudioBuffer* audio_buffer;


int playback(const void* in, void* out, FrameCount fpb, const TimeInfo* ti, StreamFlags f, void* d) {
  AudioBuffer* ab = (AudioBuffer*) d;
  size_t done;
  off_t frame_offset;
  unsigned char* audio;
  int err = mpg123_decode_frame(ab->mhandle, &frame_offset, &audio, &done);
  if(err != MPG123_OK) {
    return -11;
  }
  memcpy(out, audio, sizeof(float) * FRAME_PER_BUFFER * 2);
  return 0;
}

size_t load_data(char* ptr, size_t size, size_t nmemb, void* userdata) {
  AudioBuffer* buffer = (AudioBuffer*) userdata;
  size_t realsize = size * nmemb;
  buffer->data = realloc(buffer->data, buffer->length + realsize + 1);
  void* end = &((char*)buffer->data)[buffer->length];
  memcpy(end, ptr, realsize);
  buffer->length += realsize;
  ((char*)buffer->data)[buffer->length] = 0;
  buffer->realsize += realsize;
  return realsize;
}

void* fetch_and_play(void* url_buffer) {
  cout << "initializing curl fetch for: " << (char*) url_buffer << endl;
  CURL* curl_handle;
  audio_buffer = (AudioBuffer*) malloc(sizeof(AudioBuffer));
  audio_buffer->length = 0;
  audio_buffer->realsize = 0;
  audio_buffer->data = NULL;

  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, (char*) url_buffer);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, load_data);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, audio_buffer);
  curl_easy_perform(curl_handle);
  curl_easy_cleanup(curl_handle);

  cout << "finished loading data... " << endl;
  cout << "sending into mpg123" << endl;

  mpg123_init();
  mpg123_handle* mhandle = NULL;
  mhandle = mpg123_new(NULL, NULL);
  mpg123_open_feed(mhandle);
  mpg123_feed(mhandle, (const unsigned char*)audio_buffer->data, audio_buffer->realsize);

  unsigned char *audio;
  size_t done;
  off_t frame_offset;
  int err = mpg123_decode_frame(mhandle, &frame_offset, &audio, &done);
  if(err != MPG123_NEW_FORMAT) {
    cout << "invalid mpg file, continuing" << endl;
    return NULL;
  }

  audio_buffer->mhandle = mhandle;
  cout << "seems to be a valid mpg file, continuing" << endl;
  err = mpg123_getformat(mhandle, &audio_buffer->rate, &audio_buffer->channels, &audio_buffer->encoding);
  if(err != MPG123_OK) {
    cout << "totally invalid mpg file, continuing" << endl;
    return NULL;
  }
  cout << "totally valid file, moving to portaudio" << endl;

  err = Pa_Initialize();
  int devices = Pa_GetDeviceCount();
  if(err != paNoError || devices < 1) {
    cout << "portaudio initialize fail" << endl;
    return NULL;
  }

  err = Pa_OpenDefaultStream(&pstream, 0, 2, paFloat32, 44100, FRAME_PER_BUFFER, playback, (void*) &audio_buffer);
  if(err != paNoError) {
    cout << "portaudio open fail" << endl;
    return NULL;
  }
  cout << "opened stream, starting!" << endl;
  err = Pa_StartStream(pstream);
  cout << "started stream, godspeed" << endl;
}

void play(string url) {
  pthread_t play_thread;
  pthread_create(&play_thread, NULL, fetch_and_play, (void*) url.c_str());
}

int main(int argc, char* argv[]) {
  string url;
  cout << "enter a url for us to play: " << endl;
  cin >> url;
  play(url);

  cout << "type exit to close" << endl;
  string exit;
  do {
    cin >> exit;
  } while(exit != "exit");

  free(audio_buffer);

}
