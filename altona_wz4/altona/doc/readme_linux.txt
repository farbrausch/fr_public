Altona/Linux
============

The Linux port only supports bare bones File IO and (soon) a bit of networking. Memory
mapped files are supported, asynchronous reads aren't. Nothing even remotely GUI-related
is properly supported yet, and pretty much the same goes for input.

Basically, if you want to do more than console tools, you're screwed right now.

However, there's one new feature that may be interesting for linux devs: Outputting debug
messages to the console is pretty hard to follow, especially if you're working on console
tools. So I've redirected debug messages in console builds. They're now written to a file
called "altonadebug" in your home directory. The idea is that you do this once:

  mkfifo ~/altonadebug

And from then on, you can do

  tail -f ~/altonadebug -c +0

In some extra console window. Tadaa - this is about as good as the Visual Studio "Output"
Window. Have fun.

On sSysLog: Messages are sent to syslog. If you don't call sSysLogInit, 
messages will be sent from the "user" facility, otherwise "local0" will 
be used. The idea is that you use sSysLogInit only for server processes 
and the like.

You need to configure syslog to do something sensible with these 
messages; add a line like

  local0.*                          -/var/log/altona_server.log

(or something similar) to /etc/syslog.conf and restart syslogd.
