# Hi!

This is it. Pretty much a history of Farbrausch tools 2001-2011. We've been
meaning to release all this for ages, in various forms, and always ended up
not doing it because "we'd just have to clean it up a bit first...".

No more. This is **not** cleaned up. This is the raw deal, some from old hard
drives, some fresh from various SVN repositories. This is code written for
a bunch of different versions of Visual Studio. Some of it is really tricky
to compile, some really easy. There's some nice clean stuff there, other parts
are just a complete mess.

I haven't tried to get any of this to run. I'll probably go over everything
and try to get it to compile with VS2010 soon, but no promises - if I can't
get it to work, then well, it won't compile :). You can still look at the
source, of course.

All of this is released either under a BSD license or put in the public
domain (stated per project). Not that you're likely to want to use most of
this code, but if you want to, we see no reason to keep you.

So what do we have in here? Here's the basic directory structure:

    genthree/             - GenThree. Used for Candytron and nothing else.
      data/               - Candytron data files.
    kkrunchy/             - kkrunchy 0.23alpha code (latest I could find)
    kkrunchy_k7/          - kkrunchy_k7 0.23a3 (didn't find a4; need to re-check)
    ktg/                  - OpenKTG texture generator. See below.
    lekktor/              - May summon Eldritch Abominations. Handle with care.
    RG2/                  - RauschGenerator 2. Used for several 64k intros.
      dopplerdefekt/      - data files for fr-029: dopplerdefekt
      einschlag/          - data files for fr-022: ein.schlag
      flybye/             - data files for fr-013: flybye
      welcome_to/         - data files for fr-024: welcome to...
    v2/                   - V2 synthesizer system. Used for all our intros, kkrieger and debris.
    werkkzeug3/           - Werkkzeug3. Used for tons of demos and intros.
      data/               - Source data for kkrieger and some test projects.
        debris/           - Source data for fr-041: debris.
        theta/            - Source data for fr-038: theta.
      w3texlib/           - Werkkzeug 3 texture lib. Used for fr-033.
      wz_mobile/          - Werkkzeug Mobile. Never got used for anything.
    werkkzeug3_kkrieger/  - kkrieger branch. Game mode in here might work. :)

So, here's the sightseeing tips:

* "ktg" is OpenKTG, a proposal for a simple but relatively powerful and
  orthogonal subset of texture generation functions - designed around 2007.
  This is really nice, clean code. If you want to generate textures for WebGL
  or something like that, turning this into pixel shaders+JS code should work
  quite well. (There's no nice editor for it, though)
* GenThree contains a bunch of text files in German with various ideas, from
  the Candytron timeframe - mostly written by Chaos. Interesting bit of
  history :)
* werkkzeug3 is a mess, but come on, debris - you know you want to... :)
* werkkzeug3_kkrieger is from a branch called "kkrieger" in our SVN repository.
  It's not the actual kkrieger code, and incorporates changes that were done
  more than one year after the original kkrieger release. It was, however,
  branched off before started not caring about breaking kkrieger compatibility
  when making changes. You have a better chance of building the game from there
  than you do from the "regular" werkkzeug3 tree - though both are unlikely to
  work.
  If someone really wants a close-to-original werkkzeug3 kkrieger tree, it should
  be possible to do dig up something from 2004 :)

As a general rule, the "master" branch contains the original, unmodified code.
There's a second branch ("vs2010") that contains project files and fixes to
make the code compile (more or less) cleanly with VS2010. If you want to
actually build any of this, that's the way to go, though of course it won't
match the original projects or have the original size; but since this
repository in general contains versions of the tools and players that are more
recent than any production released with them, it would be hard to get the
"original" executables back anyway, even if you had the necessary compilers,
tools and libraries.

Contributors (in alphabetical order):

* Fabian "ryg" Giesen: GenThree, kkrunchy, kkrunchy_k7, ktg, lekktor, RG2,
  werkkzeug3, werkkzeug3_kkrieger.
* Sebastian "Wayfinder" Grillmaier: RG2, ein.schlag, debris, kkrieger.
* Tammo "kb" Hinrichs: V2, RG2, flybye, "welcome to".
* Thomas "fiver2" / "theunitedstatesofamerica" Mahlke: werkkzeug3, debris,
  kkrieger.
* Christoph "giZMo" Muetze: genthree, Candytron, RG2, flybye, "welcome to",
  werkkzeug3, wz_mobile, debris, kkrieger.
* Dierk "Chaos" Ohlerich: GenThree, lekktor, werkkzeug3, werkkzeug3_kkrieger,
  wz_mobile.
* Kai "cp" Poethkow: dopplerdefekt, ein.schlag, theta.
* Ronny Pries: debris.
* Dennis "Exoticorn" Ranke: RG2, flybye, werkkzeug3_kkrieger.
* Leonard "paniq" Ritter: V2, theta.
* Bastian "Tron" Zuehlke: werkkzeug3.


Have fun!
(released April 2012)
