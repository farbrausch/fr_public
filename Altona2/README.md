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

* Add c:\source\altona2\altona2\bin to your path (for shell and VS2012)
* run makeproject2 -r=c:\source\altona2
  * this will create project files for altona and all projects in MyProjects
* compile the project c:\source\altona2\altona2\tools\installer
  * run it
  * this will create the font-cache for altona applications
* try out the following projects:
  * c:\source\altona2\altona2\examples\graphics\cube
  * c:\source\altona2\altona2\examples\gui\window
  * remember to set the build to "debug_dx11", the "???_null" builds will not produce graphics.
* create your own project
  * copy one of the examples to your projects folder
  * modify the *.mp.txt file to fit your project. change the name!
  * run makeproject2 again to create project files according to the *.mp.txt files
  * happy hacking
* be frustrated because there is no documentation.
