First, the program creates a combined signal with a few frequencies in it, such as *7.0*, *17.0*, and *27.50*. A fourier transform is then performed to extract those frequencies from the combined signal. A visual output shows a frequency sweep with spikes where the original frequencies match.

# Compiling

```console
make fourier3
```
# Rendering

```console
make /tmp/output_fourier3.png
feh --fullscreen /tmp/output_fourier3.png
```

# Command line arguments

```console
./fourier3 1920x1080 4 0.0 60.0 | display -size 1920x1080 rgb:-
```

First the output resolution is spcified (raw image data), followed by number of threads to use which should be a power of 2.

The 0.0 to 60.0 represents that the visual output is effectively zoomed in to that range of frequencies (minimum freq and maximum freq)

The audio is stored internally as double values and with a sample rate of 48000 Hz.

# Using a signal file

You can also specify a signal file, of double values. Samplerate should be 48000. Change the number of threads as needed, for example as a power of 2.

```console
./fourier3 1920x1080 4 0.0 22050.0 infile.raw | display -size 1920x1080 rgb:-
```

# Output file of reconstructed audio

You can also specify the environment variable OUTSF to be a raw output audio file. It is a reconstructed sound file based on the magnitudes of the various frequencies that were extracted.

[Sample image](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)

![Image of output](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)
