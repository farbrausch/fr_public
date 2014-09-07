root = ~/source/altona2
dest = /home/chaos/bin
cflags = -lrt -DsDEFINE_LINUX -DsDEFINE_SHELL -DsDEFINE_MEMDEBUG -I $(root) -std=c++0x -g
objdir = Bootstrap

objlib  = $(root)/Altona2/Libs/Base/$(objdir)/Containers.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/Graphics.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/GraphicsNull.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/Machine.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/String.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/System.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/SystemLinux.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/SystemPosix.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/Types.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/Math.o
objlib += $(root)/Altona2/Libs/Util/$(objdir)/Scanner.o
objlib += $(root)/Altona2/Libs/Base/$(objdir)/Serialize.o

objincbin = $(root)/Altona2/Tools/Incbin/Bootstrap/Main.o

objasc  = $(root)/Altona2/Tools/Asc/Bootstrap/Asc.o
objasc += $(root)/Altona2/Tools/Asc/Bootstrap/Doc.o
objasc += $(root)/Altona2/Tools/Asc/Bootstrap/Hlsl.o
objasc += $(root)/Altona2/Tools/Asc/Bootstrap/Glsl.o
objasc += $(root)/Altona2/Tools/Asc/Bootstrap/Main.o

objmp  = $(root)/Altona2/Tools/MakeProject/Bootstrap/Doc.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocMake.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocVs2008.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocVs2010.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocVs2012.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocXcode4.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/DocNdk.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/Main.o
objmp += $(root)/Altona2/Tools/MakeProject/Bootstrap/Incbin.o

all : prepare incbin asc makeproject
clean :
	rm -f $(root)/Altona2/Libs/Base/Bootstrap/*.o
	rm -f $(root)/Altona2/Libs/Util/Bootstrap/*.o
	rm -f $(root)/Altona2/Tools/Incbin/Bootstrap/*.o
	rm -f $(root)/Altona2/Tools/Asc/Bootstrap/*.o
	rm -f $(root)/Altona2/Tools/MakeProject/Bootstrap/*.o

prepare :
	mkdir -p $(root)/Altona2/Libs/Base/Bootstrap
	mkdir -p $(root)/Altona2/Libs/Util/Bootstrap
	mkdir -p $(root)/Altona2/Tools/Incbin/Bootstrap
	mkdir -p $(root)/Altona2/Tools/Asc/Bootstrap
	mkdir -p $(root)/Altona2/Tools/MakeProject/Bootstrap
	
incbin : $(objlib) $(objincbin)
	g++ -o$(dest)/incbin $(objlib) $(objincbin) $(cflags)
asc : $(objlib) $(objasc)
	g++ -o$(dest)/asc $(objlib) $(objasc) $(cflags)
makeproject : $(objlib) $(objmp)
	g++ -o$(dest)/makeproject $(objlib) $(objmp) $(cflags)

### rules

$(root)/Altona2/Libs/Base/$(objdir)/%.o : $(root)/Altona2/Libs/Base/%.cpp
	g++ $(cflags) -c $< -o $@
$(root)/Altona2/Libs/Util/$(objdir)/%.o : $(root)/Altona2/Libs/Util/%.cpp
	g++ $(cflags) -c $< -o $@
$(root)/Altona2/Tools/Incbin/$(objdir)/%.o : $(root)/Altona2/Tools/Incbin/%.cpp
	g++ $(cflags) -c $< -o $@
$(root)/Altona2/Tools/Asc/$(objdir)/%.o : $(root)/Altona2/Tools/Asc/%.cpp
	g++ $(cflags) -c $< -o $@
$(root)/Altona2/Tools/MakeProject/$(objdir)/%.o : $(root)/Altona2/Tools/MakeProject/%.cpp
	g++ $(cflags) -c $< -o $@

$(root)/Altona2/Tools/MakeProject/$(objdir)/%.o : $(root)/Altona2/Tools/MakeProject/%.incbin
	( cd $(root)/Altona2/Tools/MakeProject ; $(dest)/incbin -i=$< -o=$@ -x64 )

### end

