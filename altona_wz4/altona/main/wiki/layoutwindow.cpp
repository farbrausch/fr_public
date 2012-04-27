/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

//#include "main.hpp"
#include "layoutwindow.hpp"
#include "resources.hpp"

#define FORCHILDS(box,child) for(sInt _i=0;child=((_i<box->ChildCount)?box->Childs[_i]:0);_i++)

const sF32 PDFZoom = 0.75f;

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

sU32 MakeColor(sU32 col,sInt slot)
{
  if(col>=sGC_MAX)
  {
    sSetColor2D(sGC_MAX+slot,col);
    return sGC_MAX+slot;
  }
  else
  {
    return col;
  }
}

sU32 MakeColorPDF(sU32 col)
{
  if(col<sGC_MAX)
    col = sGetColor2D(col);
  return col & 0xffffff;
}



sU32 MakeColorPDFBW(sU32 col)
{
  if(col<sGC_MAX)
    col = sGetColor2D(col);
  return (col & 0xff) > 0x80 ? 0xffffff : 0x000000;
}


/****************************************************************************/
/***                                                                      ***/
/***   red tape                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sLayoutWindow::sLayoutWindow()
{
  Root = 0;
  LayoutFlag = 1;
  DebugBoxes = 0;
  CursorChar = 0;
  ActivateOnKeyWindow = 0;
  PageMode = 0;
  ScrollToAfterLayout = 0;
  CursorFlash = 1;
  Flags |= sWF_HOVER;
  Landscape = 0;
}

sLayoutWindow::~sLayoutWindow()
{
  delete Root;
}

void sLayoutWindow::InitWire(const sChar *name)
{
  sWireClientWindow::InitWire(name);

  sWire->AddChoice(name,L"DebugBoxes",sMessage(this,&sLayoutWindow::Update),&this->DebugBoxes,L"-|Debug Boxes");
//  sWire->AddChoice(name,L"PageMode",sMessage(this,&sLayoutWindow::Layout),&this->PageMode,L"-|Page Mode");
//  sWire->AddDrag(name,L"Link",sMessage(this,&sLayoutWindow::DragLink));
}

/****************************************************************************/

void sLayoutWindow::OnCalcSize()
{
  Layout();
}

void sLayoutWindow::OnPaint2D()
{
  if(Root)
  {
    sClipPush();
    if(LayoutFlag)
      DoLayout();
    Paint(Root);
    sClipPop();
  }
  else 
    sRect2D(Client,sGC_RED);
}

sBool sLayoutWindow::OnKey(sU32 key)
{
  if(sWire->HandleKey(this,key))
    return 1;
  if(ActivateOnKeyWindow)
  {
    sGui->SetFocus(ActivateOnKeyWindow);
    return ActivateOnKeyWindow->OnKey(key);
  }
  return 0;
}

void sLayoutWindow::OnDrag(const sWindowDrag &dd)
{
  if(dd.Mode == sDD_HOVER)
  {
    sLayoutBox *box = FindBox(dd.MouseX,dd.MouseY,Root);
    MousePointer = box ? box->MousePointer : sMP_ARROW;
  }

  sWireClientWindow::OnDrag(dd);
}

void sLayoutWindow::Paint(sLayoutBox *box)
{
  sRect *rp;
  if(box->Client.IsInside(Inner))
  {
    sLayoutBox *n;
    n = box->Childs;
    while(n)
    { 
      Paint(n);
      n = n->Next;
    }

    box->Paint(this);

    if(DebugBoxes)
    {
      if(box->Clients.GetCount()>0)
        sFORALL(box->Clients,rp)
          sRectFrame2D(*rp,sGC_RED);
      else
        sRectFrame2D(box->Client,sGC_RED);
    }

    if(CursorChar >= box->CursorStart && CursorChar<box->CursorEnd && CursorFlash)
    {
      sRect r(box->Client.x0,box->Client.y0,box->Client.x0+2,box->Client.y1);
      sRect2D(r,sGC_DRAW); 
    }

    if(box->Clients.GetCount()>0)
      sFORALL(box->Clients,rp)
        sClipExclude(*rp);
    else
      sClipExclude(box->Client);
  }
}

sLayoutBox *sLayoutWindow::FindBox(sInt x,sInt y,sLayoutBox *root)
{
  if(root->Client.Hit(x,y)==0) return 0;

  sLayoutBox *c = root->Childs;
  while(c)
  {
    sLayoutBox *r = FindBox(x,y,c);
    if(r)
      return r;
    c = c->Next;
  }
  return root;
}


void sLayoutWindow::ScrollToText(const sChar *text)
{
  if(LayoutFlag)
  {
    ScrollToAfterLayout = text;
  }
  else
  {
    sLayoutBox *b = ScrollToTextR(Root,text);
    if(b)
    {
//      sDPrintF(L"scroll to %08x %d %d %d %d\n",sPtr(b),b->Client.x0,b->Client.y0,b->Client.x1,b->Client.y1);
      ScrollTo(b->Client,1);
    }
    ScrollToAfterLayout = 0;
  }
}

sLayoutBox *sLayoutWindow::ScrollToTextR(sLayoutBox *b,const sChar *text)
{
  if(b->Kind==sLBK_GLUE || b->Kind==sLBK_WORD)
  {
    sLBWord *word = (sLBWord *) b;
    if(text>=word->Text && text<word->Text+word->Length)
      return b;
    if(text>=word->CursorTextStart && text<word->CursorTextEnd+word->Length)
      return b;
  }
  sLayoutBox *c = b->Childs;
  while(c)
  {
    sLayoutBox *r = ScrollToTextR(c,text);
    if(r)
      return r;
    c = c->Next;
  }
  return 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   Html                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void sLayoutWindow::MakeHtml(sTextBuffer &tb)
{
  sLBHtmlState s;
  tb.Clear();
  tb.Print(L"<html><body>\n");
  Root->MakeHtml(tb,&s);
  s.End(tb);
  tb.Print(L"\n</body></html>\n");
}

/****************************************************************************/

static sU32 getcolor(sU32 pen)
{
  if(pen>sGC_MAX) return pen&0xffffff;
  else return sGetColor2D(pen)&0xffffff;
}

sLBHtmlState::sLBHtmlState()
{
  TextColor = sGC_BLACK;
  BackColor = sGC_WHITE;

  Font = 0;
  Active = 0;
}

sLBHtmlState::~sLBHtmlState()
{
}

void sLBHtmlState::Begin(sTextBuffer &tb)
{
  if(!Active)
  {
    sInt size = sRFS_NORMAL;
    const sChar *face = L"Arial";
    if(Font)
    {
      size = Font->LogSize;
      face = Font->Name;
    }
    tb.PrintF(L"<font color=\"#%06x\" size=\"%d\" face=\"%s\" style=\"background-color:#%06x\"",getcolor(TextColor),size+1,face,getcolor(BackColor));
    if(Font->Style & sF2C_SYMBOLS)
      tb.PrintF(L"style=\"font-family: Webdings;\"");
    tb.PrintF(L">");
    if(Font)
    {
      if(Font->Style & sF2C_ITALICS) tb.Print(L"<i>");
      if(Font->Style & sF2C_BOLD) tb.Print(L"<b>");
      if(Font->Style & sF2C_UNDERLINE) tb.Print(L"<u>");
      if(Font->Style & sF2C_STRIKEOUT) tb.Print(L"<s>");
    }
    Active = 1;
  }
}

void sLBHtmlState::End(sTextBuffer &tb)
{
  if(Active)
  {
    sInt size = sRFS_NORMAL;
    const sChar *face = L"Arial";
    if(Font)
    {
      size = Font->Size;
      face = Font->Name;
    }
    if(Font)
    {
      if(Font->Style & sF2C_STRIKEOUT) tb.Print(L"</s>");
      if(Font->Style & sF2C_UNDERLINE) tb.Print(L"</u>");
      if(Font->Style & sF2C_BOLD) tb.Print(L"</b>");
      if(Font->Style & sF2C_ITALICS) tb.Print(L"</i>");
    }
    tb.PrintF(L"</font>");
    Active = 0;
  }
}

void sLBHtmlState::Init(sU32 tc,sU32 bc,sFontResource *f)
{
  sVERIFY(!Active);
  TextColor = tc;
  BackColor = bc;
  Font = f;
}

void sLBHtmlState::Change(sU32 tc,sU32 bc,sFontResource *f,sTextBuffer &tb)
{
  if(f==0) f = Font;
  if(tc==0) tc = TextColor;
  if(bc==0) bc = BackColor;
  if(TextColor!=tc || BackColor!=bc || Font!=f)
  {
    if(Active)
    {
      End(tb);
      Init(tc,bc,f);
      Begin(tb);
    }
    else
    {
      Init(tc,bc,f);
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   pdf                                                                ***/
/***                                                                      ***/
/****************************************************************************/

sLBPdfState::sLBPdfState()
{
  pdf = new sPDF;
  Zoom = PDFZoom;
}

sLBPdfState::~sLBPdfState()
{
  delete pdf;
}

/****************************************************************************/

void sLBPdfState::CalcMatrix(sInt page,const sLBPageInfo &pageinfo,const sRect &r)
{
  Page = page;
  Client = r;

  SX = Zoom;
  SY = -Zoom;
  DX = -r.x0*SX;
  DY = -(r.y0+(pageinfo.SizeY+pageinfo.Border.y0+pageinfo.Border.y1))*SY;
}

sBool sLBPdfState::Hit(const sRect &r)
{
  return r.IsInside(Client);
}

void sLBPdfState::Rect(const sRect &r,sU32 color)
{
  if((color & 0xffffff)!=0xffffff)
  {
    sFRect rr;
    rr.x0 = r.x0*SX+DX;
    rr.y0 = r.y0*SY+DY;
    rr.x1 = r.x1*SX+DX;
    rr.y1 = r.y1*SY+DY;
    pdf->Rect(rr,color);
  }
}

void sLBPdfState::RectFrame(const sRect &r,sU32 color,sInt w)
{
  Rect(r.x0,r.y0,r.x1,r.y0+w,color);
  Rect(r.x0,r.y1-w,r.x1,r.y1,color);
  Rect(r.x0,r.y0+w,r.x0+w,r.y1-w,color);
  Rect(r.x1-w,r.y0+w,r.x1,r.y1-w,color);
/*
  sVERIFY(SX==-SY);

  sFRect rr;
  rr.x0 = r.x0*SX+DX;
  rr.y0 = r.y0*SY+DY;
  rr.x1 = r.x1*SX+DX;
  rr.y1 = r.y1*SY+DY;
  pdf->RectFrame(rr,color,w*SX);
*/
}

void sLBPdfState::Rect(sInt x0,sInt y0,sInt x1,sInt y1,sU32 color)
{
  if((color & 0xffffff)!=0xffffff)
  {
    sFRect rr ;
    rr.x0 = x0*SX+DX;
    rr.y0 = y0*SY+DY;
    rr.x1 = x1*SX+DX;
    rr.y1 = y1*SY+DY;
    pdf->Rect(rr,color);
  }
}

void sLBPdfState::SetPrint(sFontResource *font,sU32 tc,sU32 bc)
{
  if(font->Temp==0)
  {
    sInt pdfstyle = 0;
    if(font->Name==L"Courier" || font->Name==L"Courier New")
      pdfstyle |= sPDF_Courier;
    else 
      pdfstyle |= sPDF_Helvetica;

    if(font->Style & sF2C_ITALICS)
      pdfstyle |= sPDF_Italic;
    if(font->Style & sF2C_BOLD)
      pdfstyle |= sPDF_Bold;

    font->Temp = pdf->RegisterFont(pdfstyle,font->Font);
  }

  Font = font->Font;
  FontId = font->Temp;
  FontScale = sFAbs(font->Font->GetCharHeight());
  FontTextColor = tc&0xffffff;
  FontBackColor = bc&0xffffff;
}

void sLBPdfState::Print(const sRect &r,sInt x,sInt y,const sChar *text,sInt len)
{
  if(FontBackColor!=0xffffff)
    Rect(r,FontBackColor);
  y += Font->GetBaseline();
  pdf->Text(FontId,FontScale*Zoom,x*SX+DX,y*SY+DY,FontTextColor,text,len);
}

void sLBPdfState::Print(sInt align,const sRect &r,const sChar *text,sInt len)
{
  if(len==-1)
    len = sGetStringLen(text);

  sInt x,y;

  if(align & sF2P_LEFT)
    x = r.x0;
  else if(align & sF2P_RIGHT)
    x = r.x1 - Font->GetWidth(text,len);
  else 
    x = r.CenterX() - Font->GetWidth(text,len)/2;

  if(align & sF2P_TOP)
    y = r.y0;
  else if(align & sF2P_BOTTOM)
    y = r.y1 - Font->GetHeight();
  else
    y = r.CenterY() - Font->GetHeight()/2;

  Print(r,x,y,text,len);
}


/****************************************************************************/
/***                                                                      ***/
/***   parser                                                             ***/
/***                                                                      ***/
/****************************************************************************/

void sLayoutWindow::Clear()
{
  delete Root;
  if(PageMode)
    Root = new sLBPage();
  else
    Root = new sLBContinuous();
  LayoutFlag = 1;
  CursorChar = 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   layout                                                             ***/
/***                                                                      ***/
/****************************************************************************/

static void PrepXR(sLayoutBox *b)
{
  sLayoutBox *n = b->Childs;
  while(n)
  {
    PrepXR(n);
    n = n->Next;
  }

  b->PrepX();
}

static void PrepYR(sLayoutBox *b)
{
  sLayoutBox *n = b->Childs;
  while(n)
  {
    PrepYR(n);
    n = n->Next;
  }

  b->PrepY();
}

void sLayoutWindow::DoLayout()
{
  if(Root && Client.SizeX()>0 && Client.SizeY()>0)
  {
    Root->SetLandscape(Landscape);
    PrepXR(Root);
    Root->CalcX(Client.x0,Client.x0+Inner.SizeX());
    PrepYR(Root);
    Root->CalcY(Client.y0,Client.y0,Client.y0,Client.y0,0);
    ReqSizeX = Root->ReqSizeX;
    ReqSizeY = Root->ReqSizeY;
  }
  Root->Client = Client;
  LayoutFlag = 0;
  if(ScrollToAfterLayout)
  {
    ScrollToText(ScrollToAfterLayout);
    ScrollToAfterLayout = 0;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   boxes                                                              ***/
/***                                                                      ***/
/****************************************************************************/

sLayoutBox::sLayoutBox()
{
  Childs = 0;
  Link = &Childs;
  Next = 0;
  Kind = sLBK_UNKNOWN;
  MinX = 0;
  OptX = 0;
  TopY = 0;
  BotY = 0;
  FullWidth = 1;
  PageBreak = 0;
  CanBreak = 0;
  GlueWeight = 0;
  Client.Init(0,0,0,0);
  Temp = 0;
  CursorStart = 0;
  CursorEnd = 0;
  Page = -1;
  MousePointer = sMP_ARROW;
}

sLayoutBox::~sLayoutBox()
{
  sLayoutBox *next = Childs;
  sLayoutBox *box;
  while(next)
  {
    box = next;
    next = next->Next;
    delete box;
  }
}

void sLayoutBox::AddChild(sLayoutBox *box)
{
  *Link = box;
  Link = &box->Next;
}

sInt sLayoutBox::GetChildCount()
{
  sInt n = 0;
  sLayoutBox *b = Childs;
  while(b)
  {
    n++;
    b = b->Next;
  }
  return n;
}

void sLayoutBox::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  sLayoutBox *b = Childs;
  while(b)
  {
    b->MakeHtml(tb,s);
    b = b->Next;
  }
}

void sLayoutBox::MakePDF(sLBPdfState *pdf)
{
  sLayoutBox *b = Childs;
  while(b)
  {
    if(pdf->Hit(b->Client))
      b->MakePDF(pdf);
    b = b->Next;
  }
}

/****************************************************************************/

void sLayoutBox::PrepX()
{
  sVERIFY(!Childs);
}

void sLayoutBox::PrepY()
{
}

void sLayoutBox::CalcX(sInt x0,sInt x1)
{
  Client.x0 = x0;
  Client.x1 = x1;  
}

void sLayoutBox::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  Client.y0 = y0;
  Client.y1 = y1;
  Baseline = bl;
}

void sLayoutBox::Paint(sLayoutWindow *)
{
  sRect2D(Client,sGC_GREEN);
}

const sChar *sLayoutBox::Click(sInt x,sInt y)
{
  return 0;
}

#undef new

struct PoolDesc
{
  sCONFIG_SIZET ObjectSize;
  sCONFIG_SIZET RealSize;
  sMemoryPool InternalPool;
  void *FreeList;

  PoolDesc(sCONFIG_SIZET objectSize)
    : ObjectSize(objectSize),
      RealSize(sMax(objectSize,sizeof(void*))),
      InternalPool(2048 * (sInt) sMax(objectSize,sizeof(void*)),sAMF_HEAP,512),
      FreeList(0)
  {
  }

  sU8 *Alloc()
  {
    sU8 *ptr;

    if(FreeList) // pop from head
    {
      ptr = (sU8*) FreeList;
      FreeList = *((void**) FreeList);
    }
    else
      ptr = InternalPool.Alloc(sInt(RealSize),1);

    return ptr;
  }

  void Free(void *what)
  {
    *((void**) what) = FreeList;
    FreeList = (void *) what;
  }
};

// this should really be thread local!
static sStackArray<PoolDesc*,32> Pools;

static void InitLayoutPool()
{
}

static void ExitLayoutPool()
{
  sDeleteAll(Pools);
}

sADDSUBSYSTEM(LayoutPool,0xc0,InitLayoutPool,ExitLayoutPool);

sInt GetPoolForSize(sCONFIG_SIZET sz)
{
  for(sInt i=0;i<Pools.GetCount();i++)
    if(Pools[i]->ObjectSize == sz)
      return i;

  // no matching pool yet, need to add one
  PoolDesc *pd = new PoolDesc(sz);
  Pools.AddTail(pd);
  return Pools.GetCount() - 1;
}

void *sLayoutBox::operator new(sCONFIG_SIZET sz)
{
  static const sInt tagSz = sizeof(sInt);

  sInt index = GetPoolForSize(sz + tagSz);
  sU8 *ptr = Pools[index]->Alloc();

  *((sInt *) ptr) = index;
  return (void*) (ptr + tagSz);
}

void *sLayoutBox::operator new(sCONFIG_SIZET sz,const char *file,int line)
{
  return operator new(sz);
}

void sLayoutBox::operator delete(void *ptr)
{
  static const sInt tagSz = sizeof(sU32);
  if(!ptr) return;

  sU8 *pBlock = ((sU8 *) ptr) - tagSz;
  sInt index = *(sInt*) pBlock;
  Pools[index]->Free(pBlock);
}

#define new sDEFINE_NEW

/****************************************************************************/

sLBText::sLBText(const sChar *text,sFontResource *font)
{
  Text = text;
  if(font)
  {
    Font = font;
    Font->AddRef();
  }
  else
  {
    Font = sResourceManager->NewLogFont(0,sRFS_NORMAL,0);
  }
  PrintFlags = sF2P_OPAQUE|sF2P_MULTILINE|sF2P_JUSTIFIED;
  TextColor = sGC_BLACK;
  BackColor = sGC_WHITE;
}

sLBText::~sLBText()
{
  sRelease(Font);
}

void sLBText::PrepX()
{
  sVERIFY(!Childs);
  OptX = Font->Font->GetWidth(Text);

  MinX = 0;
  const sChar *str = Text;
  while(sIsSpace(*str)) str++;
  while(*str)
  {
    const sChar *start = str;
    while(*str!=0 && !sIsSpace(*str)) str++;
    sInt sx = Font->Font->GetWidth(start,str-start);
    MinX = sMax(MinX,sx);
    while(sIsSpace(*str)) str++;
  }
}

void sLBText::PrepY()
{
  sPrintInfo pi;

  pi.Init();
  pi.Mode = sPIM_GETHEIGHT;
  TopY = Font->Font->Print(PrintFlags,Client,Text,-1,0,0,0,&pi) - Client.y0;
  BotY = 0;
}

void sLBText::Paint(sLayoutWindow *)
{
  Font->Font->SetColor(MakeColor(TextColor,0),MakeColor(BackColor,1));
  Font->Font->Print(PrintFlags,Client,Text);
}

void sLBText::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->Change(TextColor,BackColor,Font,tb);
  s->Begin(tb);
  tb.Print(Text);
}

/****************************************************************************/

sLBWord::sLBWord(const sChar *text,sInt len,sFontResource *font)
{
  CursorTextStart = 0;
  CursorTextEnd = 0;
  Kind = sLBK_WORD;
  Text = text;
  Length = len;
  EscapedText = text;
  EscapedLength = len;
  WrongSpelling = 0;

  // is there a backslash in there? (escape)
  sInt backslashPos = 0;
  while(backslashPos<len && Text[backslashPos] != '\\')
    backslashPos++;

  if(backslashPos != len)
  {
    sChar *d = new sChar[len+1];
    EscapedText = d;
    EscapedLength = 0;
    for(sInt i=0;i<len;i++)
    {
      if(Text[i]=='\\' && i+1<len)
        i++;
      d[EscapedLength++] = Text[i];
    }
  }

  if(font)
  {
    Font = font;
    Font->AddRef();
  }
  else
  {
    Font = sResourceManager->NewLogFont(0,sRFS_NORMAL,0);
  }
  TextColor = sGC_BLACK;
  BackColor = sGC_WHITE;
}

sLBWord::~sLBWord()
{
  sRelease(Font);
  if(Text!=EscapedText)
    delete EscapedText;
}

void sLBWord::PrepX()
{
  sVERIFY(!Childs);
  MinX = OptX = Font->Font->GetWidth(EscapedText,EscapedLength);
}

void sLBWord::PrepY()
{
  TopY = Font->Font->GetBaseline();
  BotY = Font->Font->GetHeight()-TopY;
}

void sLBWord::Paint(sLayoutWindow *lw)
{
  if(WrongSpelling)
    Font->Font->SetColor(MakeColor(TextColor,0),MakeColor(0xff8080,1));
  else
    Font->Font->SetColor(MakeColor(TextColor,0),MakeColor(BackColor,1));
  sInt y = Baseline-TopY;
  sPrintInfo pi;
  pi.Init();
  pi.CursorPos = -1;
  if(lw->CursorFlash)
  {
    const sChar *c = lw->CursorChar;
    if(c>=CursorTextStart && c<CursorTextEnd)
    {
      if(c>Text)
        pi.CursorPos = Length-1;
      else
        pi.CursorPos = 0;
    }
    if(c>=Text && c<Text+Length)
    {
      pi.CursorPos = lw->CursorChar - Text;
      for(const sChar *s = Text;s<c;s++)
      {
        if(*s=='\\' && s+1<Text+Length)
        {
          s++;
          pi.CursorPos--;
        }
      }
    }
  }
  Font->Font->PrintMarked(sF2P_OPAQUE|sF2P_BOTTOM,&Client,Client.x0,y,EscapedText,EscapedLength,&pi);
}

void sLBWord::MakePDF(sLBPdfState *pdf)
{
  pdf->SetPrint(Font,MakeColorPDF(TextColor),MakeColorPDF(BackColor));
  pdf->Print(Client,Client.x0,Baseline-TopY,EscapedText,EscapedLength);
}

const sChar *sLBWord::Click(sInt x,sInt y)
{
  sPrintInfo pi;
  pi.Init();
  pi.Mode = sPIM_POINT2POS;
  pi.QueryX = x;
  pi.QueryY = y;
  Font->Font->PrintMarked(sF2P_OPAQUE|sF2P_BOTTOM,&Client,Client.x0,Baseline-TopY,Text,Length,&pi);
  if(pi.Mode==sPIM_QUERYDONE)
    return pi.QueryPos;
  else
    return 0;
}

void sLBWord::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->Change(TextColor,BackColor,Font,tb);
  s->Begin(tb);
  for(sInt i=0;i<Length;i++)
  {
    switch(Text[i])
    {
    case ' ':
      tb.Print(L"&nbsp;");
      break;
    case '<':
      tb.Print(L"&lt;");
      break;
    case '>':
      tb.Print(L"&gt;");
      break;
    case '&':
      tb.Print(L"&amp;");
      break;
    default:
      tb.PrintChar(Text[i]);
      break;
    }
  }
}

/****************************************************************************/

sLBGlue::sLBGlue(const sChar *text,sInt len,sFontResource *font) : sLBWord(text,len,font)
{
  Kind = sLBK_GLUE;
  GlueWeight = 1;
  BorderColor = BackColor;
  Border = 0;
}

void sLBGlue::Paint(sLayoutWindow *lw)
{
  if(!Border && (Font->Style & (sF2C_UNDERLINE | sF2C_STRIKEOUT)))
  {
    sString<256> spaces;
    for(sInt i=0;i<255;i++) spaces[i]=' ';
    spaces[255] = 0;
    sClipPush();
    sClipRect(Client);
    sPrintInfo pi;
    pi.Init();
    pi.CursorPos=-1;
    Font->Font->PrintMarked(sF2P_OPAQUE|sF2P_BOTTOM,&Client,Client.x0,Baseline-TopY,spaces,-1,&pi);
    sClipPop();
  }
  else
  {
    sRect2D(Client,MakeColor(Border ? BorderColor : BackColor,0));
  }
  if((lw->CursorChar>=Text && lw->CursorChar<Text+Length) || 
     (lw->CursorChar>=CursorTextStart && lw->CursorChar<CursorTextEnd))
  {
    if(lw->CursorFlash)
    {
      const sRect &r = Client;
      sInt x = r.x0;
      if(Text[0]==' ' && lw->CursorChar>Text)
      {
        x += Font->Font->GetWidth(L" ");
        if(x+2>r.x1) x = r.x1-2;
        if(x<r.x0) x = r.x0;
      }
      sRect2D(x,r.y0,x+2,r.y1,sGC_BLACK);
    }
  }
}


const sChar *sLBGlue::Click(sInt x,sInt y)
{
  return Text;
}

void sLBGlue::PrepX()
{
  sVERIFY(!Childs);
  if(GlueWeight)
    MinX = OptX = Font->Font->GetWidth(L" ");
  else
    MinX = OptX = 0;
}

void sLBGlue::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->Change(TextColor,BackColor,Font,tb);
  s->Begin(tb);
  tb.Print(L" ");
}

void sLBGlue::MakePDF(sLBPdfState *)
{
}

/****************************************************************************/

sLBParagraph::sLBParagraph(sFontResource *font)
{
  BackColor = sGC_WHITE;
  SpaceWidth = font ? font->Font->GetWidth(L" ") : sGui->PropFont->GetWidth(L" ");
  PrintFlags = sF2P_JUSTIFIED|sF2P_SPACE;
  CanBreak = 1;
}

sLBParagraph::sLBParagraph(const sChar *text,sFontResource *font)
{
  BackColor = sGC_WHITE;
  SpaceWidth = font ? font->Font->GetWidth(L" ") : sGui->PropFont->GetWidth(L" ");
  PrintFlags = sF2P_JUSTIFIED|sF2P_SPACE;
  AddText(text,font);
  CanBreak = 1;
}

sLBParagraph::sLBParagraph(const sChar *text,sFontResource *font,sInt count)
{
  BackColor = sGC_WHITE;
  SpaceWidth = font ? font->Font->GetWidth(L" ") : sGui->PropFont->GetWidth(L" ");
  PrintFlags = sF2P_JUSTIFIED|sF2P_SPACE;
  AddText(text,font,sGC_BLACK,count);
  CanBreak = 1;
}

sLBParagraph::~sLBParagraph()
{
}

void sLBParagraph::AddText(const sChar *text,sFontResource *font,sU32 textcolor,sInt count)
{
  if(count==-1) count = 0x7fffffff;
  while(sIsSpace(*text)) { text++; count--; }

  while(*text && count>0)
  {
    const sChar *start = text;
    while(*text && !sIsSpace(*text) && count>0) { text++; count--; }
    sLBWord *word= new sLBWord(start,text-start,font);
    word->BackColor = BackColor;
    word->TextColor = textcolor;
    AddChild(word);
    if(sIsSpace(*text))
    {
      const sChar *start = text;
      while(sIsSpace(*text)) { text++; count--; }
      sLBGlue *glue = new sLBGlue(start,text-start,font);
      glue->BackColor = BackColor;
      glue->BorderColor = BackColor;
      glue->TextColor = textcolor;
      AddChild(glue);
    }
  }
}

void sLBParagraph::Paint(sLayoutWindow *lw)
{
  sRect *rp;
  sFORALL(Clients,rp)
    sRect2D(*rp,MakeColor(BackColor,1));

  Space *space;
  sFORALL(Spaces,space)
  {
    if(lw->CursorChar >=space->Start && lw->CursorChar<space->End && lw->CursorFlash)
    {
      const sRect &r = space->Word->Client;
      sRect2D(r.x1,r.y0,r.x1+2,r.y1,sGC_BLACK);
    }
  }
}

void sLBParagraph::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->Change(0,BackColor,0,tb);
  tb.PrintF(L"<div style=\"background-color:#%06x\">",getcolor(s->BackColor));

  sLayoutBox *c = Childs;
  sBool onlyglue = 1;
  while(c && onlyglue)
  {
    if(!(c->Kind==sLBK_GLUE || (c->Kind==sLBK_WORD && ((sLBWord*)c)->Length==0)))
      onlyglue = 0;
    c = c->Next;
  }

  if(!onlyglue)
  {
    sLayoutBox::MakeHtml(tb,s);
  }
  else 
  {
    s->Begin(tb);
    tb.Print(L"&nbsp;");
  }
  s->End(tb);
  tb.Print(L"</div>\n");
}

void sLBParagraph::MakePDF(sLBPdfState *pdf)
{
  sRect *rp;
  sFORALL(Clients,rp)
    pdf->Rect(*rp,MakeColorPDF(BackColor));
  sLayoutBox::MakePDF(pdf);
}

void sLBParagraph::PrepX()
{
  sLayoutBox *b = Childs;
  MinX = 0;
  OptX = 0;
  sInt n = 0;
  while(b)
  {
    MinX = sMax(MinX,b->OptX);
    OptX = sMax(OptX,b->OptX);
    b = b->Next;
    n++;
  }

  if(PrintFlags & sF2P_SPACE)
  {
    sInt space = SpaceWidth;
    MinX += space*2;
    OptX += space*2;
  }
}

void sLBParagraph::CalcX(sInt x0,sInt x1)
{
  Lines.Clear();
  Client.x0 = x0;
  Client.x1 = x1;
  sInt space = SpaceWidth;
  sLayoutBox *b;

  // special case: only one child (for empty text, mostly)

  if(Childs && Childs->Next==0)
  {
    Childs->CalcX(x0,x1);
    return;
  }

  // some space left and right

  if(PrintFlags & sF2P_SPACE)
  {
    x0 += space;
    x1 -= space;
  }

  // mark all boxes

  b = Childs;
  while(b)
  {
    b->Temp = 0;
    if(b->Kind==sLBK_GLUE)
      ((sLBGlue *)b)->Border = 0;
    b = b->Next;
  }

  // find the lines

  sInt xs = x1-x0;
  b = Childs;
  while(b && b->Kind==sLBK_GLUE) b = b->Next;    // skip glue at begin of line
  while(b)
  {
    Line *line = Lines.AddMany(1);
    line->First = b;
    line->Last = b;
    line->LastGlue = b;
    
    sInt xp = 0;
    while(b && (xp+b->OptX<=xs || xp==0 || b->Kind==sLBK_GLUE))
    {
      xp += b->OptX;
      if(b->GlueWeight==0)     // skip glue at end of line
        line->Last = b;
      line->LastGlue = b;   // but remember glue!

      b = b->Next;
    }
    while(b && b->Kind==sLBK_GLUE) b = b->Next;    // skip glue at begin of line
  }

  // assign the lines

  Line *line;
  sFORALL(Lines,line)
  {    
    b = line->First;
    sInt used = 0;
    sInt weight = 0;
    for(;;)
    {
      used += b->OptX;
      weight += b->GlueWeight;
      if(b==line->Last) break;
      b = b->Next;
    }

    sInt xp = x0;
    sInt surplus = xs-used;

    if(PrintFlags & sF2P_JUSTIFIED)
    {
      if(weight==0)
      {
        weight=1;
        surplus=0;
      }
      sInt w0,w1,w;
      w1 = 0;
      b = line->First;
      sBool lastline = (_i==Lines.GetCount()-1);
      for(;;)
      {
        w0 = w1;
        w1 += lastline ? 0 : b->GlueWeight;
        w = b->OptX + w1*surplus/weight - w0*surplus/weight;
        b->CalcX(xp,xp+w);
        b->Temp = 1;
        xp += w;
        if(b == line->Last) break;
        b = b->Next;
      }
    }
    else
    {
      if(PrintFlags & sF2P_LEFT)
        xp = x0;
      else if(PrintFlags & sF2P_RIGHT)
        xp = x0+surplus;
      else
        xp = x0+surplus/2;

      b = line->First;
      for(;;)
      {
        b->CalcX(xp,xp+b->OptX);
        b->Temp = 1;
        xp += b->OptX;
        if(b == line->Last) break;
        b = b->Next;
      }
    }

    if(line->Last!=line->LastGlue)
    {
      b = line->Last->Next;
      for(;;)
      {
        b->CalcX(xp,Client.x1);
        b->Temp = 1;
        if(b->Kind==sLBK_GLUE)
          ((sLBGlue *)b)->Border = 1;
        if(b == line->LastGlue) break;
        b = b->Next;
      }
    }
  }

  // assign forgotten boxes

  b = Childs;
  while(b)
  {
    if(b->Temp==0)
    {
      b->CalcX(Client.x0,Client.x0);
    }
    b = b->Next;
  }

}

void sLBParagraph::PrepY()
{
  sLayoutBox *b;
  TopY = BotY = 0;
  Line *line;
  sFORALL(Lines,line)
  {
    sInt ty=0,by=0;
    b = line->First;
    for(;;)
    {
      ty = sMax(ty,b->TopY);
      by = sMax(by,b->BotY);
      if(b==line->Last) break;
      b = b->Next;
    }
    TopY += ty;
    BotY += by;
  }
}

void sLBParagraph::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  Client.y0 = y0;
  Client.y1 = y1;
  Clients.Clear();
//  Clients.AddTail(Client);

  sLayoutBox *b;

  // special case: only one child (for empty text, mostly)

  if(Childs && Childs->Next==0)
  {
    Childs->CalcY(y0,y1,bl,y1,root);
    return;
  }

  // mark all boxes

  b = Childs;
  while(b)
  {
    b->Temp = 0;
    b = b->Next;
  }

  // assign lines

  sInt yp = y0;
  Line *line;
  sFORALL(Lines,line)
  {
    sInt ty=0,by=0;
    b = line->First;
    for(;;)
    {
      ty = sMax(ty,b->TopY);
      by = sMax(by,b->BotY);
      if(b==line->Last) break;
      b = b->Next;
    }

    sInt ys =ty+by;

    if(root && yp+ys>ybreak)
    {
      Clients.AddTail(sRect(Client.x0,y0,Client.x1,yp));
      root->NextPage(yp,ybreak);
      y0 = yp;
    }

    b = line->First;
    for(;;)
    {
      b->CalcY(yp,yp+ys,yp+ty,yp+ys,root);
      b->Temp = 1;
      if(b==line->Last) break;
      if(root) b->Page = root->PageNum;
      b = b->Next;
    }

    if(line->Last!=line->LastGlue)
    {
      b = line->Last->Next;
      for(;;)
      {
        b->CalcY(yp,yp+ys,yp+ty,yp+ys,root);
        b->Temp = 1;
        if(b == line->LastGlue) break;
        b = b->Next;
      }
    }

    yp += ys;
  }
  if(yp<y1) 
    yp = y1;
  if(root)
    root->YCursor = yp;
  Clients.AddTail(sRect(Client.x0,y0,Client.x1,yp));
  Client.y1 = yp;

  // assign forgotten boxes

  b = Childs;
  while(b)
  {
    if(b->Temp==0)
    {
      b->CalcY(y0,y0,y0,y0,root);
    }
    b = b->Next;
  }

}

const sChar *sLBParagraph::Click(sInt x,sInt y)
{
  Space *space;
  sFORALLREVERSE(Spaces,space)
  {
    if(y >= space->Word->Client.y0 && y < space->Word->Client.y1)
    {
      if(x >= space->Word->Client.x1)
      {
        return space->Start;
      }
    }
  }
  return 0;
}


/****************************************************************************/

sLBImage::sLBImage(const sChar *name)
{
  Image = sResourceManager->NewImage(name);
  ImageName = name;
  ScaledX = sMax(Image->Image->GetSizeX(),1);
  ScaledY = sMax(Image->Image->GetSizeY(),1);
  BackColor = sGC_WHITE;
  Align = 0;
}

sLBImage::~sLBImage()
{
  sRelease(Image);
}

void sLBImage::PrepX()
{
  MinX = OptX = ScaledX;
  if(ScaledX==0)
    OptX = 1;
  sVERIFY(Childs==0);
}
/*
void sLBImage::CalcX(sInt x0,sInt x1)
{
  if(ScaledX==0)
    x0 =
}
*/
void sLBImage::PrepY()
{
  if(ScaledY>0)
    TopY = ScaledY;
  else
    TopY = Client.SizeX();
  BotY = 0;
}

void sLBImage::Paint(sLayoutWindow *)
{
  sRect r;
  sInt oxs = Image->Image->GetSizeX();
  sInt oys = Image->Image->GetSizeY();
  sInt xs = ScaledX;
  sInt ys = ScaledY;
  if(xs==0 || ys==0)
  {
    xs = Client.SizeX();
    ys = Client.SizeX()*oys/oxs;
  }

  r.x0 = Client.x0 + (Client.SizeX()-xs)/2;
  r.y0 = Baseline-ys;

  if(Align&1)
    r.x0 = Client.x0;
  if(Align&2)
    r.x0 = Client.x1 - xs;
  if(Align&4)
    r.y0 = Client.y0;
  if(Align&8)
    r.y0 = Client.y1 - ys;
  if(Align&16)
    r.y0 = Client.y0 + (Client.SizeY()-ys)/2;
    
  r.x1 = r.x0 + xs;
  r.y1 = r.y0 + ys;

  if(xs==oxs && ys==oys)
  {
    Image->Image->Paint(r.x0,r.y0);
  }
  else
  {
    sRect src(0,0,oxs,oys);
    Image->Image->Stretch(src,r);
  }
  sClipExclude(r);

  sRect2D(Client,MakeColor(BackColor,1));
}

void sLBImage::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->End(tb);
  tb.PrintF(L"<img src=%q>",ImageName);
  sLayoutBox::MakeHtml(tb,s);
  tb.PrintF(L"</img>");
}

/****************************************************************************/

sLBPageNumber::sLBPageNumber(sLayoutBox *ref,sFontResource *font)
{
  Ref = ref; 
  Font = font;
  TextColor = sGC_BLACK;
  BackColor = sGC_WHITE;
}

sLBPageNumber::~sLBPageNumber()
{
}

void sLBPageNumber::PrepX()
{
  MinX = OptX = Font->Font->GetWidth(L"99999");
}

void sLBPageNumber::PrepY()
{
  TopY = Font->Font->GetBaseline();
  BotY = Font->Font->GetHeight()-TopY;
}

void sLBPageNumber::Paint(sLayoutWindow *lw)
{
  sString<64> buffer;
  buffer.PrintF(L"%d",Ref->Page+1);
  
  Font->Font->SetColor(MakeColor(TextColor,0),MakeColor(BackColor,1));
  Font->Font->Print(sF2P_OPAQUE|sF2P_BOTTOM|sF2P_RIGHT,Client,buffer);
}

void sLBPageNumber::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  tb.PrintF(L"%d",Ref->Page+1);
}

void sLBPageNumber::MakePDF(sLBPdfState *pdf)
{
  sString<64> buffer;
  buffer.PrintF(L"%d",Ref->Page+1);

  pdf->SetPrint(Font,MakeColorPDF(TextColor),MakeColorPDF(BackColor));
  pdf->Print(sF2P_OPAQUE|sF2P_BOTTOM|sF2P_RIGHT,Client,buffer);
}

/****************************************************************************/

sLBSpacer::sLBSpacer(sInt x,sInt y)
{
  BackColor = sGC_WHITE;
  SpaceX = x;
  SpaceY = y;
}

sLBSpacer::sLBSpacer(sInt x,sInt y,sU32 color)
{
  BackColor = color;
  SpaceX = x;
  SpaceY = y;
}

sLBSpacer::~sLBSpacer()
{
}

void sLBSpacer::PrepX()
{
  MinX = OptX = SpaceX;
  sVERIFY(Childs==0);
}

void sLBSpacer::PrepY()
{
  TopY = SpaceY;
  BotY = 0;
}

void sLBSpacer::Paint(sLayoutWindow *)
{
  sRect2D(Client,MakeColor(BackColor,1));
}

void sLBSpacer::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
/*
  tb.PrintF(L"<table width=\"100%%\"  cellpadding=\"0\" cellspacing=\"0\" border=\"0\">"
    L"<tr><td bgcolor=\"%06x\" style=\"width:%dpx height:%dpx\"><p></p></td></tr></table>\n"
    ,getcolor(BackColor),SpaceX,SpaceY);
    */

  if(SpaceX==1 && SpaceY==1)
  {
  }
  else
  {
    tb.PrintF(L"<table width=\"100%%\"  cellpadding=\"%d\" cellspacing=\"0\" border=\"0\" bgcolor=\"%06x\">"
      L"<tr><td><p></p></td></tr></table>\n"
      ,SpaceY/2,getcolor(BackColor));
  }
}

/****************************************************************************/

sLBTable2::sLBTable2(sInt c,sInt modex)
{
  Kind = sLBK_TABLE2;
  ClickString = 0;
  Columns = c;
  CountX = 0;
  CountY = 0;
  ModeX = modex;
  BorderColor = sGC_DRAW;
  BackColor = sGC_WHITE;
  InnerBorder = 0;
  OuterBorder = 0;
  CanBreak = 1;

  if(Columns>0)
  {
    Cols.Resize(Columns);
    Column *col;
    sFORALL(Cols,col)
    {
      col->Weight = 1;
      col->Min = 0;
      col->Opt = 0;
      col->Size = 0;
      col->NoFit = 0;
    }
  }
}

const sChar *sLBTable2::Click(sInt x,sInt y)
{
  return ClickString;
}

void sLBTable2::PrepX()
{
  // count boxes

  sInt count=0;
  sLayoutBox *b = Childs;
  while(b)
  {
    b = b->Next;
    count++;
  }

  // fix box count

  if(Columns==0)
    CountX = count;
  else
    CountX = Columns;
  if(CountX==0)
    CountX = 1;
  CountY = (count+CountX-1)/CountX;

  if(CountX!=Cols.GetCount())
  {
    Cols.Resize(CountX);
    Column *col;
    sFORALL(Cols,col)
    {
      col->Weight = 1;
      col->Min = 0;
      col->Opt = 0;
      col->Size = 0;
      col->NoFit = 0;
    }
  }

  // add dummy boxes to make a full rectangle of boxes

  while(count<CountX*CountY)
  {
    AddChild(new sLayoutBox);
    count++;
  }
  sVERIFY(count==CountX*CountY);

  // calc min and opt

  b = Childs;
  MinX = 0;
  OptX = 0;
  for(sInt y=0;y<CountY;y++)
  {
    sInt mx=0;
    sInt ox=0;
    for(sInt x=0;x<CountX;x++)
    {
      mx += b->MinX;
      ox += b->OptX;

      b = b->Next;
    }
    MinX = sMax(MinX,mx);
    OptX = sMax(OptX,ox);
  }

  MinX += (CountX-1)*InnerBorder + 2*OuterBorder;
  OptX += (CountX-1)*InnerBorder + 2*OuterBorder;
}

void sLBTable2::PrepY()
{
  sLayoutBox *b = Childs;
  TopY = 0;
  BotY = 0;
  for(sInt y=0;y<CountY;y++)
  {
    sInt ty=0;
    for(sInt x=0;x<CountX;x++)
    {
      ty = sMax(ty,b->TopY+b->BotY);

      b = b->Next;
    }
    TopY += ty;
    BotY += 0;
  }

  TopY += (CountY-1)*InnerBorder + 2*OuterBorder;
}

void sLBTable2::CalcX(sInt x0,sInt x1)
{
  sLayoutBox *b;
  Column *col;

  // measure boxes

  sFORALL(Cols,col)
  {
    col->Size = 0;
    col->Opt = 0;
    col->Min = 0;
    col->NoFit = 0;
  }

  b = Childs;
  for(sInt y=0;y<CountY;y++)
  {
    for(sInt x=0;x<CountX;x++)
    {
      col = &Cols[x];
      col->Min = sMax(col->Min,b->MinX);
      col->Opt = sMax(col->Opt,b->OptX);
      b = b->Next;
    }
  }

  sInt allmin = 0;
  sInt allopt = 0;
  sInt allweight = 0;
  sFORALL(Cols,col)
  {
    allmin += col->Min;
    allopt += col->Opt;
    allweight += col->Weight;
  }

  // do it

  sInt xs = x1-x0-InnerBorder*(CountX-1)-2*OuterBorder;

  sFORALL(Cols,col)
    col->Size = col->Min;

  if(xs>allmin)
  {
    if(ModeX==sLTF_MIN)             // distribute min, then rest
    {
      sInt rest = sMax(0,xs-allmin);
      sInt w0 = 0;
      sInt w1 = 0;
      sFORALL(Cols,col)
      {
        w0 = w1;
        w1 += col->Weight;
        col->Size = col->Min + (rest*w1/allweight - rest*w0/allweight);
      }
    }
    else if(ModeX==sLTF_OPT)             // distribute min, then rest
    {
      sInt rest = sMax(0,xs-allopt);
      sInt w0 = 0;
      sInt w1 = 0;
      sFORALL(Cols,col)
      {
        w0 = w1;
        w1 += col->Weight;
        col->Size = col->Opt + (rest*w1/allweight - rest*w0/allweight);
      }
    }
    else                            // distribute evenly, if possible
    {
      sInt nofit = 1;
      do
      {
        sInt xleft = xs;
        sInt wleft = 0;
        sFORALL(Cols,col)
        {
          if(col->NoFit)
            xleft -= col->Min;
          else
            wleft += col->Weight;
        }

        nofit = 0;
        if(wleft<1)
        {
          sFORALL(Cols,col)
            col->Size = col->Min;
        }
        else
        {
          sInt w0 = 0;
          sInt w1 = 0;
          sFORALL(Cols,col)
          {
            if(col->NoFit)
            {
              col->Size = col->Min;
            }
            else
            {
              w0 = w1;
              w1 += col->Weight;
              col->Size = xleft*w1/wleft - xleft*w0/wleft;
              if(col->Size<col->Min)
              {
                col->NoFit = 1;
                nofit = 1;
              }
            }
          }
        }
      }
      while(nofit);
    }
  }

  // assign

  b = Childs;
  sInt p = x0;
  for(sInt y=0;y<CountY;y++)
  {
    p = x0+OuterBorder;
    for(sInt x=0;x<CountX;x++)
    {
      b->CalcX(p,p+Cols[x].Size);
      p+=Cols[x].Size+InnerBorder;
      b = b->Next;
    }
  }

  // sizes

  Client.x0 = x0;
  Client.x1 = x1;
  UsedX0 = x0;
  UsedX1 = p + OuterBorder-InnerBorder;
}

void sLBTable2::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  sLayoutBox *b;
  sInt sizemin,sizetotal;
  sInt n;
  Used.Clear();
  Clients.Clear();
  Client.y0 = y0;

  // prepare

  n = CountY;
  sizetotal = y1-y0-InnerBorder*(n-1)-2*OuterBorder;

  // average out

  sInt *size = sALLOCSTACK(sInt,n);
  sInt *top = sALLOCSTACK(sInt,n);
  sInt *bot = sALLOCSTACK(sInt,n);
  for(sInt i=0;i<n;i++)
    size[i] = top[i] = bot[i] = 0;
  b = Childs;
  for(sInt y=0;y<CountY;y++)
  {
    for(sInt x=0;x<CountX;x++)
    {
      top[y] = sMax(top[y],b->TopY);
      bot[y] = sMax(bot[y],b->BotY);
      b = b->Next;
    }
  }
  sizemin = 0;
  for(sInt i=0;i<n;i++)
    sizemin += top[i]+bot[i];

  // verteile minimum

  sInt current = 0;
  for(sInt i=0;i<n;i++)
  {
    size[i] = top[i]+bot[i];
    current += size[i]; 
  }

  // assign

  Rows.Clear();
  b = Childs;
  sInt ystart = y0;
  sInt p = y0+OuterBorder;
  for(sInt y=0;y<CountY;y++)
  {
    if(root && p+size[y]+OuterBorder-InnerBorder>ybreak)
    {
      p -= InnerBorder;
      p += OuterBorder;
      Used.AddTail(sRect(UsedX0,ystart,UsedX1,p));
      Clients.AddTail(sRect(Client.x0,ystart,Client.x1,p));

      root->NextPage(p,ybreak);
      ystart = p;
      p += OuterBorder;
    }
    if(root && y<CountY-1)
    {
      Row *r = Rows.AddMany(1);
      r->Page = root->PageNum;
      r->y0 = p;
      r->y1 = p+size[y];
    }

    for(sInt x=0;x<CountX;x++)
    {
      b->CalcY(p,p+size[y],p+size[y],p+size[y],root);
      b = b->Next;
    }
    p+=size[y]+InnerBorder;
  }
  p -= InnerBorder;
  p += OuterBorder;
  Used.AddTail(sRect(UsedX0,ystart,UsedX1,p));
  if(p<y1) 
    p = y1;
  if(root)
    root->YCursor = p;
  Clients.AddTail(sRect(Client.x0,ystart,Client.x1,p));


  // sizes

  Client.y1 = p;
  Baseline = bl;
}

void sLBTable2::Paint(sLayoutWindow *)
{
  sRect *rp;
  sFORALL(Used,rp)
  {
    sRect2D(*rp,MakeColor(BorderColor,3));
    sClipExclude(*rp);
  }
  sFORALL(Clients,rp)
  {
    sRect2D(*rp,MakeColor(BackColor,4));
  }
}

void sLBTable2::MakePDF(sLBPdfState *pdf)
{
  sRect *rp;

  sFORALL(Clients,rp)
  {
    pdf->Rect(*rp,MakeColorPDF(BackColor));
  }

  sLayoutBox::MakePDF(pdf);

  sFORALL(Clients,rp)
  {
    sU32 col = MakeColorPDFBW(BorderColor);
    if(OuterBorder>0)
      pdf->RectFrame(*rp,col,OuterBorder);
    sInt x = rp->x0 + OuterBorder + Cols[0].Size;
    if(InnerBorder>0)
    {
      for(sInt i=1;i<CountX;i++)
      {
        pdf->Rect(sRect(x,rp->y0+OuterBorder,x+InnerBorder,rp->y1-OuterBorder),col);
        x += Cols[i].Size + InnerBorder;
      }
      Row *row;
      sFORALL(Rows,row)
      {
        if(row->Page==pdf->GetPage())
        {
          sRect r(rp->x0+OuterBorder,row->y1,rp->x1-OuterBorder,row->y1+InnerBorder);
          pdf->Rect(r,col);
        }
      }
    }
  }
}

void sLBTable2::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  s->End(tb);
  s->Change(0,BackColor,0,tb);
  sLayoutBox *c = Childs;
  const sChar *width=L"";
  if(ModeX==sLTF_FULL) width=L" width=\"100%\"";
  tb.PrintF(L"<table cellspacing=\"%d\" cellpadding=\"0\" bgcolor=\"#%06x\"%s>\n",OuterBorder,getcolor(BorderColor),width);
  for(sInt y=0;y<CountY;y++)
  {
    tb.PrintF(L"  <tr>\n");

    sLayoutBox *ct = c;
//    sInt weight = 0;
    for(sInt x=0;x<CountX;x++)
    {
      if(ct)
      {
//        weight += ct->WeightX;
        ct = ct->Next;
      }
    }

    for(sInt x=0;x<CountX;x++)
    {
      tb.PrintF(L"    <td bgcolor=\"#%06x\" valign=\"top\"",getcolor(BackColor));
//      if(weight>0 && (ModeX==sLTF_WEIGHT || ModeX==sLTF_WEIGHTMIN))
//        tb.PrintF(L" width=\"%d%%\"",(c ? c->WeightX : 0)*100/weight);
      tb.Print(L">\n");
      if(c)
      {
        c->MakeHtml(tb,s);
        c = c->Next;
        s->End(tb);
      }
      else
      {
        s->Begin(tb);
        tb.PrintF(L"      &nbsp;");
      }
      tb.PrintF(L"    </td>\n");
    }
    tb.PrintF(L"  </tr>\n");
  }
  tb.PrintF(L"</table>\n");
}


/****************************************************************************/

sLBBorder::sLBBorder(sInt extend,sU32 col,sLayoutBox *child)
{
  Extend.Init(extend,extend,extend,extend);
  Color = col;
  AddChild(child);
  CanBreak = 1;
}

void sLBBorder::PrepX()
{
  if(Childs)
  {
    sVERIFY(!Childs->Next);

    MinX = Childs->MinX + Extend.x0 + Extend.x1;
    OptX = Childs->OptX + Extend.x0 + Extend.x1;
  }
  else
  {
    MinX = OptX = Extend.x0 + Extend.x1;
  }
}

void sLBBorder::PrepY()
{
  if(Childs)
  {
    TopY = Childs->TopY + Extend.y0 + Childs->BotY + Extend.y1;
    BotY = 0;
  }
  else
  {
    TopY = Extend.y0 + Extend.y1;
    BotY = 0;
  }
}

void sLBBorder::CalcX(sInt x0,sInt x1)
{
  Client.x0 = x0;
  Client.x1 = x1;
  if(Childs)
    Childs->CalcX(x0+Extend.x0,x1-Extend.x1);
}

void sLBBorder::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  Baseline = bl;
  if(root && Childs && Childs->CanBreak)
  {
    sInt yp = y0;
    Client.y0 = yp;
    yp += Extend.y0;

    root->YCursor = yp;
    if(Childs)
      Childs->CalcY(yp,y1-Extend.y1,y1-Extend.y1,ybreak,root);
    yp = root->YCursor;

    yp += Extend.y1;
    root->YCursor = yp;
    if(yp<y1) yp = y1;
    Client.y1 = yp;
    Clients = Childs->Clients;
    sInt max = Clients.GetCount();
    sRect *rp;
    sFORALL(Clients,rp)
    {
      rp->x0 = Client.x0;
      rp->x1 = Client.x1;
      if(_i==max-1)
      {
        rp->y1 += Extend.y1;
        rp->y1 = sMax(rp->y1,y1);
      }
    }
  }
  else
  {
    Client.y0 = y0;
    Client.y1 = y1;
    if(Childs)
      Childs->CalcY(y0+Extend.y0,y1-Extend.y1,y1-Extend.y1,y1-Extend.y1,root);
  }
}

void sLBBorder::Paint(sLayoutWindow *)
{
  sU32 col = MakeColor(Color,2);

  if(Clients.GetCount()>0)    // this code is called when the client rect spans over multiple pages. expect bugs in PDF :-)
  {
    sRect *rp;
    sFORALL(Clients,rp)
    {
      sClipPush();
      sClipRect(*rp);
      sRect2D(Client.x0          ,Client.y0          ,Client.x1          ,Client.y0+Extend.y0,col);
      sRect2D(Client.x0          ,Client.y1-Extend.y1,Client.x1          ,Client.y1          ,col);
      sRect2D(Client.x0          ,Client.y0+Extend.y0,Client.x0+Extend.x0,Client.y1-Extend.y1,col);
      sRect2D(Client.x1-Extend.x1,Client.y0+Extend.y0,Client.x1          ,Client.y1-Extend.y1,col);
      sClipPop();
    }
  }
  else
  {
    sRect2D(Client.x0          ,Client.y0          ,Client.x1          ,Client.y0+Extend.y0,col);
    sRect2D(Client.x0          ,Client.y1-Extend.y1,Client.x1          ,Client.y1          ,col);
    sRect2D(Client.x0          ,Client.y0+Extend.y0,Client.x0+Extend.x0,Client.y1-Extend.y1,col);
    sRect2D(Client.x1-Extend.x1,Client.y0+Extend.y0,Client.x1          ,Client.y1-Extend.y1,col);
  }
}

void sLBBorder::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  tb.PrintF(L"<table cellspacing=\"%d\" cellpadding=\"0\" bgcolor=\"%06x\" width=\"100%%\"><tr><td>\n"
    ,Extend.x0,getcolor(Color));
  sLayoutBox::MakeHtml(tb,s);
  tb.PrintF(L"</td></tr></table>\n");
}

void sLBBorder::MakePDF(sLBPdfState *pdf)
{
  sLayoutBox::MakePDF(pdf);

  sU32 col = MakeColorPDF(Color);

  if(Client.y0 + Extend.y0 == Client.y1 - Extend.y1)
  {
    pdf->Rect(Client.x0,Client.y0,Client.x1,Client.y1,col);
  }
  else if(Extend.x0 == Extend.y0 == Extend.x1 == Extend.y1)
  {
    pdf->RectFrame(Client,col,Extend.x0);
  }
  else
  {
    if(Extend.y0>0)  pdf->Rect(Client.x0          ,Client.y0          ,Client.x1          ,Client.y0+Extend.y0,col);
    if(Extend.y1>0)  pdf->Rect(Client.x0          ,Client.y1-Extend.y1,Client.x1          ,Client.y1          ,col);
    if(Extend.x0>0)  pdf->Rect(Client.x0          ,Client.y0+Extend.y0,Client.x0+Extend.x0,Client.y1-Extend.y1,col);
    if(Extend.x1>0)  pdf->Rect(Client.x1-Extend.x1,Client.y0+Extend.y0,Client.x1          ,Client.y1-Extend.y1,col);
  }
}

/****************************************************************************/

sLBContinuous::sLBContinuous()
{
  BackColor = sGC_WHITE;
}

void sLBContinuous::PrepX()
{
  MinX = 0;
  OptX = 0;
  sLayoutBox *b = Childs;
  while(b)
  {
    OptX = sMax(OptX,b->OptX);
    b = b->Next;
  }
}

void sLBContinuous::PrepY()
{
  TopY = 0;
  BotY = 0;
  sLayoutBox *b = Childs;
  while(b)
  {
    TopY += b->TopY + b->BotY;
    b = b->Next;
  }
}

void sLBContinuous::CalcX(sInt x0,sInt x1)
{
  Client.x0 = x0;
  Client.x1 = x1;

  sLayoutBox *b = Childs;
  ReqSizeX = x1-x0;
  while(b)
  {
    sInt xs = b->OptX;
    if(b->FullWidth)
      xs = x1-x0;
    if(b->MinX>x1-x0)
    {
      xs = b->MinX;
      ReqSizeX = sMax(ReqSizeX,xs);
    }
    b->CalcX(x0,x0+xs);
    b = b->Next;
  }
}

void sLBContinuous::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  Client.y0 = y0;
  Client.y1 = y1;
  Baseline = bl;

  sLayoutBox *b = Childs;
  sInt y = y0;
  while(b)
  {
    b->CalcY(y,y+b->TopY+b->BotY,y+b->TopY,y+b->TopY+b->BotY,root);
    y += b->TopY+b->BotY;
    b = b->Next;
  }
  ReqSizeY = y-y0;
}

void sLBContinuous::Paint(sLayoutWindow *)
{
  sRect2D(Client,MakeColor(BackColor,1));
}

void sLBContinuous::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  sLayoutBox::MakeHtml(tb,s);
}

void sLBContinuous::MakePDF(sLBPdfState *pdf)
{
  // no pdf in continous mode!
}

/****************************************************************************/

sLBPageInfo::sLBPageInfo()
{
  Border.Init(50,50,50,75);
  SizeX = 597/PDFZoom-Border.x0-Border.x1;
  SizeY = 842/PDFZoom-Border.y0-Border.y1;
  GrayBorder = 8;

  Header = L"";
  Footer = L"%t||%p";
  HeaderRuler = 0;
  FooterRuler = 1;
  Title = L"";
}

sLBPage::sLBPage()
{
  BackColor = sGC_WHITE;

  TitleFont = sResourceManager->NewLogFont(sRFF_PROP,sRFS_MAX-1);
  HeaderFont = sResourceManager->NewLogFont(sRFF_PROP,sRFS_NORMAL);
  TocFont = sResourceManager->NewLogFont(sRFF_PROP,sRFS_NORMAL);
}

sLBPage::~sLBPage()
{
  sRelease(TitleFont);
  sRelease(HeaderFont);
  sRelease(TocFont);
}

void sLBPage::SetLandscape(sBool landscape)
{
  Page.SizeX = 597/PDFZoom-Page.Border.x0-Page.Border.x1;
  Page.SizeY = 842/PDFZoom-Page.Border.y0-Page.Border.y1;
  if(landscape)
    sSwap(Page.SizeX,Page.SizeY);
}

void sLBPage::PrepX()
{
  MinX = 0;
  OptX = Page.SizeX;
  sLayoutBox *b = Childs;
  while(b)
  {
    OptX = sMax(OptX,b->OptX);
    b = b->Next;
  }
  OptX += Page.Border.x0 + Page.Border.x1 + 2*Page.GrayBorder;
}

void sLBPage::PrepY()
{
  TopY = 0;
  BotY = 0;
  sLayoutBox *b = Childs;
  while(b)
  {
    TopY += b->TopY + b->BotY;
    b = b->Next;
  }
}

void sLBPage::CalcX(sInt x0,sInt x1)
{
  x1 = x0 + OptX;
  Client.x0 = x0;
  Client.x1 = x1;

  ReqSizeX = OptX;

  x0 += Page.Border.x0 + Page.GrayBorder;
  x1 -= Page.Border.x1 + Page.GrayBorder;

  sLayoutBox *b = Childs;
  while(b)
  {
    sInt xs = b->OptX;
    if(b->FullWidth)
      xs = x1-x0;
    if(b->MinX>x1-x0)
    {
      xs = b->MinX;
      ReqSizeX = sMax(ReqSizeX,xs);
    }
    b->CalcX(x0,x0+xs);
    b = b->Next;
  }
}

void sLBPage::CalcY(sInt y0,sInt y1,sInt bl,sInt ybreak,class sLBPage *root)
{
  Client.y0 = y0;
  Client.y1 = y1;
  Baseline = bl;

  PageNum = 0;

  sLayoutBox *b = Childs;
  InnerPageRect(PageNum,PageRect);
  YCursor = PageRect.y0;
  while(b)
  {
    sInt ys = b->TopY + b->BotY;
    if((b->PageBreak && YCursor!=PageRect.y0) || (YCursor+ys>=PageRect.y1 && YCursor!=PageRect.y0 && !b->CanBreak))
    {
      PageNum++;
      InnerPageRect(PageNum,PageRect);
      YCursor = PageRect.y0;
    }
    sInt ystart = YCursor;
    YCursor += ys;
    b->Page = PageNum;
    b->CalcY(ystart,ystart+ys,ystart+b->TopY,PageRect.y1,this);
    b = b->Next;
  }
  PageCount = PageNum+1;
  ReqSizeY = PageCount*(Page.SizeY+Page.Border.y0+Page.Border.y1+Page.GrayBorder)+Page.GrayBorder;
}

void sLBPage::NextPage(sInt &y,sInt &yb)
{
  PageNum++;
  InnerPageRect(PageNum,PageRect);
  y = PageRect.y0;
  yb = PageRect.y1;
}

void sLBPage::HeaderSplit(const sChar *str,sString<1024> *out,sInt page)
{
  sString<1024> buffer;
  sString<64> num;

  for(sInt i=0;i<3;i++)
  {
    buffer[0] = 0;
    while(*str!=0 && *str!='|')
    {
      if(str[0]=='%' && str[1]!=0)
      {
        switch(str[1])
        {
        case 'p':
          num.PrintF(L"%d",page+1);
          buffer.Add(num);
          break;

        case 't':
          buffer.Add(Page.Title);
          break;

        default:
          buffer.Add(L"?");
          break;
        }
        str+=2;
      }
      else
      {
        if(str[0]=='\\' && str[1]!=0)
          str++;
        buffer.Add(*str++);
      }
    }
    if(*str=='|')
      str++;
    out[i] = buffer;
  }

}

void sLBPage::HeaderRect(sRect &r0,sRect &r1,sInt page)
{
  sRect r;
  InnerPageRect(page,r);
  r0 = r;
  r1 = r;
  sInt h = HeaderFont->Font->GetHeight();

  r0.y1 = r.y0 - h;
  r0.y0 = r0.y1 - h - 5;

  r1.y0 = r.y1 + h;
  r1.y1 = r1.y0 + h + 5;
}

void sLBPage::Paint(sLayoutWindow *lw)
{
  sRect r;

  sClipPush();
  sString<1024> mark[3];
  sRect r0,r1;
  for(sInt i=0;i<PageCount;i++)
  {
    OuterPageRect(i,r);
    sRect2D(r,MakeColor(BackColor,1));

    if(Page.Title.IsEmpty() || i!=0)
    {
      HeaderFont->Font->SetColor(sGC_BLACK,sGC_WHITE);

      HeaderRect(r0,r1,i);
      HeaderSplit(Page.Header,mark,i);
      sRect2D(r0,sGC_WHITE);
      HeaderFont->Font->Print(sF2P_LEFT |sF2P_TOP,r0,mark[0]);
      HeaderFont->Font->Print(           sF2P_TOP,r0,mark[1]);
      HeaderFont->Font->Print(sF2P_RIGHT|sF2P_TOP,r0,mark[2]);
      if(Page.HeaderRuler)
        sRect2D(r0.x0,r0.y1-1,r0.x1,r0.y1,sGC_BLACK);
      HeaderSplit(Page.Footer,mark,i);
      sRect2D(r1,sGC_WHITE);
      HeaderFont->Font->Print(sF2P_LEFT |sF2P_BOTTOM,r1,mark[0]);
      HeaderFont->Font->Print(           sF2P_BOTTOM,r1,mark[1]);
      HeaderFont->Font->Print(sF2P_RIGHT|sF2P_BOTTOM,r1,mark[2]);
      if(Page.FooterRuler)
        sRect2D(r1.x0,r1.y0,r1.x1,r1.y0+1,sGC_BLACK);

      sClipExclude(r0);
      sClipExclude(r1);
    }
    sClipExclude(r);
  }
  sRect2D(Client,sGC_BUTTON);
  sClipPop();
}

void sLBPage::MakeHtml(sTextBuffer &tb,sLBHtmlState *s)
{
  sLayoutBox::MakeHtml(tb,s);
}

void sLBPage::MakePDF(sLBPdfState *pdf)
{
  sString<1024> mark[3];
  sRect r0,r1;
  sResourceManager->ClearTemp();
  pdf->pdf->BeginDoc((Page.SizeX+Page.Border.x0+Page.Border.x1)*pdf->Zoom,(Page.SizeY+Page.Border.y0+Page.Border.y1)*pdf->Zoom);
  for(sInt i=0;i<PageCount;i++)
  {
    sRect r;
    OuterPageRect(i,r);
    pdf->CalcMatrix(i,Page,r);

    pdf->pdf->BeginPage();
    sLayoutBox::MakePDF(pdf);

    pdf->SetPrint(HeaderFont,0);

    if(Page.Title.IsEmpty() || i!=0)
    {
      HeaderRect(r0,r1,i);
      HeaderSplit(Page.Header,mark,i);
      pdf->Print(sF2P_LEFT |sF2P_TOP,r0,mark[0]);
      pdf->Print(           sF2P_TOP,r0,mark[1]);
      pdf->Print(sF2P_RIGHT|sF2P_TOP,r0,mark[2]);
      if(Page.HeaderRuler)
        pdf->Rect(r0.x0,r0.y1-1,r0.x1,r0.y1,0);

      HeaderSplit(Page.Footer,mark,i);
      pdf->Print(sF2P_LEFT |sF2P_BOTTOM,r1,mark[0]);
      pdf->Print(           sF2P_BOTTOM,r1,mark[1]);
      pdf->Print(sF2P_RIGHT|sF2P_BOTTOM,r1,mark[2]);
      if(Page.FooterRuler)
        pdf->Rect(r1.x0,r1.y0,r1.x1,r1.y0+1,0);
    }

    pdf->pdf->EndPage();
  }
  pdf->pdf->EndDoc(L"dummy.pdf");
}

void sLBPage::InnerPageRect(sInt n,sRect &r)
{
  OuterPageRect(n,r);
  r.x0 += Page.Border.x0;
  r.x1 -= Page.Border.x1;
  r.y0 += Page.Border.y0;
  r.y1 -= Page.Border.y1;
}

void sLBPage::OuterPageRect(sInt n,sRect &r)
{
  sInt h = Page.SizeY+Page.Border.y0+Page.Border.y1;
  r.y0 = Client.y0 + n*(h+Page.GrayBorder)+Page.GrayBorder;
  r.y1 = r.y0 + h;
  r.x0 = Client.x0 + Page.GrayBorder;
  r.x1 = r.x0 + Page.SizeX+Page.Border.x0+Page.Border.x1;
}

/****************************************************************************/
