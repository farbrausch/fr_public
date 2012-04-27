/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/
/***                                                                      ***/
/***   This example compares different algorithms to pack dxt textures    ***/
/***                                                                      ***/
/***   operation:                                                         ***/
/***   - start with the name of a directory                               ***/
/***   - directory gets scanned for *.pic and *.bmp files                 ***/
/***   - images are loaded and displayed                                  ***/
/***   - different algos are used to calculate the DXT's                  ***/
/***   - result quality gets measured                                     ***/
/***   - speed gets displayed                                             ***/
/***                                                                      ***/
/****************************************************************************/

#include "main.hpp"
#include "gui.hpp"

/****************************************************************************/

void sMain()
{
  sInit(sISF_2D|sISF_3D,800,600);
  sInitGui();
  sGui->AddBackWindow(new MainWindow);
}

/****************************************************************************/

