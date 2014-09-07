/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_BASE_SYSTEMOSX_HPP
#define FILE_ALTONA2_LIBS_BASE_SYSTEMOSX_HPP

#include "Altona2/Libs/Base/Base.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Basic function set to communicate with Altona2                     ***/
/***   Only to be used if Altona2 is not been compiled as standalone      ***/
/***                                                                      ***/
/****************************************************************************/

namespace Altona2 {
namespace Setup {

/****************************************************************************/
    
enum sKeyTypes
{
    sKT_Up		=	1,
    sKT_Down	=	2,
    sKT_Flags   = 	3
};

/****************************************************************************/

struct sPrivateTouch
{
  int count;
  int x;
  int y;
  unsigned int id_;
};

/****************************************************************************/
    
bool sInitSystem();
void sExitSystem();
sApp *sGetApp();
void sSetApp(sApp *app);
void sStartApp();
void sPauseApp();
void sHandleKey(void *event, int type);		//event must be from type NSEvent
void sSetScreenSize(int sizex, int sizey);
void sSetGLFrameBuffer(unsigned int glname);
void sOnPaintApp();
void sOnFrameApp();
bool sCheckRetina();
    
/****************************************************************************/

} // namespace Setup
} // namespace Altona2

#endif
