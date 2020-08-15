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

#include <sys/mman.h>

#include <pthread.h>

#include "mini_gxkit.h"

#include "fill_sound.h"

#include "writefile.h"

#define def_numthreads 4

#define def_outsf "/tmp/outfile_fourier3-snd.raw"

const long int FDEPTH = 4096;

double cmag(double complex z) {

  double x, y;

  x = creal(z);
  y = cimag(z);

  return sqrt(x * x + y * y);
  
}

int fill_freq(double *samples, double freq, double duration, long int num_samples) {

  long int sampleno;

  double vol;

  vol = 0.9875;
  
  for (sampleno = 0; sampleno < num_samples; sampleno++) {

    samples[sampleno] = vol * cos((2.0 * M_PI * freq * duration * sampleno) / num_samples);

  }
    
  return 0;

}

typedef struct {

  double min_freq;

  double max_freq;
  
} zoom_param;

enum { FNONE, FRUNNING, FWAITING, FDONE };

typedef struct {

  uint64_t state;

  zoom_param *vis_zoom;

  double complex *weighted_sums;

  pthread_mutex_t *wsum_mutex;
  
  double duration;

  long int freqno;

  long int num_freqs;
  
  long int taken_count;
  
  double *samples_combined;

  long int num_samples;
  
  long int sampleno_start;

  long int sampleno_end;  

  long int num_threads;
  
  long int samplerate;
  
} fwork;

void *fourier_work(void *extra) {

  void *ret;

  fwork *fw;
  
  long int sampleno;

  double complex fvalue;
  
  double t0;

  long int takenno;

  double v;

  double freq;

  double complex weighted_sum;

  double complex precompute;
  
  zoom_param *vis_zoom;

  double percent;
  
  fw = (fwork*) extra;

  vis_zoom = fw->vis_zoom;
  
  for ( ; fw->freqno < fw->num_freqs; fw->freqno++) {

    v = fw->freqno; v /= fw->num_freqs;
    
    freq = (1.0 - v) * vis_zoom->min_freq + (v * vis_zoom->max_freq);

    percent = fw->freqno; percent /= (fw->num_freqs - 1);
    
    if (!(fw->freqno % (5 * fw->num_threads))) {
      fprintf(stderr, "%s: (freq %g) Percent complete %.2g     \r", __FUNCTION__, freq, percent * 100.0);
    }

    weighted_sum = 0.0;

    precompute = -2.0 * M_PI * I * freq;
    
    for (takenno = 0; takenno < fw->taken_count; takenno++) {

      sampleno = fw->sampleno_start;
    
      sampleno += (takenno * (fw->sampleno_end - fw->sampleno_start)) / fw->taken_count;
      
      t0 = (sampleno * fw->duration) / fw->num_samples;
    
      fvalue = fw->samples_combined[sampleno] * cexp(precompute * t0);

      weighted_sum += fvalue;

    }

    pthread_mutex_lock(fw->wsum_mutex);
    fw->weighted_sums[fw->freqno] += weighted_sum;
    pthread_mutex_unlock(fw->wsum_mutex);
    
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

enum { GEN_NONE, GEN_OWNAUDIO, GEN_PROVIDED };

int main(int argc, char *argv[]) {

  long int input_xres, input_yres;

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

  long int xpos;

  point_t pt;

  pixel_t white;

  double complex *weighted_sums;

  double audio_freqs[3] = { 7.0, 17.0, 27.50 };
  
  long int num_freqs;

  long int freqno;

  zoom_param vis_zoom;
  
  pixel_t red;

  pixel_t green;
  
  double sf;

  int retval;

  long int num_threads;

  pthread_t *threads;

  fwork *fws;

  pthread_mutex_t wsum_mutex;
  
  long int threadno;

  char *sndraw_fn;

  int fd;
  struct stat buf;
  void *m;
  
  duration = 10.0;

  samplerate = 48000;

  sndraw_fn = argc>5 ? argv[5] : NULL;

  samples_track1 = NULL;
  samples_track2 = NULL;
  samples_track3 = NULL;
  
  if (sndraw_fn != NULL) {

    fd = open(sndraw_fn, O_RDWR);
    if (fd == -1) {
      perror("open");
      return -1;
    }
    
    retval = fstat(fd, &buf);
    if (retval == -1) {
      perror("fstat");
      return -1;
    }
    
    m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
      perror("mmap");
      return -1;
    }

    samples_combined = (double*) m;

    num_samples = buf.st_size / sizeof(double);

    duration = ((double) num_samples) / samplerate;
    
  }

  else {

    m = NULL;

    fd = -1;
    
    duration = 10.0;
    
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

  }

  fprintf(stderr, "%s: Using num_samples %ld\n", __FUNCTION__, num_samples);
  
  retval = argc>1 ? sscanf(argv[1],"%ldx%ld",&input_xres,&input_yres) : -1;
  if (retval != 2) {
    fprintf(stderr, "%s: Please specify render resolution.\n", __FUNCTION__);
    return -1;
  }
  
  img_freqsweep.xres = input_xres;
  img_freqsweep.yres = input_yres;

  num_pixels = img_freqsweep.xres * img_freqsweep.yres;
  img_sz = num_pixels * sizeof(pixel_t);
  
  img_freqsweep.rgb = malloc(img_sz);
  if (img_freqsweep.rgb == NULL) {
    perror("malloc");
    return -1;
  }

  memset(img_freqsweep.rgb, 0, img_sz);
  
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
  
  num_freqs = img_freqsweep.xres * 2;

  vis_zoom.min_freq = argc>3 ? strtol(argv[3],NULL,10) : 0.0;
  vis_zoom.max_freq = argc>4 ? strtol(argv[4],NULL,10) : 0.0;

  weighted_sums = malloc(num_freqs * sizeof(double complex));
  if (weighted_sums == NULL) {
    perror("malloc");
    return -1;
  }
  
  for (freqno = 0; freqno < num_freqs; freqno++) {

    weighted_sums[freqno] = 0.0;
    
  }

  fprintf(stderr, "%s: Performing fourier transform.\n", __FUNCTION__);

  retval = pthread_mutex_init(&wsum_mutex, NULL);
  if (retval != 0) {
    fprintf(stderr, "%s: Trouble with call to pthread_mutex_init.\n", __FUNCTION__);
    return -1;
  }
  
  for (threadno = 0; threadno < num_threads; threadno++) {

    fws[threadno].state = FRUNNING;
      
    fws[threadno].weighted_sums = weighted_sums;

    fws[threadno].wsum_mutex = &wsum_mutex;
    
    fws[threadno].freqno = 0;

    fws[threadno].num_freqs = num_freqs;
      
    fws[threadno].vis_zoom = &vis_zoom;
      
    fws[threadno].duration = duration;

    fws[threadno].samples_combined = samples_combined;

    fws[threadno].taken_count = FDEPTH;
      
    fws[threadno].num_samples = num_samples;

    fws[threadno].sampleno_start = threadno * num_samples / num_threads;

    fws[threadno].sampleno_end = (threadno+1) * num_samples / num_threads;

    fws[threadno].num_threads = num_threads;
      
    fws[threadno].samplerate = samplerate;
      
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

  for (freqno = 0; freqno < num_freqs; freqno++) {
    weighted_sums[freqno] /= (FDEPTH * num_threads);
  }
  
  fprintf(stderr, "%s: Generating plot of image data.\n", __FUNCTION__);
  
  {

    double *r1, *i1, *c1;

    double max_r1, max_i1, max_c1;
    
    r1 = malloc(num_freqs * sizeof(double));
    if (r1 == NULL) {
      perror("malloc");
      return -1;
    }

    i1 = malloc(num_freqs * sizeof(double));
    if (i1 == NULL) {
      perror("malloc");
      return -1;
    }

    c1 = malloc(num_freqs * sizeof(double));
    if (c1 == NULL) {
      perror("malloc");
      return -1;
    }

    max_r1 = 0.0;
    max_i1 = 0.0;
    max_c1 = 0.0;
    
    for (freqno = 0; freqno < num_freqs; freqno++) {

      r1[freqno] = creal(weighted_sums[freqno]);
      i1[freqno] = cimag(weighted_sums[freqno]);
      c1[freqno] = cmag(weighted_sums[freqno]);

      if (r1[freqno] > max_r1) {
	max_r1 = r1[freqno];
      }

      if (i1[freqno] > max_i1) {
	max_i1 = i1[freqno];
      }

      if (c1[freqno] > max_c1) {
	max_c1 = c1[freqno];
      }      
      
    }

    fprintf(stderr, "[DEBUG] max_r1 %g max_i1 %g max_c1 %g\n", max_r1, max_i1, max_c1);
    
    for (freqno = 0; freqno < num_freqs; freqno++) {
    
      xpos = (freqno * img_freqsweep.xres) / num_freqs;

      sf = 10.0;
      
      pt.y = r1[freqno];
      pt.y *= -1.0;
      pt.y /= max_r1;
      pt.y *= sf;
      plotpoint(&img_freqsweep, "Real", xpos, pt.y, red);

      pt.y = i1[freqno];
      pt.y *= -1.0;
      pt.y /= max_i1;
      pt.y *= sf;
      plotpoint(&img_freqsweep, "Imag", xpos, pt.y, green);
    
      pt.y = c1[freqno];
      pt.y *= -1.0;
      pt.y /= max_c1;
      pt.y *= sf;
      plotpoint(&img_freqsweep, "Mag", xpos, pt.y, white);

    }
    
    free(r1);
    free(i1);
    free(c1);
    
  }
  
  fprintf(stderr, "%s: Preparing final image.\n", __FUNCTION__);

  {

    char *env_OUTSF;

    char *out_fn;
    
    env_OUTSF = getenv("OUTSF");

    out_fn = env_OUTSF != NULL ? env_OUTSF : def_outsf;
    
    fprintf(stderr, "%s: Writing a reconstructed sound file.\n", __FUNCTION__);

    {
    
      int out_fd;

      mode_t mode;

      size_t fsize;
    
      mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

      out_fd = open(out_fn, O_RDWR | O_CREAT | O_TRUNC, mode);
      if (out_fd == -1) {
	perror("open");
	return -1;
      }
    
      fsize = num_samples * sizeof(double);

      retval = ftruncate(out_fd, fsize);
      if (retval == -1) {
	perror("ftruncate");
	return -1;
      }

      {
	void *m;

	m = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, out_fd, 0);
	if (m == MAP_FAILED) {
	  perror("mmap");
	  return -1;
	}

	retval = fill_sound(m, num_samples, samplerate, vis_zoom.min_freq, vis_zoom.max_freq, num_freqs, weighted_sums);
	if (retval == -1) {
	  fprintf(stderr, "%s: Trouble reconstructing sound.\n", __FUNCTION__);
	  return -1;
	}
      
	retval = munmap(m, fsize);
	if (retval == -1) {
	  perror("munmap");
	  return -1;
	}

      }
    
      retval = close(out_fd);
      if (retval == -1) {
	perror("close");
	return -1;
      }
      
    }

  }
  
  fprintf(stderr, "%s: Writing image output to stdout.\n", __FUNCTION__);
  
  {
    int out_fd;
    ssize_t bytes_written;

    out_fd = 1;
    bytes_written = writefile(out_fd, img_freqsweep.rgb, img_sz);
    if (bytes_written != img_sz) {
      perror("write");
      return -1;
    }
  }

  if (sndraw_fn != NULL && fd != -1 && m != NULL) {

    retval = munmap(m, buf.st_size);
    if (retval == -1) {
      perror("munmap");
      return -1;
    }

    retval = close(fd);
    if (retval == -1) {
      perror("close");
      return -1;
    }
    
  }

  else if (m == NULL) {

    free(samples_track1);
    free(samples_track2);
    free(samples_track3);  

    free(samples_combined);
    
  }

  free(fws);
  
  free(threads);

  free(weighted_sums);
  
  free(img_freqsweep.rgb);
  
  return 0;

}
