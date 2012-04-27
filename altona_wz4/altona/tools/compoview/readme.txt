/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

= CompoView V1.2 =

CompoView is a simple image viewer designed to do a graphics competition at
a demo party. It allows the competition organizer to smoothly zoom into and
scroll the images to highlight details.

Version 1.0 of this tool was used at Revision 2011.

== Credits ==

Code: Chaos/Farbrausch

Poweruser: Fashion/Farbrausch

Using Altona.

== Usage ==

Create a directory with all the images of the competition, working steps
and beam slides. Bring them into the correct alphabetical order. put the
executable (32 or 64 bit) into the directory and start it.

It will load all the images at start-up, that can take some time.

Then you can scroll with the left mouse button and zoom with the mouse wheel.
Press SPACE to center the image.

You can switch the next and previous image with the cursor keys.

With F1 you can toggle the help display and review your current settings.

With F2 you can toggle on and of filtering. For old-school graphics you will 
want filtering to be switched of, for modern graphics you might want it to
be on.

With F3 you can switch between 3 zooming modes:
* "smooth zoom"
  Images initially fill the whole screen, and you can zoom smoothly.
* "initial integral"
  Images are initially zoomed only by integral factors (1,2,3,4...), but
  you can still zoom smoothly. This avoids any image filtering of the 
  initial display. If the image is larger than the screen resolution, it
  will be down-scaled to best fit.
* "only integral"
  Images will only be zoomed by an integral factor. Not that useful.

== Details ==

* Use the 64 bit version when you can.
* You can abort the loading process by pressing escape. You will then be
  able to use the tool with all the images that have been loaded.

This used stb_image to load PNG's and JPG's. This introduces some problems:

* Some rare variants of PNG and JPG can not be loaded.
* Although stb_image is a very solid library, it is not hardened against
  attacks.

How to be paranoid in the preparation of your competition:

* Well before the competition, check if it works on the same computer you
  will be using during competition.
* Check all images. Are they in the correct order? Are they displayed 
  correctly? Does the tool crash in the middle? Do this on the same
  computer you will be using during competition.
* This is a party hack. It might crash any moment. Have a plan b.

== Change History ==

V1.2:

* Display Free Memory
* Add ALT-F4 to exit
* Initial public release
* Some cleanup in help menu.

V1.1:

* Added zoom modes (F3)

V1.0:

* Used at Revision 2011


