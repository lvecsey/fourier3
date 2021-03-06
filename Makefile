
CC=gcc

MEMCHECK=-fsanitize=address,undefined -fno-omit-frame-pointer

CFLAGS=-O3 -Wall -g -pg # $(MEMCHECK) 

LIBS=-lm -lpthread

4K = 3840x2160

1080P = 1920x1080

720P = 1280x720

/tmp/output_fourier3.png : RENDER_RES=$(4K)

/tmp/output_fourier3.png : OUTPUT_RES=$(1080P)

/tmp/output_fourier3.png : NUM_THREADS=4

/tmp/output_fourier3.png : fourier3
	@./$^ $(RENDER_RES) $(NUM_THREADS) 0.0 60.0 | convert -size $(RENDER_RES) rgb:- -resize $(OUTPUT_RES) -depth 32 $@

/tmp/output_fourier3-sndfile.png : RENDER_RES=$(4K)

/tmp/output_fourier3-sndfile.png : OUTPUT_RES=$(1080P)

/tmp/output_fourier3-sndfile.png : NUM_THREADS=4

/tmp/output_fourier3-sndfile.png : SNDRAW_FN='/tmp/process_audio.raw'

/tmp/output_fourier3-sndfile.png : fourier3
	@./$^ $(RENDER_RES) $(NUM_THREADS) 0.0 22050.0 $(SNDRAW_FN) | convert -size $(RENDER_RES) rgb:- -resize $(OUTPUT_RES) -depth 32 $@

/tmp/outfile_fourier3-snd.wav : /tmp/outfile_fourier3-snd.raw
	ffmpeg -f f64le -i $^ -ar 48000 -ac 1 $@

/tmp/output_fourier3-wide.png : RENDER_RES=7680x2160

/tmp/output_fourier3-wide.png : OUTPUT_RES=3840x1080

/tmp/output_fourier3-wide.png : NUM_THREADS=4

/tmp/output_fourier3-wide.png : fourier3
	@OUTSF=/tmp/output_fourier3-sndfile.raw ./$^ $(RENDER_RES) $(NUM_THREADS) 0.0 60.0 | convert -size $(RENDER_RES) rgb:- -resize $(OUTPUT_RES) -depth 32 $@

fourier3: cmag.o fill_sound.o writefile.o fourier3.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

.PHONY:
clean:
	-rm *.o fourier3
