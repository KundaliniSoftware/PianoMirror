# PianoMirror

Kundalini Piano Mirror for creating a mirror image digital piano. 

To utilize this software, it is necessary to have a USB-to-Midi interface between a Windows computer and a Midi Keyboard that features a
"local off feature". (Local off means for the digital piano to send midi messages each time a key is pressed, but to not produce sound. 
Please consult the documentation for your digital keyboard to verify if "local off" is available, and if so, how to enable it.)

Assuming local off is enabled and the digital piano is connected to a computer running this software, the purpose of the Piano Mirror is to
echo incoming midi messages to midi out, after performing a suitable transpostion. This transposition (or remapping) has the effect of
transforming which physical key on the piano corresponds to which sound that is produced. 

The Piano mirror works in 4 mode:

## no transposition

<info coming soon>

## left hand ascending mode 

<info coming soon>

## right hand descending mode

< info coming soon>

## mirror image mode

< info coming soon > 


This gibhub repository contains the Visual Studio Project, for creating a Win64 console App to perform the functionality described
above by using the excellent PortMidi library:

http://portmedia.sourceforge.net/portmidi/

User documentation for the Piano Mirror can be found at: https://www.kundalinisoftware.com/piano-mirror/
