Simulates the VolksEEG acquisition unit, e.g. the device that acquires EEG signals and sends them to the PC via USB.

Notes:

1. Developed using platform.io with the Arduino platform.
1. Uses the *SdFat - Adafruit Fork* library.
1. Uses the 8 channel, 16-bit simple packet format as described at https://github.com/VolksEEG/VolksEEG/wiki/EEG-Box-to-PC-Streaming-Protocol
1. Reads the EEG from an EDF file named *output.edf*, located on the root directory of an SD card.
1. In theory, the EDF file can contain any number of channels and the application will ignore or pad channels as needed to get to 8 channels. In reality, it's only been tested with an 8-channel EDF file.
1. Only EDF channels sampled at the same rate as channel 0 will be used; other channels will be ignored.
1. Data begins streaming as soon as the application starts running
1. This has only been tested with a single EDF file, supplied within this repo as /test_eds/output.edf. This should be copied to your SD cards' root.
1. Currently outputs on the Feather's dedicated hardware serial port - RX and TX pins coming out from the board, rather than using the Freather's built-in USB. Will try to switch to the built-in USB in the future, there was previously a challenge with this. 15,200 n, 8, 1