/* Writing Functions */
void play_sin(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision);

void play_triangular(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision);

void play_square(snd_pcm_t *handle, int frames, int num_frames, double rate, double frequency, int max_amplitude, int precision);


/* Reading Functions */
int write_data_16(const char *filename, int *data, int size);

int write_data_24(const char *filename, long long *data, int size);

int acquire_data(snd_pcm_t *handle, int frames, int loops, int precision);