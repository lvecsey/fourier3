
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <math.h>

#include <complex.h>

#include "fill_sound.h"

int fill_sound(double *samples, long int num_samples, long int samplerate, double min_freq, double max_freq, long int num_freqs, double complex *weighted_sums) {

  long int freqno;

  double duration;

  double vf;

  double freq;

  long int sampleno;

  double percent;

  double t0;

  double complex fvalue;

  duration = num_samples; duration /= samplerate;

  for (sampleno = 0; sampleno < num_samples; sampleno++) {

    t0 = sampleno; t0 /= num_samples;
    
    if (!(sampleno%5)) {
      percent = t0;
      fprintf(stderr, "%s: Writing percent complete %g     \r", __FUNCTION__, percent * 100.0);
    }

    fvalue = 0.0;
    
    for (freqno = 0; freqno < num_freqs; freqno++) {

      vf = freqno; vf /= num_freqs;

      freq = (1.0 - vf) * min_freq + (vf * max_freq);
    
      fvalue += weighted_sums[freqno] * cexp(2.0 * M_PI * freq * t0) / (2.0 * M_PI);

    }

    samples[sampleno] = creal(fvalue);
    
  }

  putchar('\n');

  return 0;
	
}
