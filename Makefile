
CC=gcc

CFLAGS=-O3 -Wall -g -pg

LIBS=-lm -lpthread

4K = 3840x2160

1080P = 1920x1080

720P = 1280x720

/tmp/output_fourier3.png : RENDER_RES=$(4K)

/tmp/output_fourier3.png : OUTPUT_RES=$(1080P)

/tmp/output_fourier3.png : fourier3
	@./$^ $(RENDER_RES) | convert -size $(RENDER_RES) rgb:- -resize $(OUTPUT_RES) -depth 32 $@

fourier3 : writefile.o fourier3.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
