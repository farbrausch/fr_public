To whom it may concern,

Yes, this source code is a total mess. Good luck getting it to compile - I had
to take out lots of things to make a source release possible. So before you even
try opening the VS project make sure you have NASM or YASM installed and then
add the WTL and the plugin SDKs for Winamp2 (or 5), VST2 and Buzz into the 
source tree at the denoted locations (PLACE_..._HERE). And even then it probably
won't work because the main VS project itself is in no shape to compile. But in theory
all the source code for compiling the synth library and plugins is there, so it 
should be doable. :)

There is a VS2010 project tho that successfully compiles the libv2, conv2m.exe
(converts old V2Ms to the newest version of the synth) and the tinyplayer 
example, even fully size optimized in Release mode.

Also the synth core in synth.asm is fully self contained, if you leave out the
speech synthesizer by undefining RONAN (greetings btw ;) and figure out how
to set the patch data you will get a fully capable synthesizer that happily 
converts MIDI notes to waveform data. If I were you this would actually be the
point to start.

Serious inquiries (and by serious I mean either detail questions that tell me
that you got the obvious stuff or volunteers to port the thing to C++ to make
it actually maintainable) to kb@kebby.org :)

That's all for now,
   Tammo "kb" Hinrichs
   