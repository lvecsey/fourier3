#ifndef FILL_SOUND_H
#define FILL_SOUND_H

#include <complex.h>

int fill_sound(double *samples, long int num_samples, long int samplerate, double min_freq, double max_freq, long int num_freqs, double complex *weighted_sums);

#endif
