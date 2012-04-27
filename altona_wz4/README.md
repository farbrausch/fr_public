README for Altona and Werkkzeug4
================================

So, you want to get going with Werkkzeug4? Just read on!

The very easy way
------------------

try wz4/bin - there you'll find Werkkzeug4 for creating a demo and the 
wz4player for releasing one. 

Just start Werkkzeug4.exe and hack away. If we're nice we'll write 
instructions at some point, or somebody else will.

If you want to ship a demo made with Wz4, please first check the "Document
Options" because you can state the name of your production and a handful of 
options there. The operator that gets shown as demo is "root" as always, and if
you want to have a fancy loading bar, try "loading" as op name (the progress 
will be used as time between 0 and 1 (16 beats)).

Then to ship, just call wz4player with

    wz4player path_to_wz4_file --pack name_of_packfile.pak
	
... and bundle the player exe and the .pak. Easy as pie. :)


Compiling
----------

So basically there's two directories. 

"Altona" contains our aptly named framework for graphics, sound, IO and the 
rest of the essential stuff that's needed for creating demos, games and GUI 
tools. You will also find an "empty" Wz4 there which has the full GUI but 
there are no operators that actually do anything.

"Wz4" contains the library with the "Farbrausch engine" and all the effects,
and also a GUI and player version of the Wz4 that uses this library. That's
where the demos come from.

Step 0: Make sure you have at least Visual Studio 2008 (Express versions will 
do, also VS2005 should work but hasn't been tested for a long time), the
DirectX SDK and YASM.

Step 1: Look for "altona_config_sample.hpp" in the altona/ dir, copy it to
"altona_config.hpp", open that and change the constants found therein (most
prominently the VS version). Make sure altona/bin is in the PATH from now on, 
then open a command line and type

    makeproject -r path_to_source 
   
If everything goes according to plan, which it will because we've got a PhD in 
awesomeness, you should now find solution and project files in every directory.

Step 2: Open the wz4/werkkzeug4 or wz4/wz4player projects, compile, enjoy.

Compiling (the hard way)
------------------------

To create all the tools found in altona/bin/ from scratch you need to locate the
"bootstrap" project in altona/tools/makeproject/bootstrap - this should build
without any further dependencies. Create the makeproject.exe, then call it
(if you got sCONFIG_CODEROOT_WINDOWS in altona_config.hpp right, you can from now
on omit the -r parameter). The VS projects should now be created and you can 
proceed to compile at least ASC and Wz4Ops (in this order). Put all executables
in the PATH and Werkkzeug should compile.


That's it for now - more docs or blog entries will surely follow, this codebase
is actually good for something. Have fun poking around or even making demos
with wz4. Or even writing your own engine and using wz4 as editor, or use
it as GUI for creating VMs to be used in 4K intros - that's exactly what it's for.

Sincerely,
Tammo "kb" Hinrichs / farbrausch
 