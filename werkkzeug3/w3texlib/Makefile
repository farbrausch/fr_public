# Submit variables: OUTDIR OUTFILE BUILDDIR CFLAGS

BD=$(BUILDDIR)
OF=$(OUTDIR)\\$(OUTFILE)

OBJS=$(BD)\\w3texlib.obj $(BD)\\w3texmain.obj $(BD)\\genbitmap.obj $(BD)\\genvector.obj \
	 $(BD)\\_types.obj $(BD)\\_start.obj $(BD)\\_rygdxt.obj

$(OF): $(OBJS)
	lib /nologo /out:$@ $**

dist: $(OF)
	copy w3texlib.hpp $(OUTDIR)

clean:
	@del $(OF) $(OBJS) $(OUTDIR)\\w3texlib.hpp 2>nul
	
$(BD)\\w3texlib.obj: w3texlib.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\w3texmain.obj: w3texmain.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\_rygdxt.obj: _rygdxt.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\genbitmap.obj: genbitmap.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\genvector.obj: genvector.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\_start.obj: _start.cpp
	cl /c $(CFLAGS) /Fo$@ $**

$(BD)\\_types.obj: _types.cpp
	cl /c $(CFLAGS) /Fo$@ $**

w3texlib.cpp: _config.hpp _types.hpp _rygdxt.hpp w3texlib.hpp w3texmain.hpp
w3texmain.cpp: _config.hpp _types.hpp _rygdxt.hpp _start.hpp genbitmap.hpp w3texlib.hpp w3texmain.hpp
genbitmap.cpp: _config.hpp _types.hpp _start.hpp genbitmap.hpp
genvector.cpp: _config.hpp _types.hpp _start.hpp genbitmap.hpp genvector.hpp
_rygdxt.cpp: _config.hpp _types.hpp _rygdxt.hpp
_start.cpp: _config.hpp _types.hpp _start.hpp
_types.cpp: _config.hpp _types.hpp _start.hpp