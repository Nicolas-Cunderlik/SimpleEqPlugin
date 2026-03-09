<img width="99%" alt="EqDemoImage" src="https://github.com/user-attachments/assets/a2f1fdec-f1db-478d-8d7e-67aea8a4d9e9" />

Instructions:
- The standalone .exe can be launched and used as-is

- To use the plugin in a DAW, download the .vst3 plugin and move it to your common .vst3 directory, typically C:\Program Files\Common Files\VST3 on Windows
- Then, refresh your plugin list (assuming it checks the directory) for it to appear
- Find the plugin in your list of plugins and attach it to a mixer channel to use it

This project is the beginning of my DSP journey.
I plan to use it as a stepping stone for implementing a ML algorithm for polyphonic key detection and audio-to-MIDI conversion.

The equalizer has low pass, peak, and high pass filters with adjustable parameters.
The low pass and high pass filters use my own implementation of a high order Butterworth IIR filter, found in BandPass.cpp/h.

Note that this repo only contains the source files for the project. You cannot rebuild the project unless you install JUCE.
