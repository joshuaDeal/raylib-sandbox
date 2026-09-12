#!/bin/bash
# Not very fleshed out right now. Mostly just here for reference.
# This is how I made the sound the donut produces.

# Make the sound
sox -n -r 44100 -c 1 up.wav synth 1 triangle 50/80 #gain -18
sox up.wav down.wav reverse
sox up.wav down.wav double-sweep.wav
sox double-sweep.wav double-sweep.ogg

# Clean up
rm {up,down,double-sweep}.wav
