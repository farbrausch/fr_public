/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_PLANPAD_VIEW_HPP
#define FILE_PLANPAD_VIEW_HPP

#include "base/types2.hpp"
#include "gui/gui.hpp"
#include "util/scanner.hpp"
#include "wiki/pdf.hpp"

class sFontResource;
class sImageResource;

/****************************************************************************/

struct sLBHtmlState
{
  sU32 TextColor;
  sU32 BackColor;
  sFontResource *Font;
  sBool Active;

  sLBHtmlState();
  ~sLBHtmlState();
  void Begin(sTextBuffer &);
  void End(sTextBuffer &);
  void Init(sU32 tc,sU32 bc,sFontResource *f);
  void Change(sU32 tc,sU32 bc,sFontResource *f,sTextBuffer &tb);
};

struct sLBPageInfo
{
  sLBPageInfo();

  sInt SizeX;
  sInt SizeY;
  sRect Border;
  sInt GrayBorder;

  sPoolString Header;
  sPoolString Footer;
  sPoolString Title;
  sBool HeaderRuler;
  sBool FooterRuler;
};

class sLBPdfState
{
  sInt Page;
  sRect Client;
  sF32 DX;
  sF32 DY;
  sF32 SX;
  sF32 SY;
  sFont2D *Font;
  sInt FontId;
  sF32 FontScale;
  sU32 FontBackColor;
  sU32 FontTextColor;
public:
  sLBPdfState();
  ~sLBPdfState();

  sPDF *pdf;
  sF32 Zoom;
  
  void CalcMatrix(sInt page,const sLBPageInfo &pageinfo,const sRect &r);
  sBool Hit(const sRect &r);
  void Rect(const sRect &r,sU32 color);
  void RectFrame(const sRect &r,sU32 color,sInt w);
  void Rect(sInt x0,sInt y0,sInt x1,sInt y1,sU32 color);
  void SetPrint(sFontResource *,sU32 tc=0,sU32 bc=~0);
  void Print(const sRect &r,sInt x,sInt y,const sChar *text,sInt len=-1);
  void Print(sInt align,const sRect &r,const sChar *text,sInt len=-1);
  sInt GetPage() const { return Page; }
};

class sLayoutBox
{
public:
  sLayoutBox *Childs;
  sLayoutBox **Link;
  sLayoutBox *Next;
  sInt Kind;
  sInt MinX;
  sInt OptX;
  sInt TopY,BotY;
  sInt GlueWeight;
  sRect Client;
  sArray<sRect> Clients;        // if it spans over multiple pages, we have multiple client rects! this overrides Client
  sInt Baseline;
  sBool FullWidth;
  sBool PageBreak;                // this will cause a page break
  sBool CanBreak;                 // this can handle pagebreaks
  sPoolString LeftAction;
  sPoolString RightAction;
  sPoolString MiddleAction;
  sInt Temp;
  sInt Page;                      // on which page was this printed?
  sInt MousePointer;
  const sChar *CursorStart;
  const sChar *CursorEnd;

  sLayoutBox();
  virtual ~sLayoutBox();
  void AddChild(sLayoutBox *);
  sInt GetChildCount();
  virtual void MakeHtml(sTextBuffer &,sLBHtmlState *);
  virtual void MakePDF(sLBPdfState *);

  virtual void PrepX();
  virtual void PrepY();
  virtual void CalcX(sInt x0,sInt x1);
  virtual void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
//  virtual void CalcY(sInt y0,sInt y1,sInt baseline);
//  virtual sBool CalcY(sInt &y,sInt &page,class sLBPage *);
  virtual void Paint(class sLayoutWindow *);
  virtual const sChar *Click(sInt x,sInt y);

#undef new
  void *operator new(sCONFIG_SIZET sz);
  void *operator new(sCONFIG_SIZET sz,const char *file,int line);
  void operator delete(void *ptr);
#define new sDEFINE_NEW
};

enum sLayoutBoxKind
{
  sLBK_UNKNOWN = 0,
  sLBK_WORD,
  sLBK_GLUE,
  sLBK_TABLE2, 
};

/****************************************************************************/

class sLayoutWindow : public sWireClientWindow
{
  sBool LayoutFlag;
  void Paint(sLayoutBox *box);
  sLayoutBox *ScrollToTextR(sLayoutBox *b,const sChar *text);
  const sChar *ScrollToAfterLayout;
public:
  sLayoutWindow();
  ~sLayoutWindow();
  void InitWire(const sChar *name);
  void Layout() { LayoutFlag = 1; }
  void DoLayout();
//  void Parse(const sChar *,const sChar *);
  void Clear();

  void OnCalcSize();
  void OnPaint2D();
  sBool OnKey(sU32 key);
  void OnDrag(const sWindowDrag &dd);
  sLayoutBox *FindBox(sInt x,sInt y,sLayoutBox *root);

  class sLBTopBox *Root;
  sBool DebugBoxes;
  sBool PageMode;
  sBool Landscape;
  const sChar *CursorChar;
  sWindow *ActivateOnKeyWindow;
  sBool CursorFlash;

//  void DragClick(const sWindowDrag &dd);
  void ScrollToText(const sChar *text);
  void MakeHtml(sTextBuffer &);
};

/***************************************************************************/

class sLBText : public sLayoutBox
{
public:
  sLBText(const sChar *,sFontResource *font=0);
  ~sLBText();
  void PrepX();
  void PrepY();
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);

  sU32 TextColor;
  sU32 BackColor;
  const sChar *Text;
  sFontResource *Font;
  sInt PrintFlags;
};

class sLBWord : public sLayoutBox
{
public:
  sLBWord(const sChar *,sInt len,sFontResource *font=0);
  ~sLBWord();
  void PrepX();
  void PrepY();
  void Paint(sLayoutWindow *);
  const sChar *Click(sInt x,sInt y);
  void MakeHtml(sTextBuffer &,sLBHtmlState *);
  void MakePDF(sLBPdfState *pdf);


  sU32 TextColor;
  sU32 BackColor;
  const sChar *Text;
  sPoolString EscapedTextBuffer;
  const sChar *EscapedText;
  sInt EscapedLength;
  sInt Length;
  const sChar *CursorTextStart;
  const sChar *CursorTextEnd;
  sFontResource *Font;
  sBool WrongSpelling;
};

class sLBGlue : public sLBWord
{
public:
  sLBGlue(const sChar *,sInt len,sFontResource *font=0);

  void Paint(sLayoutWindow *);
  const sChar *Click(sInt x,sInt y);
  void PrepX();
  void MakeHtml(sTextBuffer &,sLBHtmlState *);
  void MakePDF(sLBPdfState *pdf);

  sBool Border;                 // this box has been layouted to the border
  sU32 BorderColor;             // when this box is at the border, use this color
};


class sLBParagraph : public sLayoutBox
{
  struct Space
  {
    sLBWord *Word;
    const sChar *Start;
    const sChar *End;
  };
  sArray<Space> Spaces;
  struct Line
  {
    sLayoutBox *First;          // first box in line
    sLayoutBox *Last;           // last box in line, inclusive
    sLayoutBox *LastGlue;       // glue after this line, up to the next line
  };
  sArray<Line> Lines;
public:
  sLBParagraph(sFontResource *font=0);
  sLBParagraph(const sChar *text,sFontResource *font=0);
  sLBParagraph(const sChar *text,sFontResource *font,sInt count);
  ~sLBParagraph();
  void PrepX();
  void PrepY();
  void CalcX(sInt x0,sInt x1);
  void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
//  sBool CalcY(sInt &yp,sInt &page,sLBPage *box);
  void Paint(sLayoutWindow *);
  const sChar *Click(sInt x,sInt y);

  void AddText(const sChar *text,sFontResource *font=0,sU32 textcolor=sGC_TEXT,sInt count=-1);
  void MakeHtml(sTextBuffer &,sLBHtmlState *);
  void MakePDF(sLBPdfState *pdf);

  sU32 BackColor;
  sInt PrintFlags;      // sF2P_???
  sInt SpaceWidth;
};


class sLBImage : public sLayoutBox
{
public:
  sLBImage(const sChar *name);
  ~sLBImage();
  void PrepX();
//  void CalcX(sInt x0,sInt x1);
  void PrepY();
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);

  class sImageResource *Image;
  sU32 BackColor;
  sInt Align;         // 1 2 4 8 16 |  left right top bottom center
  sInt ScaledX;
  sInt ScaledY;
  sPoolString ImageName;
};

class sLBPageNumber : public sLayoutBox
{
  sLayoutBox *Ref;
  sFontResource *Font;
public:
  sLBPageNumber(sLayoutBox *ref,sFontResource *font);
  ~sLBPageNumber();
  void PrepX();
  void PrepY();
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);
  void MakePDF(sLBPdfState *pdf);

  sU32 TextColor;
  sU32 BackColor;
};

class sLBSpacer : public sLayoutBox
{
public:
  sLBSpacer(sInt x=1,sInt y=1);
  sLBSpacer(sInt x,sInt y,sU32 backcolor);
  ~sLBSpacer();
  void PrepX();
  void PrepY();
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);

  sInt SpaceX;
  sInt SpaceY;
  sU32 BackColor;
};

/***************************************************************************/

enum sLBTableMode
{
  sLTF_FULL         = 0x00,   // try best to find a good choice
  sLTF_MIN          = 0x01,   // minimal size
  sLTF_OPT          = 0x02,   // optimal size, falling back to min when required
};

class sLBTable2 : public sLayoutBox
{
  sInt UsedX0;
  sInt UsedX1;
  sArray<sRect> Used;

  struct Column
  {
    sInt Weight;
    sInt Min;
    sInt Opt;
    sInt Size;
    sInt NoFit;
  };
  struct Row
  {
    sInt Page;
    sInt y0,y1;
  };
  sArray<Column> Cols;
  sArray<Row> Rows;
public:
  sLBTable2(sInt columns,sInt modex=sLTF_FULL);
  const sChar *Click(sInt x,sInt y);
  void PrepX();
  void PrepY();
  void CalcX(sInt x0,sInt x1);
  void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);
  void MakePDF(sLBPdfState *pdf);

  void SetWeight(sInt i,sInt w) { Cols[i].Weight = w; }

  sInt Columns;
  sInt CountX;
  sInt CountY;
  sInt ModeX;
  sInt InnerBorder;
  sInt OuterBorder;
  sU32 BorderColor;
  sU32 BackColor;
  const sChar *ClickString;
};


class sLBBorder : public sLayoutBox
{
public:
  sLBBorder(sInt extend=1,sU32 col=sGC_DRAW,sLayoutBox *child=0);
  void PrepX();
  void PrepY();
  void CalcX(sInt x0,sInt x1);
  void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);
  void MakePDF(sLBPdfState *pdf);

  sRect Extend;
  sU32 Color;
};

class sLBTopBox : public sLayoutBox
{
public:
  sInt ReqSizeX;
  sInt ReqSizeY;
  sLBPageInfo Page;
  virtual void SetLandscape(sBool landscape) {}
};

class sLBContinuous : public sLBTopBox
{
public:
  sLBContinuous();
  void PrepX();
  void PrepY();
  void CalcX(sInt x0,sInt x1);
  void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);
  void MakePDF(sLBPdfState *pdf);

  sU32 BackColor;

};

class sLBPage : public sLBTopBox
{
  sFontResource *TitleFont;
  sFontResource *HeaderFont;
  sFontResource *TocFont;
  sRect PageRect;

  void HeaderSplit(const sChar *str,sString<1024> *out,sInt page);
  void HeaderRect(sRect &r0,sRect &r1,sInt page);
public:
  sLBPage();
  ~sLBPage();
  void SetLandscape(sBool landscape);
  void PrepX();
  void PrepY();
  void CalcX(sInt x0,sInt x1);
  void CalcY(sInt y0,sInt y1,sInt baseline,sInt ybreak,class sLBPage *);
  void Paint(sLayoutWindow *);
  void MakeHtml(sTextBuffer &tb,sLBHtmlState *s);
  void MakePDF(sLBPdfState *pdf);
  void InnerPageRect(sInt n,sRect &r);
  void OuterPageRect(sInt n,sRect &r);

  void NextPage(sInt &ycursor,sInt &ybreak);

  sInt PageNum;
  sInt YCursor;
  sU32 BackColor;
  sInt PageCount;
};

/****************************************************************************/

#endif // FILE_PLANPAD_VIEW_HPP

