# Hi!

This is it. Pretty much a history of Farbrausch tools 2001-2011. We've been
meaning to release all this for ages, in various forms, and always ended up
not doing it because "we'd just have to clean it up a bit first...".

No more. This is **not** cleaned up. This is the raw deal, some from old hard
drives, some fresh from various SVN repositories. This is code written for
a bunch of different versions of Visual Studio. Some of it is really tricky
to compile, some really easy. There's some nice clean stuff there, other parts
are just a complete mess.

The original unmodified code (with a few bug fixes) is archived under the
"original" tag, in case there's historical interest, but as of April 16, 2012
the master branch of this repository contains code that builds with Visual
Studio 2010 (which is presumably more useful to people). We originally
anticipated that getting everything to work with a recent compiler would
prove difficult, but it turned out to be fairly easy and required only small
changes to the code base, so there's little value in keeping the two branches
separate.

All of this is released either under a BSD license or put in the public
domain (stated per project). Not that you're likely to want to use most of
this code, but if you want to, we see no reason to keep you.

So what do we have in here? Here's the basic directory structure:

```
  altona_wz4/           - Altona and Werkkzeug4. Our most recent code foundation and tool.
    altona/             - The framework libraries (includes base Werkkzeug4 GUI).
    wz4/                - Wz4FRlib (demo ops) and Wz4Player.
  altona2/              - successor of altona
    altona2/            - The framework libraries (Werkkzeug5 is not yet released)
  genthree/             - GenThree. Used for Candytron and nothing else.
    data/               - Candytron data files.
  kkrunchy/             - kkrunchy 0.23alpha code (latest we could find)
  kkrunchy_k7/          - kkrunchy_k7 0.23a4/asm07 (most recent version)
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
```

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
  branched off before we started not caring about breaking kkrieger
  compatibility when making changes. You have a better chance of building the
  game from there than you do from the "regular" werkkzeug3 tree - though both
  are unlikely to work.
  If someone really wants a close-to-original werkkzeug3 kkrieger tree, it should
  be possible to dig up something from 2004 :)
* altona_wz4 should actually be fully functional. It has been tested and there are
  binaries which should function as fully-featured demomaker without the need to
  touch any code. Also it's a good foundation to write your own engine or game
  or tool or whatever. This stuff has seen heavy duty use in several companies and
  went through several actual QA departments. It works.

Contributors (in alphabetical order):

* Fabian "ryg" Giesen: GenThree, kkrunchy, kkrunchy_k7, ktg, lekktor, RG2,
  werkkzeug3, werkkzeug3_kkrieger, altona, werkkzeug4
* Sebastian "Wayfinder" Grillmaier: RG2, dopplerdefekt, ein.schlag, debris,
  kkrieger
* Tammo "kb" Hinrichs: V2, RG2, altona, werkkzeug4, flybye, "welcome to", easterparty
* Thomas "fiver2" / "theunitedstatesofamerica" Mahlke: werkkzeug3, werkkzeug4,
  debris, kkrieger.
* Christoph "giZMo" Muetze: genthree, Candytron, RG2, flybye, "welcome to",
  werkkzeug3, wz_mobile, debris, kkrieger.
* Dierk "Chaos" Ohlerich: GenThree, lekktor, werkkzeug3, werkkzeug3_kkrieger,  
  wz_mobile, altona, werkkzeug4
* Kai "cp" Poethkow: dopplerdefekt, ein.schlag, theta.
* Ronny Pries: debris.
* Dennis "Exoticorn" Ranke: RG2, flybye, werkkzeug3_kkrieger.
* Leonard "paniq" Ritter: V2, theta.
* Bastian "Tron" Zuehlke: werkkzeug3, werkkzeug4

Both altona_wz5 and altona2 contain various of the stb_??? libraries from Sean
Barret, nothings.org.

Have fun!
(released April 2012)
(updated October 2014 for Altona2)
