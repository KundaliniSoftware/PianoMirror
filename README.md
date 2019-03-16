﻿# About

**The Kundalini Piano Mirror** is a program for real-time transposition (remapping) of midi messages. 

By echoing incoming midi messages to midi out -- after performing a suitable transposition -- this software facilitates changing which *pitches* are produced from which *physical keys*, making novel ways of playing the piano keyboard possible.

Specifically, the Piano Mirror software makes it possible to either mirror left-handed passages into the right hand, mirror right-handed passages into the left hand, or to completely reverse the keyboard such that the left hand plays the original right-hand part in mirror image, while the left hand simultaneously plays the original right-hand part in mirror image.

These three remapping types are referred to as "Left Hand Ascending Mode", "Right Hand Descending Mode", and "Mirror Image Mode".

For additional information and explanation of these modes, along with background information on symmetrical inversion and why this software is useful, please see the website for this project: https://www.kundalinisoftware.com/piano-mirror/

#Hardware

To utilize this software, it is necessary to have a USB-to-Midi interface between a Windows computer and a Midi Keyboard that features a "local off feature". Enabling "Local off" mode the digital piano to send midi messages via midi out each time a key is pressed, but to not produce any sound. Instead, the keyboard will produce sound only for *incoming* midi messages -- which in this case, will be the remapped messages sent by the Piano Mirror software.

Please consult the documentation for your digital keyboard to verify if "local off" is available, and if so, how to enable it. Additionally, please note that if local off is not available, this software will not function as intended.


## Getting started

Complete documentation (and pre-built binary installers) can be found at:

https://www.kundalinisoftware.com/piano-mirror/

## About this repository

This gibhub repository contains a Visual Studio Project for creating a Win32 console App to perform the functionality described above by using the excellent PortMidi library:

http://portmedia.sourceforge.net/portmidi/

Benjamin Pritchard / Kundalini Software
24-Feb-2019

