#
# Automatic Generated Makefile by Maketool
#

LIBDIR =  -L../../main/base -L../../main/util
INCDIR =  -I./ -I../../main/
TARGET = makeproject
OBJS   = main.o doc.o doc_output.o doc_makefile.o
DEFINES = -DsCONFIG_GUID=\{0xB405548B,\{0xF306,0x45C9,0x856B},\{0x76,0x7B,0xB3,0xF6,0xB6,0xE0\}\} -DsCONFIG_BUILD_DEBUG -DsCONFIG_RENDER_BLANK -DsCONFIG_OPTION_SHELL -DsCONFIG_SYSTEM_LINUX
LIBS    = -lbase -lutil -lpthread -lrt
 
#
# System settings
#

CFLAGS      = -O2 -Wall -Werror -I$(INCDIR)  -fno-strict-aliasing -fshort-wchar -msse
CXXFLAGS    = -O2 -Wall $(DEFINES) $(INCDIR) -fno-exceptions -fno-strict-aliasing -fno-rtti -fshort-wchar -msse

LDFLAGS     = -Wl -mno-crt0 $(LIBDIR) -lm

#
# Rules
#

.SUFFIXES: .c .s .cc .dsm


all: spacer $(TARGET).elf 

relink: remtarget $(TARGET).elf

spacer:
	@echo -e "\n\n\n B E G I N\n";

$(TARGET).elf: $(OBJS)
	g++ -o $@  $(OBJS) $(LDFLAGS) $(LIBS) 

lib$(TARGET).a: $(OBJS)
	ar cr $(OBJS)

.c.o:
	gcc $(CFLAGS) -c $< -o $*.o

.cc.o:
	gcc $(CXXFLAGS) -c $< -o $*.o

.cpp.o:
	gcc $(CXXFLAGS) -c $< -o $*.o

#
# Fake Targets
#

remtarget:
	/bin/rm -f $(TARGET).elf

clean:
	/bin/rm -f *.o *.map *.lst core *.dis *.elf 

#
# end
#
