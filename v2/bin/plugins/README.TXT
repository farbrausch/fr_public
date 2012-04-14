Welcome to the farbrausch consumer consulting V2 synthesizer system.

For the mandatory license and disclaimer texts, please scroll down.


I apologize for lack of thorough documentation, but I wanted to release
it now; and I promise better docs will be released later. But for the
moment I'm assuming you're good enough to install and operate VSTi and
Buzz plugins. If this is not the case, don't try to contact me for
support, your emails will be deleted instantly.


- Installation
--------------

There are three kinds of plugins available: A VST instrument, a Buzz
machine and a Winamp plugin for playing back songs.

- To use the VSTi, copy the two files in the "VSTi" folder to your
  host application's plugin directory (normally it's called VSTPlugins
  or sth like that)
  
- To use the Buzz machine, copy the two files in the "Buzz" folder to
  Buzz's machine directory
  
- To use the Winamp plugin, copy the file in the "Winamp" folder to
  Winamp's "plugins" directory.
  
  
  
- Using the VST instrument
--------------------------

A warning first: The V2 instrument is multitimbral. This means it supports
several channels and so must your host application. If you want to make
complete songs with the V2 (instead of using it "only" as one softsynth
of many) there has to be only ONE instance of the plugin, and you'll have
to use the different channels. If there's more than one instance running,
it will be impossible to write out the .v2m files you'll need for including
the song somewhere else.

Another "quirk" that results from the plugin's multitimbrality is that changing
the program in the VST host WILL NOT change the synthesizer's current program,
but rather it will only set the editor GUI to that program. To change programs
for playback, you'll have to send MIDI program change commands, either by manually
inserting them as events or by a "program" field in your "track properties", whatever
software you may be using.

So just treat the V2 like one external MIDI synth and everything should be fine.



- Using the Buzz machine
------------------------

Like the VST plugin above, also the Buzz machine is multitimbral and there's essentially
only one instance running. That means that the first instance you open will be the "master"
and all others will run as "slave" and run through the output of the "master" machine.
Other than that the machine's pretty normal.



- Composing
------------------------

In the GUI you'll find the usual file/edit buttons and a few status displays in the upper
area. In the lower, there are three tabs, "Patch", "Global" and "Speech". 

In the "Patch" screen you'll find all synth parameters of the currently selected patch to the left, and
to the right there's the "modulation list" where you can assign controllers or LFOs/EGs
to about every parameter there is.

The "Global" screen consists of a few parameters which affect the two AUX busses and their
effects, as well as a few insert effects for the sum signal. Yes, there's only ONE global
section and you can't change this per sound or assign controllers to it.




- The speech synth
------------------------

The speech synth is a special insert effect which resides on Channel 16 (no, there's no
way to turn it off). You can define text lines in the "Speech" screen of the editor,
where you find an adaptive english-to-phonemes translator at the top and up to 64
strings (of phonemes) at the bottom. In the phoneme strings there are two special
chars: "!" makes the speech parser wait for a note-on signal, "_" will make it
wait for a note_off. That way you can synchronise the speech to the notes.
Additionally there are two hardcoded MIDI CCs: CC#4 is responsible for selecting
the currently spoken/sung line (from 0 to 63) and its speed (from 64 (fast) to 
127 (slooooooooooow)), CC#5 can change the voice's color from male to female or
various comic figures.

The voice's "voicing source" is the sound coming out of whatever you play on Channel
16.


- Writing out a song
------------------------

To export a song for use in the Winamp plugin, the .werkkzeug or your own stuff 
(using libv2) there's the "record button":

1. Rewind your music program and make sure everything's set up correctly
2. Press "record". The button will stick and display "stop" instead.
3. Let your song play. The V2 plugin will record everything that goes
   through the synth, ...
4. ... until the song is over and you press the button (which now reads
   "Stop") again. 
5. In the fileselector that shows up, type in a filename and you're set.

The resulting .v2m files can then be played back in the Winamp plugin or used
directly in .werkkzeug or with libv2. Don't worry if the file is somewhat
bigger than you expected - it's optimized to be compressed afterwards. If you
compress it with eg. Zip or include it in your own .exe and run an executable
compressor on it, it should be much, much smaller.


That kind of was it for the preliminary documentation - IMHO it should be enough
to enable you to use the V2 to produce and re-use own songs. One last word:
Please refrain from using the preset sounds too extensively - they've all been
used in various farbrausch productions and most of them are kinds of "trademark
sounds" that are recognizable way too easily.

If everything works so far and you THEN have questions, you can of course contact
me with them - I won't delete your mail. Hopefully. If you make a competent impression
at least ;)


Ok, have fun with the plugins and please try to make GOOD music with them, they're
worth it,

   Tammo "kb" Hinrichs
   farbrausch 
   



LICENSE:

This program is freeware. This means you can use it as you wish, and
distribute it as long as all the contents of the archive including this
text file and the directory structure remain unmodified. You are not
allowed to charge any fee for redistributing this archive; of course this 
excludes the usual copying costs.

This archive and all the contained programs are (C) 2004 Tammo "kb" Hinrichs,
all rights reserved. For any questions contact me at kb@1337haxorz.de

DISCLAIMER:

BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY 
GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE 
USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF 
DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD 
PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), 
EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF 
SUCH DAMAGES.