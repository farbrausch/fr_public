How to use this framework with Visual Studio 2012?

Assuming the following directory structure

c:\sources\altona2
  c:\source\altona2\altona2
    c:\source\altona2\altona2\lib
      c:\source\altona2\altona2\base
      c:\source\altona2\altona2\gui
      c:\source\altona2\altona2\util
      ..
    c:\source\altona2\altona2\tools
    c:\source\altona2\altona2\examples
    c:\source\altona2\altona2\bin
    ...
  c:\source\altona2\MyProjects

* Add c:\source\altona2\altona2\bin\winx86 to your path (for shell and VS2012)
* run makeproject2 -r=c:\source\altona2
  * this will create project files for altona and all projects in MyProjects
  * you may need to restart visual studio and your command-line for this to work
* you will need yasm.exe in your path (The Yasm Modular Assembler Project)
  * download it from http://yasm.tortall.net/Download.html
  * you only need the exe, renamed to "yasm.exe", in your path
  * you may just copy it into the altona2\bin\winx86 folder
  * yasm is not used as a real assembler, but as a simple way to convert binaries
    to object files without printing hex-dumps to text files.
* try out the following projects:
  * c:\source\altona2\altona2\examples\graphics\cube
  * c:\source\altona2\altona2\examples\gui\window
  * confirm the build is set to "debug_dx11", the "???_null" builds will not produce graphics.
* compile the project c:\source\altona2\altona2\tools\installer
  * run it
  * this will create the font-cache for altona applications
* create your own project
  * copy one of the examples to your projects folder
  * modify the *.mp.txt file to fit your project. change the name!
  * run makeproject2 again to create project files according to the *.mp.txt files
  * happy hacking
* be frustrated because there is no documentation.
