#include <alsa/asoundlib.h>

// Generate a sinusoidal function with specified parameters and assign it to memory areas
void generate_sine(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude);

// Generate a triangular function with specified parameters and assign it to memory areas
void generate_triangular(const snd_pcm_channel_area_t *areas, snd_pcm_uframes_t offset, int count, double *_phase, int amplitude);

// Recovery of the board if an error occours
int xrun_recovery(snd_pcm_t *handle, int err);

// Write data to the board
int write_loop(snd_pcm_t *handle, signed long *samples, snd_pcm_channel_area_t *areas, 
                unsigned int seconds, char *type, unsigned int amplitude, char *write_type);

// Read data from the board
int read_loop(snd_pcm_t *capture_handle, snd_pcm_uframes_t frames, int loops, long long *data);

// Write an array of data to a file
int data_to_file(const char *filename, long long *data, int size);