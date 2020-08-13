First, the program creates a combined signal with a few frequencies in it, such as *7.0*, *17.0*, and *27.50*. A fourier transform is then performed to extract those frequencies from the combined signal. A visual output shows a frequency sweep with spikes where the original frequencies match. The visual output is effectively zoomed in to a range of about 3.5 to 55.0 Hz for this example.

The audio is stored as double values and with a sample rate of 48000 Hz.

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

[Sample image](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)

![Image of output](https://phrasep.com/~lvecsey/software/fourier3/output_fourier3.png)
