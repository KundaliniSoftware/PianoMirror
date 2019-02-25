# About

"**The Kundalini Piano Mirror** a program for real-time transposition (remapping) of midi messages, designed for use with a standard midi piano keyboard.

The remapping performed results in changing which pitches are produced from which notes, making novel ways of playing the piano keyboard possible.

Specifically, the Piano Mirror software makes it possible to either mirror left-handed passages into the right hand, mirror right-handed passages into the left hand, or to completely reverse the keyboard such that the left hand plays the original right-hand part in mirror image, while the left hand simultaneously plays the original right-hand part in mirror image.

[..]"

For a complete explanation of how this software works, and why it is useful, please read this post:

https://www.kundalinisoftware.com/piano-mirror/

## Hardware

To utilize this software, it is necessary to have a USB-to-Midi interface between a Windows computer and a Midi Keyboard that features a "local off feature". (Local off means for the digital piano to send midi messages each time a key is pressed, but to not produce sound. Please consult the documentation for your digital keyboard to verify if "local off" is available, and if so, how to enable it.)

Assuming local off is enabled and the digital piano is connected to a computer running this software, the purpose of the Piano Mirror is to echo incoming midi messages to midi out, after performing a suitable transpostion. This transposition (or remapping) has the effect of transforming which physical key on the piano corresponds to which sound that is produced. 

## About this repository

This gibhub repository contains the Visual Studio Project for creating a Win64 console App to perform the functionality described above by using the excellent PortMidi library:

http://portmedia.sourceforge.net/portmidi/

Currently no installers exist, I just relased the source. 

Benjamin Pritchard / Kundalini Software
24-Feb-2019
