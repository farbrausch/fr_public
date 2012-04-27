
libv2 1.5 - Farbrausch Consumer Consulting's famous tiny synthesizer 
and music player for YOU.

(C) Tammo 'kb' Hinrichs 2000-2008
Placed under the Artistic License 2.0, see LICENSE.txt for details

This archive consists of libv2 (as found in libv2.lib and libv2.h),
the V2M player as source (v2mplayer.cpp and v2mplayer.h) and an 
example program which shows how to use its basic functions and how
to produce small Windows .EXEs with Visual C++.

To include libv2 the only thing you have to do is copying the four
files into your project directory, including them into your project
and then calling a few functions (look into libv2.h and the example
for documentation). Simple as that.

The version 1.5 does feature a few improvements; for musicians there's
a plethora of added synthesizer features to use, and for programmers
this release is way better than the old one:

- The synthesizer, the V2M player and the DirectSound output code are
  now separated from each other. This means you can easily implement
  stuff like own sound output, disk writers, multiple synth instances
  running at once and much more.
  
- You can now get VU meter information from the synthesizer for the
  single channels; this should make way for some good visualisation
  ideas
  
- The V2M player now comes as full source, and the low-level synth 
  functions are exposed and kind of documented; that means if you want 
  you can make the synthesizer play other things than mere V2M files, 
  you can eg. hand-feed it with MIDI data that you've generated yourself,
  or you can exchange the speech synthesizer texts on the fly. The
  possibilites are endless :)

- But be warned, much of this exposition comes simply because I was
  kinda fed up with all the pleas for support and improvements. You
  can be pretty much sure I'll be unresponsive as ever; there's other
  things in my life than constantly maintaining this 7.5 year old
  code base. Thank you :)

So, have fun with it (I certainly had),

  Tammo 'kb' Hinrichs
  farbrausch consumer consulting
  