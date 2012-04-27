/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "pdf.hpp"
#include "base/system.hpp"
#include "base/windows.hpp"

/****************************************************************************/
/****************************************************************************/

void sPDF::Print(const sChar *text)
{
  sInt len = sGetStringLen(text);

  sU8 *dest = Data.AddMany(len);
  for(sInt i=0;i<len;i++)
    dest[i] = text[i];
}

sInt sPDF::AddObject()
{
  sInt id = Objects.GetCount();
  *Objects.AddMany(1) = Data.GetCount();
  PrintF(L"%d 0 obj\n",id);
  return id;
}

void sPDF::Stream(const sChar *text)
{
  Stream(text,sGetStringLen(text));
}

void sPDF::Stream(const sChar *text,sInt len)
{
  sU8 *dest = StreamData.AddMany(len);
  for(sInt i=0;i<len;i++)
  {
    *dest++ = text[i];
//    *dest++ = text[i];
  }
}

void sPDF::StreamString(const sChar *text,sInt len)
{
  if(len==-1)
    len = sGetStringLen(text);

  Stream(L"(");
  sInt escapes = 0;
  for(sInt i=0;i<len;i++)
  {
    if(text[i]=='\\' || text[i]=='(' || text[i]==')')
      escapes++;
  }

  sU8 *dest = StreamData.AddMany((len+escapes)*1);
  for(sInt i=0;i<len;i++)
  {
    if(text[i]=='\\' || text[i]=='(' || text[i]==')')
    {
      *dest++ = '\\';
    }
    if(text[i]==0x2022)
      *dest++ = 0225;      // bullet character, octal!
    else
      *dest++ = text[i];
  }
  Stream(L")");
}

sInt sPDF::AddStream()
{
  sInt id = AddObject();
  sInt len = StreamData.GetCount();
  sU8 *src = StreamData.GetData();

  PrintF(L"<<\n/Length %d\n>>\n",len);
  Print(L"stream\n");
  sU8 *dest = Data.AddMany(len);
  sCopyMem(dest,src,len);
  Print(L"\nendstream endobj\n");


  StreamData.Clear();

  return id;
}

void sPDF::PageFont(sInt font)
{
  sInt *ip;
  sFORALL(PageFonts,ip)
    if(*ip==font)
      return;
  *PageFonts.AddMany(1) = font;
}

/****************************************************************************/
/****************************************************************************/

sPDF::sPDF()
{
  Page = 0;
}

sPDF::~sPDF()
{
}

/****************************************************************************/

void sPDF::BeginDoc(sF32 xs,sInt ys)
{
  Data.Clear();
  StreamData.Clear();
  Objects.Clear();
  Pages.Clear();
  PageFonts.Clear();
  *Objects.AddMany(1) = 0;    // dummy
  *Objects.AddMany(1) = 0;    // pages
  *Objects.AddMany(1) = 0;    // catalog
  Page = -1;
  SizeX = xs;
  SizeY = ys;

  Print(L"%PDF-1.1\n");
  Print(L"%íì¦\"\n");
}

sInt sPDF::RegisterFont(sInt flags,sFont2D *font)
{
  const sChar *name = L"Helvetica";
  switch(flags)
  {
  case sPDF_Regular   |sPDF_Times:     name = L"Times-Roman"; break;
  case sPDF_Bold      |sPDF_Times:     name = L"Times-Bold"; break;
  case sPDF_Italic    |sPDF_Times:     name = L"Times-Italic"; break;
  case sPDF_BoldItalic|sPDF_Times:     name = L"Times-BoldItalic"; break;
  case sPDF_Regular   |sPDF_Courier:   name = L"Courier"; break;
  case sPDF_Bold      |sPDF_Courier:   name = L"Courier-Bold"; break;
  case sPDF_Italic    |sPDF_Courier:   name = L"Courier-Oblique"; break;
  case sPDF_BoldItalic|sPDF_Courier:   name = L"Courier-BoldOblique"; break;
  case sPDF_Regular   |sPDF_Helvetica: name = L"Helvetica"; break;
  case sPDF_Bold      |sPDF_Helvetica: name = L"Helvetica-Bold"; break;
  case sPDF_Italic    |sPDF_Helvetica: name = L"Helvetica-Oblique"; break;
  case sPDF_BoldItalic|sPDF_Helvetica: name = L"Helvetica-BoldOblique"; break;
  case sPDF_Symbol:                    name = L"Symbol"; break;
  case sPDF_Dingbats:                  name = L"ZapfDingbats"; break;
  }

  sInt wid = AddObject();
  sChar string[2];
  string[1] = 0;
  sInt height = font->GetCharHeight();
  Print(L"[");
  for(sInt i=32;i<=255;i++)
  {
    string[0] = i;
    if((i & 15) ==0) Print(L"\n");
    PrintF(L" %d",font->GetWidth(string,1)*1000/height);
  }
  Print(L"\n]\n");
  Print(L"endobj\n");

  sInt id = AddObject();
  Print(L"<<\n");
  Print(L"/Type /Font\n");
  Print(L"/Subtype /Type1\n");
  PrintF(L"/Name /F%d\n",id);
  PrintF(L"/BaseFont /%s\n",name);
  PrintF(L"/Encoding /WinAnsiEncoding\n");
  PrintF(L"/FirstChar 32\n");
  PrintF(L"/LastChar 255\n");
  PrintF(L"/Widths %d 0 R\n",wid);
  Print(L">>\n");
  Print(L"endobj\n");
  return id;
}

void sPDF::EndDoc(const sChar *filename)
{
  sInt *ip;
  Objects[1] = Data.GetCount();
  Print(L"1 0 obj\n");
  Print(L"<<\n");
  Print(L"/Type /Pages\n");
  Print(L"/Kids [");
  sFORALL(Pages,ip)
    PrintF(L"%d 0 R ",*ip);
  Print(L"]\n");
  PrintF(L"/Count %d\n",Pages.GetCount());
  PrintF(L"/MediaBox [0 0 %d %d]\n",SizeX,SizeY);
  Print(L">>\n");
  Print(L"endobj\n");

  Objects[2] = Data.GetCount();
  Print(L"2 0 obj\n");
  Print(L"<<\n");
  Print(L"/Type /Catalog\n");
  Print(L"/Pages 1 0 R\n");
  Print(L">>\n");
  Print(L"endobj\n");

  sInt xref = Data.GetCount();
  Print(L"xref\n");
  PrintF(L"0 %d\n",Objects.GetCount());
  PrintF(L"%010d %05d f \n",0,0xffff);
  for(sInt i=1;i<Objects.GetCount();i++)
    PrintF(L"%010d %05d n \n",Objects[i],0);
  Print(L"trailer\n");
  Print(L"<<\n");
  PrintF(L"/Size %d\n",Objects.GetCount());
  Print(L"/Root 2 0 R\n");
  Print(L">>\n");
  Print(L"startxref\n");
  PrintF(L"%d\n",xref);
  Print(L"%%EOF\n");

  sSaveFile(filename,Data.GetData(),Data.GetCount());
}

/****************************************************************************/

sInt sPDF::BeginPage()
{
  Page++;

  InvalidateCache();
  PageFonts.Clear();

  Stream(L"2 J 0 j 1 w\n");

  return Page;
}

void sPDF::EndPage()
{
  sInt *ip;
  sInt content = AddStream();

  sInt res = AddObject();
  Print(L"<<\n");
  Print(L"/ProcSet [/PDF /Text ]\n");
  Print(L"/Font <<\n");
  sFORALL(PageFonts,ip)
    PrintF(L"/F%d %d 0 R\n",*ip,*ip);
  Print(L">>\n");
  Print(L">>\n");
  Print(L"endobj\n");
   
  sInt page = AddObject();
  Print(L"<<\n");
  Print(L"/Type /Page\n");
  Print(L"/Parent 1 0 R\n");
  PrintF(L"/Resources %d 0 R\n",res);
  PrintF(L"/Contents %d 0 R\n",content);
  Print(L">>\n");
  Print(L"endobj\n");

  *Pages.AddMany(1) = page;
}

void sPDF::Text(sInt font,sF32 s,sF32 x,sF32 y,sU32 col,const sChar *text,sInt len)
{
  if(len==-1)
    len = sGetStringLen(text);
  if(len>0)
  {
    PageFont(font);

    Stream(L"BT ");
    _Tf(font,s);
    _rg(col);
    StreamF(L"%f %f Td ",x,y);
    StreamString(text,len);
    Stream(L"Tj ");
    Stream(L"ET\n");
  }
}

void sPDF::Text(sInt font,sF32 m[6],sU32 col,const sChar *text,sInt len)
{
  if(len==-1)
    len = sGetStringLen(text);
  if(len>0)
  {
    PageFont(font);

    Stream(L"BT ");
    _Tf(font,1);
    _rg(col);
    StreamF(L"%f %f %f %f %f %f Tm\n",m[0],m[1],m[2],m[3],m[4],m[5]);
    Stream(L"(");
    StreamString(text,len);
    Stream(L")Tj\n");
    Stream(L"ET\n");
  }
}

void sPDF::Rect(const sFRect &r,sU32 col)
{
  _rg(col);
  StreamF(L"%f %f %f %f re f\n",r.x0,sMin(r.y0,r.y1),r.SizeX(),sAbs(r.SizeY()));
}

void sPDF::RectFrame(const sFRect &r,sU32 col,sF32 w)
{
  sF32 h = w/2;
  _rg(col);
  StreamF(L"%f w %f %f %f %f re s\n",w,r.x0+h,sMin(r.y0,r.y1)+h,r.SizeX()-h,sAbs(r.SizeY())-h);
}

void sPDF::InvalidateCache()
{
  cache_Tf_font = -1; 
  cache_Tf_scale = -1;
  cache_rg = ~0;
}

void sPDF::_Tf(sInt font,sF32 scale)
{
  if(cache_Tf_font!=font || cache_Tf_scale!=scale)
  {
    StreamF(L"/F%d %f Tf ",font,scale);
    cache_Tf_font = font;
    cache_Tf_scale = scale;
  }
}
void sPDF::_rg(sU32 col)
{
  col = col & 0xffffff;
  if(cache_rg!=col)
  {
    StreamF(L"%f %f %f rg "
      ,((col>>16)&255)/255.0f
      ,((col>>8)&255)/255.0f
      ,(col&255)/255.0f
    );
    cache_rg=col;
  }
}

/****************************************************************************/

