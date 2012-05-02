# About OpenKTG
This directory contains OpenKTG. This dates back to 2007, when we were asked
to write a proposal for what was to be an open texture generation standard.
OpenKTG takes a lot of the ideas from our existing texture generators, but was
designed to be simple to describe and orthogonal, much more so than any of our
texture generators up to that point had been. It also fixes a bunch of
conceptual problems: for example, this is the first of our texture generators
to properly respect (premultiplied) alpha everywhere.

This code was designed to function as a reference implementation and is
written for clarity and simplicity, not speed. Textures are represented with
full 16 bits per color channel (without any precision reduction as in RG2
or Werkkzeug3), and everything gets rounded properly. Also, there are
consistent rules for texture sampling/filtering and pixel centers that are
designed to make compliant hardware implementations easy: texture and pixel
coordinates match OpenGL / OpenGL ES and D3D10+ rules, for instance.

In short, this code is not fast, but some care was taken to ensure that fast
and correct implementations would be possible and reasonably easy. It seems
like a good starting point for ports to other execution environments
(particular pixel/compute shader based texture generation), definitely more so
than the original Werkkzeug3 texture generator.

## Building
You can either use the provided solution for MS Visual Studio 2010 or CMake
to build the OpenKTG library and a small demo executable.

MS Visual Studio versions prior to 2010 will need a C99 compatible 'stdint.h'
header to build this because they don't ship one themselves. A reasonably
compatible free implementation which is available under a BSD license can
be downloaded from http://msinttypes.googlecode.com/
