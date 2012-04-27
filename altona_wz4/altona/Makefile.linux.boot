all: base util makeproject_bootstrap asc makeproject

base:
	@make -C main/base -f Makefile.linux.boot

util:	base
	@make -C main/util -f Makefile.linux.boot

makeproject_bootstrap: base util
	@make -C tools/makeproject -f Makefile.linux.boot

asc: 	base util makeproject_bootstrap
	-@tools/makeproject/makeproject.elf
	@make -C tools/asc
	@mkdir -p bin
	-@cp -f tools/asc/release_linux_blank_shell/asc bin/

makeproject:	base util makeproject_bootstrap asc
	@make -C tools/makeproject ASCC=../../tools/asc/release_linux_blank_shell/asc
	-@cp -f tools/makeproject/release_linux_blank_shell/makeproject bin/
