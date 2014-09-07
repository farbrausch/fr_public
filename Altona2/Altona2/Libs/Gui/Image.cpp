/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "image.hpp"
#include "Altona2/Libs/Util/painter.hpp"

namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sImageWindow::sImageWindow()
{
    Image = 0;
    FImage = 0;
    Painter = 0;
}

sImageWindow::~sImageWindow()
{
    delete Painter;
}

/****************************************************************************/

void sImageWindow::SetImage(sImage *img,int flags,int format)
{
    Image = img;
    FImage = 0;
    Flags = flags;
    Format = format;
    UpdateImage();
}

void sImageWindow::SetImage(sFImage *img,int flags,int format)
{
    FImage = img;
    Image = 0;
    Flags = flags;
    Format = format;
    UpdateImage();
}

void sImageWindow::UpdateImage()
{
    delete Painter;
    Painter = 0;
    if(FImage)
        Painter = new sPainterImage(Adapter(),FImage,Flags,Format);
    else if(Image)
        Painter = new sPainterImage(Adapter(),Image,Flags,Format);
}

/****************************************************************************/

void sImageWindow::OnCalcSize()
{
    if(FImage)
    {
        ReqSizeX = FImage->GetSizeX();
        ReqSizeY = FImage->GetSizeY();
    }
    else if(Image)
    {
        ReqSizeX = Image->GetSizeX();
        ReqSizeY = Image->GetSizeY();
    }
}

void sImageWindow::OnPaint(int layer)
{
    sPainter *pnt = sWindow::Painter();
    sGuiStyle *sty = Style();

    pnt->SetLayer(layer);
    pnt->Rect(sty->Colors[sGC_Back],Client);
    pnt->SetLayer(layer+1);
    if(Painter)
        pnt->Image(Painter,0xffffffff,Client.x0,Client.y0);
}

/****************************************************************************/
}
