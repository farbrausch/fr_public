// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "appprefs.hpp"

/****************************************************************************/

#define sGC_NONE        0         // unused palette entry
#define sGC_BACK        1         // client area clear color
#define sGC_TEXT        2         // normal text color
#define sGC_DRAW        3         // color for lines etc.
#define sGC_HIGH        4         // emboss box high color (outer)
#define sGC_LOW         5         // emboss box low color (outer)
#define sGC_HIGH2       6         // emboss box high color (inner)
#define sGC_LOW2        7         // emboss box low color (inner)
#define sGC_BUTTON      8         // button face
#define sGC_BARBACK     9         // scrollbar background
#define sGC_SELECT      10        // selected item text color
#define sGC_SELBACK     11        // selected item background color
#define sGC_MAX         12        // max color

sPrefsApp::sPrefsApp()
{
  sGridFrame *Form;
  sInt i;
  static sChar *colorname[sGC_MAX] =
  {
    "Desktop",
    "Background",
    "Text",
    "Drawing",
    "Outer High",
    "Outer Low",
    "Inner High",
    "Inner Low",
    "Button Face",
    "Scrollbar",
    "Selected Text",
    "Selected Back",
  };


  Form = new sGridFrame;
  Form->SetGrid(3,sGC_MAX,0,sPainter->GetHeight(sGui->PropFont)+8);
  for(i=0;i<sGC_MAX;i++)
  {
    Form->AddCon(0,i,3,1)->EditRGB(1,Colors+i*3,colorname[i]);
    Colors[i*3+0] = ((sGui->Palette[i]>>16)&255)<<8;
    Colors[i*3+1] = ((sGui->Palette[i]>> 8)&255)<<8;
    Colors[i*3+2] = ((sGui->Palette[i]    )&255)<<8;
  }
  Form->AddScrolling(0,1);
  AddChild(Form);
}

sBool sPrefsApp::OnCommand(sU32 cmd)
{
  sInt i;
  switch(cmd)
  {
  case 1:
    for(i=0;i<sGC_MAX;i++)
    {
      sGui->Palette[i] = 0xff000000 
                       | (((Colors[i*3+0]>>8)&255)<<16)
                       | (((Colors[i*3+1]>>8)&255)<< 8)
                       | (((Colors[i*3+2]>>8)&255)    );
    }
    return sTRUE;
  default:
    return sFALSE;
  }
}

void sPrefsApp::OnKey(sU32 key)
{
  if(key==sKEY_APPCLOSE)
    Parent->RemChild(this);
}

/****************************************************************************/
