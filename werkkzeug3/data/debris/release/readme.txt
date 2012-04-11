                          farbrausch @ breakpoint 2007
                              fr-041: d e b r i s .

credits:
--------

  visuals, direction: theunitedstatesofamerica (aka fiver2)
  code: chaos, ryg
  soundtrack: ronny
  synthesizer: kb
  sound effects: wayfinder
  additional graphics: giZMo
  additional code: tron
  special thanks: fried
  
system requirements:
--------------------

- minimum:
  * p4 2ghz or athlon 2000+ (with sse)
  * 512mb ram
  * ps2.0 capable graphics card with 128mb vram
  * directx 9.0c
  
- recommended:
  * core2duo or athlon x2 with >=2.4ghz
  * 1024mb ram
  * geforce 7600/radeon x1600 or better with 256mb vram
  
settings:
---------

- resolution: pick your favourite.
- aspect ratio: 
  * for fullscreen, pick one that matches your screen.
  * for windowed, it should match your desktop resolution.
- multisampling: if you have a fast graphics card, go wild here.
- texture quality:
  * normal: lower-than-default texture resolution+dxt compressed textures.
    be sure to pick this if you have 512mb ram. (also speeds up loading time)
  * high: default texture size, dxt compressed textures. recommended setting.
  * ultra: default texture size, no texture compression. pick this if your
    system is too fast and you want to burn fillrate and bandwidth for
    a barely visible improvement.
- full screen: leave it on if possible :)
- wait for vsync: looks better with vsync on, but turn it off if you want.
- shadows: you can turn off realtime shadowcasting here. makes things faster
  (especially with older graphics cards) but definitely looks worse.
- loop demo: turn it on and just don't touch escape, baby!

blah-section:
-------------

  hm.
  
  maybe for the final? :)
  (tired, compos to organize, kthxbye! :)

tech notes:
-----------
  
  at its heart this is still the werkkzeug3/kkrieger engine, except with a
  new(*) lighting/material system. models and textures are still generated.
  (though we don't use the sizeoptimized versions of everything, which made
  our lives a lot easier). we also still use shadow volumes, which makes the
  demo somewhat cpu-hungry, especially towards the end, since we have to
  transform a lot of the animated models by hand. but we didn't want to change
  the whole lighting pipeline in the middle of a project.
  
  (*): it was actually used for some scenes in fr-048: precision, but (as you
  can see from the fr-number) that was started a lot later than debris, so this
  demo is atleast historically its first real user)
  
contact:
--------

  www.farbrausch.com
  www.theprodukkt.com
  
  .werkkzeug3 texture edition soon to be available.
  stay tuned and check the .theprodukkt website for details.