#include <iostream>
#include <portaudio.h>
#include <pthread.h>
#include <mpg123.h>
#define FPB 4096

typedef PaStreamCallbackTimeInfo TimeInfo;
typedef PaStreamCallbackFlags StreamFlags;
typedef unsigned long FrameCount;

typedef struct {
  PaStream *stream;
  int size;
  float* buffer;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  mpg123_handle* mh;
} Portaudio;


void* load_buffer(void *data) {
  Portaudio* portaudio = (Portaudio*) data;
  size_t done;

  while(true) {
    pthread_cond_wait(&portaudio->cond, &portaudio->mutex);
    int err = mpg123_read(portaudio->mh, (unsigned char *) portaudio->buffer, portaudio->size * sizeof(float), &done);
  }

  pthread_exit(NULL);
}

int onPlay(const void* input, void* output, FrameCount fpb, const TimeInfo* time_info, StreamFlags flags, void* data) {
  Portaudio* portaudio = (Portaudio*) data;
  size_t done;
  pthread_mutex_lock(&portaudio->mutex);

  memcpy(output, portaudio->buffer, sizeof(float) * portaudio->size);

  pthread_cond_broadcast(&portaudio->cond);
  pthread_mutex_unlock(&portaudio->mutex);
  return 0; 
}

int main(int argc, char* argv[]) {
  std::string input;
  if(argc < 2)
    return 1;

  std::string file_name = argv[1];
  int success = Pa_Initialize();
  int devices = Pa_GetDeviceCount();
  int mpg_error = MPG123_OK;

  if(success != paNoError || devices < 1) {
    std::cout << "-!> portaudio initialize fail" << std::endl;
    return 1;
  }

  std::cout << "-> Portaudio fine, moving onto mpg123" << std::endl;

  int error = 0;
  mpg123_init();
  mpg123_handle* mh;
  mh = mpg123_new(NULL, NULL);
  
  mpg_error = mpg123_open(mh, file_name.c_str());
  if(mpg_error == MPG123_ERR) {
    std::cout << "-!> mpg123 failed to open on a generic error file: " << mpg_error << std::endl;
    mpg123_close(mh);
    return 1;
  } else if(mpg_error != MPG123_OK) {
    std::cout << "-!> mpg123 failed to open the file: " << mpg_error << std::endl;
    mpg123_close(mh);
    return 1;
  }
  std::cout << "-> Mpg123 opened file fine, attempting to play" << std::endl;

  mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
  
  long rate;
  int channels, encoding;
  mpg_error = mpg123_getformat(mh, &rate, &channels, &encoding);
  if(mpg_error != MPG123_OK) {
    std::cout << "-!> mpg123 failed to get format for the file: " << mpg_error << std::endl;
    mpg123_close(mh);
    return 1;
  }

  if(encoding != MPG123_ENC_FLOAT_32) {
    std::cout << "-!> bad encoding " << std::endl;
    mpg123_close(mh);
    return 1;
  }
  mpg123_format_none(mh);
  mpg123_format(mh, rate, channels, encoding);

  std::cout << "-> Mpg123 got format information: ";
  std::cout << "rate[" << rate << "] | ";
  std::cout << "channels[" << channels << "] | ";
  std::cout << "ecoding[" << encoding << "]" << std::endl;

  int framesPerBuffer = FPB;
  size_t done = 0;
  Portaudio *portaudio = (Portaudio *) malloc(sizeof(Portaudio));
  pthread_mutex_init(&portaudio->mutex, NULL);
  pthread_cond_init(&portaudio->cond, NULL);
  portaudio->size = framesPerBuffer * 2.0;
  portaudio->buffer = (float *) malloc(sizeof(float) * portaudio->size);
  portaudio->mh = mh;

  int pa_error = Pa_OpenDefaultStream(&portaudio->stream, 0, 2, paFloat32, 44100, framesPerBuffer, onPlay, (void*)portaudio);

  if(pa_error != paNoError) {
    std::cout << "-!> failed opening port audio stream" << std::endl;
    free(portaudio->buffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    return 1;
  }
  std::cout << "-> port audio opened a stream!" << std::endl;

  int i;
  pthread_t load_thread;
  pthread_create(&load_thread, NULL, load_buffer, (void *)portaudio);

  pa_error = Pa_StartStream(portaudio->stream);
  if(pa_error != paNoError) {
    std::cout << "-!> failed starting port audio stream" << std::endl;
    free(portaudio->buffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    return 1;
  }
  std::cout << "-> port audio started a stream!" << std::endl;

  while(input != "exit") {
    std::cin >> input;
  }

  free(portaudio->buffer);
  mpg123_close(mh);
  mpg123_delete(mh);

  Pa_StopStream(portaudio->stream);
  Pa_CloseStream(portaudio->stream);

  return 0;
}

void print_message_function(void* data) {
  std::cout << "whoa" << std::endl;
}


