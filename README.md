First, the program creates a combined signal with a few frequencies in it, such as *7.0*, *17.0*, and *27.50*. A fourier transform is then performed to extract those frequencies from the combined signal. A visual output shows a frequency sweep with spikes where the original frequencies match. The visual output is effectively zoomed in to a range of 0.0 to 60.0 Hz for this example.

```console
./fourier3 1920x1080 4 0.0 60.0 | display -size 1920x1080 rgb:-
```

The audio is stored internally as double values and with a sample rate of 48000 Hz.

# Compiling

```console
make fourier3
```

# Rendering

```console
make /tmp/output_fourier3.png
feh --fullscreen /tmp/output_fourier3.png
```

By default the program uses 4 threads for computation.

You can also specify a signal file, of double values. Samplerate should be 48000. Change the number of threads as needed, for example as a power of 2.

```console
./fourier3 1920x1080 4 0.0 22050.0 infile.raw | display -size 1920x1080 rgb:-
```

You can also specify the environment variable OUTSF to be a raw output audio file. It is a reconstructed sound file based on the magnitudes of the various frequencies that were extracted.

[Sample image](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)

![Image of output](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)
