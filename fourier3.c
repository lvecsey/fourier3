/*
    Basic implementation of a fourier transform for a signal, multi threaded.
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <complex.h>

#include <errno.h>
#include <math.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include "mini_gxkit.h"

#include "writefile.h"

#define def_numthreads 4

double cmag(double complex z) {

  double x, y;

  x = creal(z);
  y = cimag(z);

  return sqrt(x * x + y * y);
  
}

int fill_freq(double *samples, double freq, double duration, long int num_samples) {

  long int sampleno;

  for (sampleno = 0; sampleno < num_samples; sampleno++) {

    samples[sampleno] = cos(2.0 * M_PI * freq * duration * sampleno / num_samples);

  }
    
  return 0;

}

typedef struct {

  image_t *img_bg;
  
  double complex weighted_sum;

  double dt;
  
  double freq;

  double duration;
  
  double *samples_combined;

  long int num_samples;
  
  long int sampleno_start;

  long int sampleno_end;  
    
} fwork;

void *fourier_work(void *extra) {

  void *ret;

  fwork *fw;
  
  long int sampleno;

  double radius;

  double complex fvalue;
  
  point_t pt;
  
  long int xpos, ypos;

  long int offset;

  double aspect;

  image_t *img;

  double t0;
  
  fw = (fwork*) extra;

  img = fw->img_bg;
  
  aspect = ((double) img->xres) / img->yres;
  
  for (sampleno = fw->sampleno_start; sampleno < fw->sampleno_end; sampleno++) {

    t0 = (sampleno * fw->duration) / fw->num_samples;
    
    radius = 0.5 + (0.5 * fw->samples_combined[sampleno]);
    
    fvalue = radius * cexp(-2.0 * M_PI * I * t0 * fw->freq);

    fw->weighted_sum += (fvalue * fw->dt);

    {
      
      pt.x = creal(fvalue);
      pt.y = cimag(fvalue);

      pt.x /= aspect;
      pt.y *= -1.0;
    
      xpos = pt.x * (img->xres >> 1); xpos += img->xres >> 1;
      ypos = pt.y * (img->yres >> 1); ypos += img->yres >> 1;    

      if (xpos < 0 || xpos >= img->xres) {
	continue;
      }

      if (ypos < 0 || ypos >= img->yres) {
	continue;
      }

      offset = ypos * img->xres + xpos;
      
      img->rgb[offset].r += 2;
      img->rgb[offset].g += 2;
      img->rgb[offset].b += 2;      

    }
      
  }
  
  ret = NULL;
  
  return ret;
  
}

int plotpoint(image_t *img, char *desc, long int xpos, double y, pixel_t fill_color) {

  long int ypos;
  
  ypos = y * (img->yres >> 1); ypos += (img->yres >> 1);    

  if (ypos < 0 || ypos >= img->yres) {
    return -1;
  }

  img->rgb[ypos * img->xres + xpos] = fill_color;   

  return 0;
  
}

int main(int argc, char *argv[]) {

  long int input_xres, input_yres;

  image_t img;
  
  image_t img_bg;

  image_t img_freqsweep;
  
  long int num_pixels;
  size_t img_sz;
  
  double duration;

  long int samplerate;

  double *samples_track1;

  double *samples_track2;

  double *samples_track3;  

  double *samples_combined;
  
  long int num_samples;

  long int sampleno;

  double freq;

  long int xpos;

  point_t pt;

  pixel_t white;

  double complex weighted_sum;

  double audio_freqs[3] = { 7.0, 17.0, 27.50 };
  
  long int num_freqs;

  long int freqno;

  double min_freq;
  
  double max_freq;

  pixel_t red;

  pixel_t green;
  
  double percent;

  double sf;

  double v;

  int retval;

  long int num_threads;

  pthread_t *threads;

  fwork *fws;

  long int threadno;
  
  duration = 10.0;

  samplerate = 48000;

  num_samples = samplerate * duration;

  fprintf(stderr, "%s: Allocating original audio tracks, and combined track.\n", __FUNCTION__);
  
  samples_track1 = malloc(num_samples * sizeof(double));
  if (samples_track1 == NULL) {
    perror("malloc");
    return -1;
  }

  samples_track2 = malloc(num_samples * sizeof(double));
  if (samples_track2 == NULL) {
    perror("malloc");
    return -1;
  }

  samples_track3 = malloc(num_samples * sizeof(double));
  if (samples_track3 == NULL) {
    perror("malloc");
    return -1;
  }

  samples_combined = malloc(num_samples * sizeof(double));
  if (samples_combined == NULL) {
    perror("malloc");
    return -1;
  }

  fprintf(stderr, "%s: Placing signal into audio tracks + combining.\n", __FUNCTION__);
  
  fill_freq(samples_track1, audio_freqs[0], duration, num_samples);
  fill_freq(samples_track2, audio_freqs[1], duration, num_samples);
  fill_freq(samples_track3, audio_freqs[2], duration, num_samples);  

  for (sampleno = 0; sampleno < num_samples; sampleno++) {
    samples_combined[sampleno] = (samples_track1[sampleno] + samples_track2[sampleno] + samples_track3[sampleno]) / 3.0;
  }

  retval = argc>1 ? sscanf(argv[1],"%ldx%ld",&input_xres,&input_yres) : -1;
  if (retval != 2) {
    fprintf(stderr, "%s: Please specify render resolution.\n", __FUNCTION__);
    return -1;
  }
  
  img.xres = input_xres;
  img.yres = input_yres;

  num_pixels = img.xres * img.yres;
  img_sz = num_pixels * sizeof(pixel_t);
  
  img.rgb = malloc(img_sz);
  if (img.rgb == NULL) {
    perror("malloc");
    return -1;
  }

  img_bg = (image_t) { .rgb = malloc(img_sz), .xres = input_xres, .yres = input_yres };
  if (img_bg.rgb == NULL) {
    perror("malloc");
    return -1;
  }

  img_freqsweep = (image_t) { .rgb = malloc(img_sz), .xres = input_xres, .yres = input_yres };
  if (img_freqsweep.rgb == NULL) {
    perror("malloc");
    return -1;
  }

  
  num_threads = argc>2 ? strtol(argv[2],NULL,10) : def_numthreads;

  fprintf(stderr, "%s: Allocating %ld threads.\n", __FUNCTION__, num_threads);
  
  threads = malloc(num_threads * sizeof(pthread_t));
  if (threads == NULL) {
    perror("malloc");
    return -1;
  }

  fws = malloc(num_threads * sizeof(fwork));
  if (fws == NULL) {
    perror("malloc");
    return -1;
  }
  
  white = (pixel_t) { .r = 65535, .g = 65535, .b = 65535 };

  red = (pixel_t) { .r = 65535, .g = 0, .b = 0 };

  green = (pixel_t) { .r = 0, .g = 65535, .b = 0 };
  
  num_freqs = img.xres * 2;

  min_freq = 0.0;
  max_freq = min_freq;

  {
    long int ano;
    for (ano = 0; ano < sizeof(audio_freqs) / sizeof(long int); ano++) {
      if (audio_freqs[ano] > max_freq) {
	max_freq = audio_freqs[ano];
      }
    }
    max_freq *= 2.0;
  }
  
  for (freqno = 0; freqno < num_freqs; freqno++) {

    v = freqno; v /= num_freqs;
    
    freq = (1.0 - v) * min_freq + (v * max_freq);
    
    percent = freqno; percent /= (num_freqs - 1);
    
    if (!(freqno%5)) {
      fprintf(stderr, "%s: (freq %g) Percent complete %.2g     \r", __FUNCTION__, freq, percent * 100.0);
    }

    for (threadno = 0; threadno < num_threads; threadno++) {

      fws[threadno].img_bg = &img_bg;
      
      fws[threadno].weighted_sum = 0.0;

      fws[threadno].dt = ((double) num_threads) / num_samples;

      fws[threadno].freq = freq;

      fws[threadno].duration = duration;
      
      fws[threadno].samples_combined = samples_combined;

      fws[threadno].num_samples = num_samples;

      fws[threadno].sampleno_start = threadno * num_samples / num_threads;

      fws[threadno].sampleno_end = (threadno+1) * num_samples / num_threads;
      
      retval = pthread_create(threads + threadno, NULL, fourier_work, fws + threadno);
      if (retval == -1) {
	perror("pthread_create");
	return -1;
      }

    }

    for (threadno = 0; threadno < num_threads; threadno++) {

      retval = pthread_join(threads[threadno], NULL);
      if (retval == -1) {
	perror("pthread_join");
	return -1;
      }
      
    }

    weighted_sum = 0.0;
    
    for (threadno = 0; threadno < num_threads; threadno++) {
      weighted_sum += fws[threadno].weighted_sum;
    }
      
    xpos = (freqno * img.xres) / num_freqs;

    sf = duration;
    
    pt.y = creal(weighted_sum);
    pt.y *= -1.0;
    pt.y *= sf;
    plotpoint(&img_freqsweep, "Real", xpos, pt.y, red);

    pt.y = cimag(weighted_sum);
    pt.y *= -1.0;
    pt.y *= sf;
    plotpoint(&img_freqsweep, "Imag", xpos, pt.y, green);

    pt.y = cmag(weighted_sum);
    pt.y *= -1.0;
    pt.y *= sf;
    plotpoint(&img_freqsweep, "Mag", xpos, pt.y, white);
    
  }

  fprintf(stderr, "%s: Preparing final image.\n", __FUNCTION__);

  memcpy(img.rgb, img_bg.rgb, img_sz);

  {
    long int pixelno;
    for (pixelno = 0; pixelno < num_pixels; pixelno++) {
      if (img_freqsweep.rgb[pixelno].r || img_freqsweep.rgb[pixelno].g || img_freqsweep.rgb[pixelno].b) {
	img.rgb[pixelno] = img_freqsweep.rgb[pixelno];
      }
    }
  }
    
  fprintf(stderr, "%s: Writing image output to stdout.\n", __FUNCTION__);
  
  {
    int out_fd;
    ssize_t bytes_written;

    out_fd = 1;
    bytes_written = writefile(out_fd, img.rgb, img_sz);
    if (bytes_written != img_sz) {
      perror("write");
      return -1;
    }
  }

  free(fws);
  
  free(threads);

  free(samples_track1);
  free(samples_track2);
  free(samples_track3);  

  free(samples_combined);

  free(img_bg.rgb);

  free(img_freqsweep.rgb);
  
  free(img.rgb);
  
  return 0;

}
