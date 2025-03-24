#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define maxvalue 8388607

static unsigned int maxval = maxvalue;
static int precision = 24;                          // Generate a signal 10 seconds long
static double frequency = 700;                     // in Hz
static unsigned int rate = 192000;                   // Work at the maximum possible sampling rate
static int n_channels = 2;                          // This is really the only choice since I didn't manage to make it works with just 1 channel
static char *device = "hw:CARD=sndrpihifiberry,DEV=0";                 // Name of the device 
static char *type = "s";
static char *write_type = "c";                  // Continuous write method
static int amplitude = maxvalue;
static int duration = 2;                        // Duration of the signal in seconds

static unsigned int buffer_time = 500000;
static unsigned int period_time = 100000;
static int resample = 1;
static int period_event = 0;
static int verbose = 0;
static int morehelp = 0;
static int method = 0;

static int loops = 15;

static snd_pcm_format_t format = SND_PCM_FORMAT_S24_LE;

static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static snd_output_t *output = NULL;

static int rc, err, dir;

// Set the hardware parameters of the board
int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params, snd_pcm_access_t access);

// Set the software parameters of the board
int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams);

// Set the hardware parameters of the board for the reading process
int set_read_hwparameters(snd_pcm_t *capture_handle, snd_pcm_hw_params_t *hw_params, snd_pcm_uframes_t frames);