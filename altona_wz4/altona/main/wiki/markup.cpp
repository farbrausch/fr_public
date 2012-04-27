/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

//#include "main.hpp"
#include "markup.hpp"
#include "layoutwindow.hpp"

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/**************************************************************************+*/

SpellChecker::SpellChecker()
{

  HashSize = 256*1024;
  HashTable = new Word*[HashSize];
  for(sInt i=0;i<HashSize;i++)
    HashTable[i] = 0;

  Pool = new sMemoryPool(256*1024);

  sClear(IsLetter);
  for(sInt i='a' ;i<='z' ;i++)  IsLetter[i] = 1;
  for(sInt i='A' ;i<='Z' ;i++)  IsLetter[i] = 1;
  for(sInt i=0xa0;i<=0xff;i++)  IsLetter[i] = 1;
  sClear(LowerLetter);
  for(sInt i='A' ;i<='Z'; i++)  LowerLetter[i] = i+0x20;
  for(sInt i=0xc0;i<=0xdf;i++)  LowerLetter[i] = i+0x20;
  LowerLetter[0xd7] = 0;
}

SpellChecker::~SpellChecker()
{
  delete[] HashTable;
  delete Pool;
}

sBool SpellChecker::Check(const sChar *string,sInt len)
{
  if(len==-1) len = sGetStringLen(string);

  const sChar *s0 = string;
  const sChar *s1 = string+len;
  sString<256> buffer;
  sU32 hash;
  Word *w;

  while(s0<s1)
  {
    while(s0<s1 && !isletter(*s0)) s0++;
    len = 0;
    while(s0+len<s1 && isletter(s0[len]))
      len++;

    if(len>0 && len<255)
    {
      sInt mode = 0;
      for(sInt i=0;i<len;i++)
      {
        sInt c = s0[i];
        if(c<256 && LowerLetter[c])
        {
          c = LowerLetter[c];
          if(i==0)
            mode = 1;
          else 
            mode = 2;
        }
        buffer[i] = c;
      }
      if(mode==2)
        for(sInt i=0;i<len;i++)
          buffer[i] = s0[i];
      buffer[len] = 0;
      hash = Hash(buffer,len);
    
      w = HashTable[hash & (HashSize-1)];
      sBool ok = 0;
      while(w && !ok)
      {
        if(sCmpMem(buffer,w->String,len*sizeof(sChar))==0)
        {
          if(mode==w->Mode) ok = 1;
          if(mode==1 && w->Mode==0) ok = 1;
        }
        w = w->Next;
      }

      if(!ok) return 0;
    }
    s0+=len;
  }
  return 1;
}

sU32 SpellChecker::Hash(const sChar *s,sInt len)
{
  return sChecksumMurMur((const sU32 *)s,(len+1)/2);
}

/****************************************************************************/

void SpellChecker::Clear()
{
  for(sInt i=0;i<HashSize;i++)
    HashTable[i] = 0;
  Pool->Reset();
}

void SpellChecker::AddDict(const sChar *filename)
{
  const sChar *text = sLoadText(filename);
  if(text)
    AddWords(text);
  delete[] text;
}

void SpellChecker::AddWords(const sChar *s,sInt len)
{
  const sChar *e = s-1;
  if(len) 
    e = s+len;

  while((*s && s!=e) && sIsSpace(*s)) s++;
  while(*s && s!=e)
  {
    const sChar *start = s;
    while((*s && s!=e) && !sIsSpace(*s)) s++;
    sInt len = s-start;
    AddWord(start,len);
    while((*s && s!=e) && sIsSpace(*s)) s++;
  }
}

void SpellChecker::AddWord(const sChar *word,sInt len)
{
  sVERIFY(len>=0);
  sVERIFY(sizeof(sChar)==2);
  sChar *buffer = Pool->Alloc<sChar>(len+1);

  sInt mode = 0;
  for(sInt i=0;i<len;i++)
  {
    sInt c = word[i];
    if(c<256 && LowerLetter[c])
    {
      c=LowerLetter[c];
      if(i==0)
        mode = 1;
      else 
        mode = 2;
    }
    buffer[i] = c;
  }
  if(mode==2)
    for(sInt i=0;i<len;i++)
      buffer[i] = word[i];

  buffer[len] = 0;
  sU32 hash = Hash(buffer,len) & (HashSize-1);

  Word *w = Pool->Alloc<Word>(1);
  w->String = buffer;
  w->Length = len;
  w->Next = HashTable[hash];
  w->Mode = mode;
  HashTable[hash] = w;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/**************************************************************************+*/

MarkupType::MarkupType()
{
  Verbatim = 0;
  SingleLine = 0;
  ComplexCode = 0;
  Execute = 0;
  NoChilds = 0;
}

MarkupType::~MarkupType()
{
}


void MarkupType::EndBoxes(MarkupBlock *,sLayoutBox *)
{
}


/****************************************************************************/

MarkupWordType::MarkupWordType()
{
}

MarkupWordType::~MarkupWordType()
{
}

void MarkupWordType::ChangeStyle(MarkupStyle &style,const sChar *&t,sLBParagraph *par,Markup *)
{
}

sU32 GetColor(const sChar *&t)
{
  sInt val = sGC_RED;

  if(t[0]=='#' && sIsHex(t[1]))
  {
    t++;
    sScanHex(t,val);
  }

  return val;
}

sBool GetInt(const sChar *&t,sInt &val)
{
  sInt sign = 1;
  while(*t==' ') t++;

  if(t[0]=='-' && sIsDigit(t[1]))
  {
    sign=-1;
    t++;
  }
  if(sIsDigit(t[0]))
  {
    sInt v = 0;
    sScanInt(t,v);
    val = v*sign;
    return 1;
  }
  return 0;
}

sBool GetString(const sChar *&t,const sStringDesc &desc)
{
  while(*t==' ') t++;
  if(*t!='"') return 0;
  t++;
  const sChar *s = t;
  while(*t!=0 && *t!='\n' && *t!='"') t++;
  if(*t!='"') return 0;
  sCopyString(desc.Buffer,s,sMin<sInt>(desc.Size-1,t-s+1));
  t++;
  return 1;
}


/****************************************************************************/

MarkupBlock::MarkupBlock()
{
  Type = 0;
  Text.Start = 0;
  Text.Count = 0;
  Text.File = 0;
}

MarkupBlock::~MarkupBlock()
{
  sDeleteAll(Childs);
}

/****************************************************************************/

MarkupStyle::MarkupStyle()
{
  TextColor = sGC_BLACK;
  BackColor = sGC_WHITE;
  LineColor = sGC_DRAW;
  HeadColor = sGC_LTGRAY;
  FontStyle = 0;
  FontSize = sRFS_NORMAL;
  FontFamily = sRFF_PROP;
  Verbatim = 0;
  AlignBlock = sF2P_JUSTIFIED;
  MousePointer = sMP_ARROW;

  Font = 0;
  Dict = 0;
}

void MarkupStyle::Init(const MarkupStyle &src)
{
  TextColor = src.TextColor;
  BackColor = src.BackColor;
  LineColor = src.LineColor;
  HeadColor = src.HeadColor;
  FontStyle = src.FontStyle;
  FontSize = src.FontSize;
  FontFamily = src.FontFamily;
  Verbatim = src.Verbatim;
  LeftAction = src.LeftAction;
  RightAction = src.RightAction;
  MiddleAction = src.MiddleAction;
  AlignBlock = src.AlignBlock;
  MousePointer = src.MousePointer;

  Dict = src.Dict;

  if(Font)
    Font->Release();
  Font = src.Font;
  if(Font)
    Font->AddRef();
}

MarkupStyle::MarkupStyle(const MarkupStyle &src)
{
  Font = 0;
  Dict = 0;
  Init(src);
}

MarkupStyle::~MarkupStyle()
{
  if(Font)
    Font->Release();
}

void MarkupStyle::UpdateFont()
{
  if(Font)
    Font->Release();
  Font = sResourceManager->NewLogFont(FontFamily,FontSize,FontStyle);
}

void MarkupStyle::SetBox(sLayoutBox *box) const
{
  if(!LeftAction  .IsEmpty())  box->LeftAction   = LeftAction;
  if(!RightAction .IsEmpty())  box->RightAction  = RightAction;
  if(!MiddleAction.IsEmpty())  box->MiddleAction = MiddleAction;
}

/****************************************************************************/
/****************************************************************************/

Markup::Markup()
{
  Script = 0;
  Root = 0;
  StdFont = 0;
  ChapterFormfeed = 1;
  Landscape = 0;
  Clear();
  PlainType = new MarkupTypePlain();
  Types.AddTail(new MarkupTypeSvnConflict(L"<<<<<<<",1));
  Types.AddTail(new MarkupTypeSvnConflict(L"=======",2));
  Types.AddTail(new MarkupTypeSvnConflict(L">>>>>>>",3));
  Types.AddTail(new MarkupTypeBullets(L"*"));
  Types.AddTail(new MarkupTypeBullets(L"**"));
  Types.AddTail(new MarkupTypeBullets(L"***"));
  Types.AddTail(new MarkupTypeBullets(L"****"));
  Types.AddTail(new MarkupTypeHeadline(L"="));
  Types.AddTail(new MarkupTypeHeadline(L"=="));
  Types.AddTail(new MarkupTypeHeadline(L"==="));
  Types.AddTail(new MarkupTypeHeadline(L"===="));
  Types.AddTail(new MarkupTypeCode(L"%",0));
  Types.AddTail(new MarkupTypeCode(L"%%",80));
  Types.AddTail(new MarkupTypePlain(L"~~",0));
  Types.AddTail(new MarkupTypePlain(L"~",1.0f/3));
  Types.AddTail(new MarkupTypeTask(L"&"));
  Types.AddTail(new MarkupTypeLink(L">>",L"open",0xa0a000));
  Types.AddTail(new MarkupTypeLink(L">:",L"shell",sGC_RED));
  Types.AddTail(new MarkupTypeLink(L">",L"load",sGC_BLUE));
  Types.AddTail(new MarkupTypeExecute(L"$"));
  Types.AddTail(new MarkupTypeStyle(L"!"));

  Types.AddTail(new MarkupTypeTable(L"!t",0));
  Types.AddTail(new MarkupTypeTable(L"!T",1));
  Types.AddTail(new MarkupTypeItem(L"!i"));
  Types.AddTail(new MarkupTypeItems(L"!|"));
  Types.AddTail(new MarkupTypeTable(L"!I",2));
  Types.AddTail(new MarkupTypeBox(L"!b"));
  Types.AddTail(new MarkupTypeEmpty(L"!_"));
  Types.AddTail(new MarkupTypeRuler(L"----"));
  Types.AddTail(new MarkupTypeImage(L"!img"));
  Types.AddTail(new MarkupTypeBreak(L"!break"));
  Types.AddTail(new MarkupTypeTitle(L"!title"));
  Types.AddTail(new MarkupTypeToc(L"!toc"));
  Types.AddTail(new MarkupTypeDict(L"~+"));
  Types.AddTail(new MarkupTypeChapterFormfeed(L"!chapterformfeed"));
  Types.AddTail(new MarkupTypeLandscape(L"!landscape"));

  WordTypes.AddTail(new MarkupWordStyle(L"b",sF2C_BOLD,0));
  WordTypes.AddTail(new MarkupWordStyle(L"i",sF2C_ITALICS,0));
  WordTypes.AddTail(new MarkupWordStyle(L"u",sF2C_UNDERLINE,0));
  WordTypes.AddTail(new MarkupWordStyle(L"s",sF2C_STRIKEOUT,0));
  WordTypes.AddTail(new MarkupWordStyle(L"B",0,sF2C_BOLD));
  WordTypes.AddTail(new MarkupWordStyle(L"I",0,sF2C_ITALICS));
  WordTypes.AddTail(new MarkupWordStyle(L"U",0,sF2C_UNDERLINE));
  WordTypes.AddTail(new MarkupWordStyle(L"S",0,sF2C_STRIKEOUT));
  WordTypes.AddTail(new MarkupWordSize(L"+",1));
  WordTypes.AddTail(new MarkupWordSize(L"-",-1));
  WordTypes.AddTail(new MarkupWordCode(L"%"));
  WordTypes.AddTail(new MarkupWordTextColor(L"tc"));
  WordTypes.AddTail(new MarkupWordBackColor(L"bc"));
  WordTypes.AddTail(new MarkupWordHeadColor(L"hc"));
  WordTypes.AddTail(new MarkupWordLineColor(L"lc"));
  WordTypes.AddTail(new MarkupWordFontFamily(L"fixedfont",sRFF_FIXED));
  WordTypes.AddTail(new MarkupWordFontFamily(L"propfont",sRFF_PROP));
  WordTypes.AddTail(new MarkupWordFontFamily(L"symbolfont",sRFF_SYMBOL));
  WordTypes.AddTail(new MarkupWordImage(L"img"));
  WordTypes.AddTail(new MarkupWordLink(L">>",L"open",0xa0a000));
  WordTypes.AddTail(new MarkupWordLink(L">:",L"shell",sGC_RED));
  WordTypes.AddTail(new MarkupWordLink(L">",L"load",sGC_BLUE));
  WordTypes.AddTail(new MarkupWordAlign(L"left",sF2P_LEFT));
  WordTypes.AddTail(new MarkupWordAlign(L"right",sF2P_RIGHT));
  WordTypes.AddTail(new MarkupWordAlign(L"block",sF2P_JUSTIFIED));
  WordTypes.AddTail(new MarkupWordAlign(L"center",0));
  WordTypes.AddTail(new MarkupWordDict(L"~"));
  WordTypes.AddTail(new MarkupWordDict(L"~en"));
  WordTypes.AddTail(new MarkupWordDict(L"~de"));
}

Markup::~Markup()
{
  delete Root;
  delete PlainType;
  sDeleteAll(Types);
  sDeleteAll(WordTypes);
  sDeleteAll(Lines);
  MarkupFile *mf;
  sFORALL(TempFiles,mf)
    delete mf->Text;
  sDeleteAll(TempFiles);
  StdFont->Release();

  sDeleteAll(Dicts);
}

void Markup::Clear()
{
  sRelease(StdFont);
  StdFont = sResourceManager->NewLogFont(0,sRFS_NORMAL,0);
  delete Root;
  Root = new MarkupBlock();
  Root->Type = PlainType;
  MarkupFile *mf;
  sFORALL(TempFiles,mf)
    delete mf->Text;
  sDeleteAll(TempFiles);
  ScanRecursion = 0;
  MarkupStyle style;
  GlobalStyle.Init(style);
  GlobalStyle.UpdateFont();
  GlobalStyle.ChapterFormfeed = ChapterFormfeed;
  Landscape = 0;

  ChainLastWord = 0;
  ChainLastChar = 0;
  ChainEndChar = 0;

  FindHeadlineCursor = 0;
  FindHeadlineBox = 0;
  FindHeadline = 0;

  Title = L"";
  PageMode = 0;
  Toc = 0;
  TocTable = 0;
  sClear(Chapters);
  Chapters[0] = -1;
  PageDict.Clear();
}

/****************************************************************************/

void Markup::Scan(MarkupFile *text)
{
  const sChar *t = text->Text->Get();
  if(ScanRecursion>4)
    t = L"too many inlines() nested";
  while(*t)
  {
    sInt leadingspaces = 0;
    sInt newlines = 0;
    while(t[0])
    {
      while(t[0]=='\n' || t[0]=='#') 
      { 
        if(t[0]=='\n')
        {
          t++; 
          newlines++; 
        }
        else 
        {
          while(t[0]!='\n' && t[0]!=0)
            t++;
          if(t[0]=='\n')
            t++;
        }
      }
      leadingspaces = 0;
      while(t[leadingspaces]==' ') leadingspaces++;
      if(t[leadingspaces]=='\n')
        t+=leadingspaces;
      else
        break;
    }
    sInt n = leadingspaces;

    // find code

    sInt codechars = 0;
    while(t[n+codechars]>' ') codechars++;
    sPoolString code(t+n,codechars);

    // find type and build block

    MarkupLine *b = new MarkupLine;
    Lines.AddTail(b);
    b->Type = 0;
    MarkupType *type;
    sFORALL(Types,type)
    {
      if(type->ComplexCode)
      {
        if(sCmpStringLen(type->Code,code,sGetStringLen(type->Code))==0)
        {
          b->Type = type;
          break;
        }
      }
      else
      {
        if(type->Code==code)
        {
          b->Type = type;
          break;
        }
      }
    }
//    sFind(Types,&MarkupType::Code,code);
    b->Text.File = text;
    b->Newlines = newlines;
    b->MarkupIndent = leadingspaces;
    b->ChildIndent = leadingspaces;
    if(b->Type==0) 
    {
      b->Type = PlainType;
    }
    else
    {
      n+=codechars;
      leadingspaces+=codechars;
      b->ChildIndent++;
      while(t[n]==' ' || t[n]=='\t') { n++; leadingspaces++; }
    }

    // scan text

    sInt textsize = 0;
    while(t[n+textsize]!='\n' && t[n+textsize]!=0) textsize++;
    b->Text.Start = t+n;
    b->Text.Count = textsize;
    n+=textsize;

    b->Line.Start = t;
    b->Line.Count = n;
    b->Line.File = text;
    t+=n;
  }
}

void Markup::Parse(MarkupFile *text)
{
  sDeleteAll(Lines);
  Scan(text);
  sInt n = 0;
  while(n<Lines.GetCount())
  {
    if(!ParseR(Root,n,0))
      break;
  }
}

sBool Markup::ParseR(MarkupBlock *parent,sInt &n,sInt indent)
{
  MarkupLine *l = Lines[n];


  // possibly go up one level

  if(l->MarkupIndent<indent)
    return 0;

  // make this node

  if(Lines[n]->Newlines>2)
  {
    MarkupBlock *b = new MarkupBlock;
    b->Type = sFind(Types,&MarkupType::Code,L"!_");
    b->Indent = l->ChildIndent;

    parent->Childs.AddTail(b);
  }


  MarkupBlock *b = new MarkupBlock;
  b->Type = l->Type;
  b->Text = l->Text;
  b->Line = l->Line;
  b->Indent = l->ChildIndent;
  n++;

  // add more lines to this node

  while(n<Lines.GetCount() && Lines[n]->MarkupIndent>=l->ChildIndent && Lines[n]->Type==PlainType && ((Lines[n]->Newlines<2 && !l->Type->SingleLine) || l->Type->Verbatim))
  {
    b->Text.Count = Lines[n]->Text.Start + Lines[n]->Text.Count - b->Text.Start;
    n++;
  }

  // special case: execute!

  if(b->Type->Execute)
  {

    MarkupFile *mf = new MarkupFile;
    TempFiles.AddTail(mf);
    mf->Text = new sTextBuffer;
    if(Script)
      (*Script)(sPoolString(b->Text.Start,b->Text.Count),mf->Text);

    ScanRecursion++;
    sInt lastline = Lines.GetCount();
    Scan(mf);     // add new lines after lastline
    sInt m = lastline;
    sInt newend = Lines.GetCount();

    while(m<newend)
    {
      if(!ParseR(parent,m,0))
        break;
    }
    ScanRecursion--;

    for(sInt i=lastline;i<newend;i++)
      delete Lines[i];
    Lines.Resize(lastline);   // delete new lines. this should work with recursion.

    delete b;     // don't need it anymore.
  }
  else  // find childs
  {
    parent->Childs.AddTail(b);
    if(!b->Type->NoChilds)
    {
      while(n<Lines.GetCount())
      {
        if(!ParseR(b,n,l->ChildIndent))
          break;
      }
    }
  }

  // done


  return 1;
}

/****************************************************************************/

void Markup::Generate(sLayoutBox *box)
{
  ChainFile = 0;
  GenerateR(Root,box);
  ChainWordEnd();
  ChainFile = 0;
}

void Markup::GenerateR(MarkupBlock *block,sLayoutBox *box)
{
  if(block->Line.File && ChainFile!=block->Line.File)
  {
    ChainFile = block->Line.File;
    ChainWordBegin(ChainFile->Text->Get(),ChainFile->Text->Get()+ChainFile->Text->GetCount());
  }
  sLayoutBox *n = block->Type->BeginBoxes(block,box,this);
  
  MarkupStyle oldstyle;
  oldstyle.Init(GlobalStyle);
  MarkupBlock *c;
  sFORALL(block->Childs,c)
    GenerateR(c,n);
  GlobalStyle.Init(oldstyle);

  block->Type->EndBoxes(block,box);
}

/****************************************************************************/

void Markup::StartChapters()
{
  for(sInt i=0;i<sCOUNTOF(Chapters);i++)
    Chapters[i] = 0;
}

void Markup::PrintChapter(const sStringDesc &desc,sInt nest)
{
  sString<64> buffer;
  sString<64> num;
  if(Chapters[0]<0)
  {
    sCopyString(desc.Buffer,L"",desc.Size);
  }
  else
  {
    for(sInt i=0;i<=nest;i++)
    {
      if(i>0)
        buffer.Add(L".");
      num.PrintF(L"%d",Chapters[i]);
      buffer.Add(num);
    }
    buffer.Add(L" ");
    sCopyString(desc.Buffer,buffer,desc.Size);
  }
}

void Markup::AddChapterToToc(sInt nest,const sChar *number,const sChar *name,sLayoutBox *body)
{
  if(Toc && TocTable)
  {
    ChapterRightStyle.UpdateFont();

    sLBParagraph *par = new sLBParagraph();
    AddText(ChapterLeftStyle,par,number);
    TocTable->AddChild(par);

    par = new sLBParagraph();
    AddText(ChapterLeftStyle,par,name);
    TocTable->AddChild(par);

    sLBPageNumber *num = new sLBPageNumber(body,ChapterRightStyle.Font);
    num->TextColor = ChapterRightStyle.TextColor;
    num->BackColor = ChapterRightStyle.BackColor;
    TocTable->AddChild(num);

  }
}

void Markup::IncrementChapter(sInt nest)
{
  if(Chapters[0]>=0)
  {
    Chapters[nest]++;
    for(sInt i=nest+1;i<sCOUNTOF(Chapters);i++)
      Chapters[i] = 0;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Word Layout                                                        ***/
/***                                                                      ***/
/****************************************************************************/

void Markup::ChainWord(sLBWord *word)
{
  if(ChainEndChar && word->Text>=ChainLastChar && word->Text+word->Length<=ChainEndChar)
  {
    if(ChainLastWord)
    {
      sVERIFY(ChainLastWord->CursorTextStart<=word->Text);
      ChainLastWord->CursorTextEnd = word->Text;
    }

    if(ChainLastChar)
      word->CursorTextStart = ChainLastChar;
    else
      word->CursorTextStart = word->Text;
    word->CursorTextEnd = word->Text+word->Length;

    ChainLastChar = word->Text+word->Length;
    ChainLastWord = word;
  }
}

void Markup::ChainWordBegin(const sChar *start,const sChar *end)
{
  ChainLastWord = 0;
  ChainLastChar = start;
  ChainEndChar = end;
}

void Markup::ChainWordEnd()
{
  if(ChainLastWord)
    ChainLastWord->CursorTextEnd = ChainEndChar;
  ChainEndChar = 0;
  ChainLastChar = 0;
}

void Markup::AddText(const MarkupStyle &style,sLBParagraph *par,const sChar *s,const sChar *e)
{
  par->BackColor = style.BackColor;
  AddTextSpace = s;
  AddTextSpaceCount = 0;
  AddTextBorderColor = style.BackColor;
  AddTextR(style,&style,par,s,e);

  // add one last glue

  sLBGlue *glue = new sLBGlue(AddTextSpace,e-AddTextSpace+1,style.Font);
  ChainWord(glue);
  glue->BackColor = style.BackColor;
  glue->TextColor = style.TextColor;
  glue->BorderColor = style.BackColor;
  par->AddChild(glue);

  // set alignment

  par->PrintFlags &= ~(sF2P_LEFT|sF2P_RIGHT|sF2P_JUSTIFIED);
  par->PrintFlags |= style.AlignBlock;
}

void Markup::AddGlue(const sChar *n0,sLBParagraph *par)
{
  if(n0!=AddTextSpace)
  {
    sBool spaces = 0;
    for(const sChar *n=AddTextSpace;n<n0;n++)
      if(sIsSpace(*n))
        spaces++;

    sLBGlue *glue = new sLBGlue(AddTextSpace,n0-AddTextSpace,GlueStyle->Font);
    if(par->Childs)
      ChainWord(glue);
    GlueStyle->SetBox(glue);
    glue->BackColor = GlueStyle->BackColor;
    glue->TextColor = GlueStyle->TextColor;
    glue->BorderColor = AddTextBorderColor;
    if(spaces==AddTextSpaceCount) // spaces that are part of the markup do not generate glue
      glue->GlueWeight = 0;
    par->AddChild(glue);
  }
  AddTextSpace = n0;
  AddTextSpaceCount = 0;
}

void Markup::AddTextR(const MarkupStyle &style,const MarkupStyle *oldstyle,sLBParagraph *par,const sChar *s,const sChar *e)
{
  do
  {
    const sChar *ps = s;

    if(style.Verbatim)
      s = e;
    else
      while(s<e && *s!='[') s+= (s[0]=='\\' && s[1]>=32)?2:1;

    while(ps<s)
    {
      const sChar *n0 = ps;
      while(n0<s && sIsSpace(*n0)) n0+= (n0[0]=='\\' && n0[1]>=32)?2:1;
      const sChar *n1 = n0;
      while(n1<s && !sIsSpace(*n1)) n1+= (n1[0]=='\\' && n1[1]>=32)?2:1;

      if(n0<n1)
      {
        GlueStyle = oldstyle;
        AddGlue(n0,par);
        AddTextSpace = n1;
        sLBWord *word= new sLBWord(n0,n1-n0,style.Font);
        if(style.Dict) word->WrongSpelling = !Check(style.Dict,word->Text,word->Length);
        ChainWord(word);
        style.SetBox(word);
        word->BackColor = style.BackColor;
        word->TextColor = style.TextColor;
        word->MousePointer = style.MousePointer;
        par->AddChild(word);
      }
      ps = n1;
      oldstyle = &style;
    }
    oldstyle = &style;


    if(s<e)
    {
      sVERIFY(*s=='[');
      MarkupStyle newstyle(style);

      s++;
      ps = s;
      while(s<e && *s!='[' && *s!=']' && !sIsSpace(*s) && *s!=0)
      {
        s+= (s[0]=='\\' && s[1]>=32)?2:1;
      }
      sPoolString tag(ps,s-ps);

      while(sIsSpace(*s))
      {
        s++;
        AddTextSpaceCount++;
      }

      MarkupWordType *type = sFind(WordTypes,&MarkupWordType::Code,tag);
      GlueStyle = oldstyle;
      if(type)
        type->ChangeStyle(newstyle,s,par,this);

      while(sIsSpace(*s))
      {
        s++;
        AddTextSpaceCount++;
      }

      sInt indent = 1;
      ps = s;
      while(s<e && indent>0)
      {
        if(*s=='[') indent++;
        else if(*s==']') indent--;
        s+= (s[0]=='\\' && s[1]>=32)?2:1;
      }
      newstyle.UpdateFont();
      AddTextR(newstyle,&style,par,ps,s-(s[-1]==']' && s[-2]!='\\'));
    }
  }
  while(s<e);
}

/****************************************************************************/
/***                                                                      ***/
/***   Basic Types                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sLayoutBox *MarkupTypePlain::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  if(block->Text.Start)
  {
    sInt ad = sInt(markup->GlobalStyle.Font->Font->GetHeight()*Advance);
    if(box->Childs && ad>0) 
    {
      box->AddChild(new sLBSpacer(1,ad,markup->GlobalStyle.BackColor));
    }

    sLBParagraph *par = new sLBParagraph;
    markup->AddText(par,block->Text.Start,block->Text.Start+block->Text.Count);
    box->AddChild(par);
  }
  return box;
}

/****************************************************************************/

MarkupTypeStyle::MarkupTypeStyle(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeStyle::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  if(block->Text.Start)
  {
    const sChar *s = block->Text.Start;
    const sChar *e = s+block->Text.Count;

    while(s<e)
    {
      while(s<e && sIsSpace(*s)) s++;
      const sChar *sp = s;
      while(s<e && !sIsSpace(*s)) s++;
      sPoolString tag(sp,s-sp);
      while(s<e && sIsSpace(*s)) s++;
      MarkupWordType *type = sFind(markup->WordTypes,&MarkupWordType::Code,tag);
      if(type)
        type->ChangeStyle(markup->GlobalStyle,s,0,markup);
    }
  }
  return box;
}

/****************************************************************************/

MarkupTypeHeadline::MarkupTypeHeadline(const sChar *code)
{
  Code = code;
}

MarkupTypeHeadline::~MarkupTypeHeadline()
{
}

sLayoutBox *MarkupTypeHeadline::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  if(block->Text.Start)
  {
    sInt nest = sGetStringLen(Code)-1;
    markup->IncrementChapter(nest);

    const sChar *t = block->Text.Start;
    sInt n = block->Text.Count;

    while(n>0 && t[n-1]=='=')
      n--;
    while(n>0 && t[n-1]==' ')
      n--;

    sString<1024> chapter_number;
    markup->PrintChapter(chapter_number,nest);


    sLBParagraph *par = new sLBParagraph();
    MarkupStyle style(markup->GlobalStyle);
    style.BackColor = markup->GlobalStyle.HeadColor;
    style.FontSize = sRFS_BIG4-nest;
    style.AlignBlock = sF2P_LEFT;
    style.UpdateFont();
    if(!chapter_number.IsEmpty())
      markup->AddText(style,par,sPoolString(chapter_number));
    markup->AddText(style,par,block->Text.Start,block->Text.Start+n);
    if(markup->PageMode && nest==0 && markup->GlobalStyle.ChapterFormfeed)
      par->PageBreak = 1;
    else
      if(box->Childs) 
        box->AddChild(new sLBSpacer(1,style.Font->Font->GetHeight()/2,markup->GlobalStyle.BackColor));
    box->AddChild(par);

    if(markup->FindHeadline)
    {
      if(sCmpStringILen(t,markup->FindHeadline,sGetStringLen(markup->FindHeadline))==0)
      {
        markup->FindHeadlineCursor = t+sGetStringLen(markup->FindHeadline);
        markup->FindHeadlineBox = par;
        markup->FindHeadline = 0;
      }
    }

    markup->AddChapterToToc(nest,sPoolString(chapter_number),sPoolString(block->Text.Start,n),par);
  }
  return box;
}

/****************************************************************************/

MarkupTypeBullets::MarkupTypeBullets(const sChar *code)
{
  Code = code;
}

MarkupTypeBullets::~MarkupTypeBullets()
{
}

sLayoutBox *MarkupTypeBullets::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  MarkupStyle s(markup->GlobalStyle);
  s.Dict = 0;
  sLBTable2 *table = new sLBTable2(2,sLTF_MIN);
  table->SetWeight(0,0);
  table->BackColor = markup->GlobalStyle.BackColor;
  box->AddChild(table);
  sLBTable2 *boxbullet = new sLBTable2(0,sLTF_MIN);
  boxbullet->BackColor = markup->GlobalStyle.BackColor;
  for(sInt i=0;i<sGetStringLen(Code);i++)
  {
    sLBParagraph *p = new sLBParagraph(); 
    p->PrintFlags = 0;
    markup->AddText(s,p,L"\x00a0\x00a0\x00a0");
    boxbullet->AddChild(p);
  }

  sLBParagraph *p0 = new sLBParagraph(); 
  p0->PrintFlags = 0;
  markup->AddText(s,p0,L"\x2022");

  sLBParagraph *p1 = new sLBParagraph(); 
  p1->PrintFlags = 0;
  markup->AddText(s,p1,L"\x00a0");

  boxbullet->AddChild(p0);
  boxbullet->AddChild(p1);
  table->AddChild(boxbullet);
  sLBTable2 *list = new sLBTable2(1,sLTF_MIN);
  list->BackColor = markup->GlobalStyle.BackColor;
  table->AddChild(list);

  sLBParagraph *par = new sLBParagraph();
  markup->AddText(par,block->Text);
  list->AddChild(par);

  return list;
}

/****************************************************************************/
/***                                                                      ***/
/***   Task                                                               ***/
/***                                                                      ***/
/****************************************************************************/

MarkupTypeTask::MarkupTypeTask(const sChar *code)
{
  Code = code;
  SingleLine = 1;
  ComplexCode = 1;
}

MarkupTypeTask::~MarkupTypeTask()
{
}

sLayoutBox *MarkupTypeTask::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  // parse 

  const sChar *s = block->Line.Start;
  sInt indent = 0;
  sU32 color = sGC_WHITE;
  const sChar *text;
  sBool hide = 0;
  sBool percent = 0;

  while(*s==' ') s++;
  while(*s=='&')
  {
    s++;
    indent++;
  }
  sBool parsenum = 1;
  switch(*s++)
  {
  default:
    parsenum = 0;
  case '_':
    text = L" ";
    break;
  case '.':
    text = L"planned";
    break;
  case '-':
    color = sGC_RED;
    text = L"blocked";
    break;
  case 'o':
    color = sGC_YELLOW;
    text = L"working";
    break;
  case '*':
    color = sGC_GREEN;
    text = L"done";
    break;
  case '!':
    color = 0xffff7000;
    text = L"urgent";
    break;
  case '>':
    color = 0xffbfff00;
    text = L"next";
    break;
  case '/':
    text = L"hide";
    hide = 1;
    break;
  case '#':
    color = sGC_GREEN;
    text = L"clear";
    break;
  case '?':
    color = 0xfffa0a0;
    text = L"unclear";
    break;

  case '0':  case '1':  case '2':  case '3':  case '4':
  case '5':  case '6':  case '7':  case '8':  case '9':
    color = sGC_YELLOW;
    text = L"working";
    percent = 1;
    s--;
    break;
  }

  if(parsenum && sIsDigit(*s))
  {
    sInt n;
    sScanInt(s,n);
    sString<64> buffer;
    buffer.PrintF(L"%d",n);
    if(percent)
      buffer.Add(L"%");
    text = sPoolString(buffer);
  }
  
  // make boxes

  if(hide)
    return box;

  sLBTable2 *table = new sLBTable2(2,sLTF_FULL);
  table->SetWeight(0,5);
  table->SetWeight(1,1);

  sLBParagraph *p0 = new sLBParagraph();
  markup->AddText(p0,block->Text);

  sLBParagraph *p1 = new sLBParagraph();
  MarkupStyle statusstyle(markup->GlobalStyle);
  statusstyle.BackColor = color;
  markup->AddText(statusstyle,p1,text);

  p1->PrintFlags = 0;
  table->AddChild(p0);
  table->AddChild(p1);
  table->InnerBorder = 1;
  table->BorderColor = markup->GlobalStyle.LineColor;

  if(block->Childs.GetCount()>0)
  {
    sLBTable2 *list = new sLBTable2(1,sLTF_OPT);
    list->OuterBorder = 1;
    list->InnerBorder = 1;
    list->BorderColor = markup->GlobalStyle.LineColor;

    sLBTable2 *payload = new sLBTable2(1,sLTF_OPT);
    list->AddChild(table);
    list->AddChild(payload);  

    sLBBorder *border = new sLBBorder(4,markup->GlobalStyle.BackColor,list);
    border->Extend.x0 = indent*4;
    box->AddChild(border);

    return payload;
  }
  else
  {
    table->OuterBorder = 1;
    table->BorderColor = markup->GlobalStyle.LineColor;
    sLBBorder *border = new sLBBorder(4,markup->GlobalStyle.BackColor,table);
    border->Extend.x0 += (indent-1)*8;
    box->AddChild(border);

    return box;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Link                                                               ***/
/***                                                                      ***/
/****************************************************************************/

MarkupTypeLink::MarkupTypeLink(const sChar *code,const sChar *cmd,sInt tc)
{
  Code = code;
  Command = cmd;
  ComplexCode = 1;
  TextColor = tc;
}

MarkupTypeLink::~MarkupTypeLink()
{
}

sLayoutBox *MarkupTypeLink::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  const sChar *s = block->Line.Start;
  const sChar *e = s+block->Line.Count;
  const sChar *pathstart = 0;
  const sChar *pathend = 0;

  while(*s==' ') *s++;
  s+=sGetStringLen(Code);
  while(sIsSpace(*s)) s++;
  if(*s=='\"')
  {
    s++;
    pathstart = s;
    while(s<e && *s!='\"') s++;
    pathend = s;        
    if(*s=='\"') s++;
  }
  else
  {
    pathstart = s;
    while(s<e && *s!=':') s++;
    pathend = s;
  }
  if(*s==':') s++;

  sLBTable2 *table = new sLBTable2(2,sLTF_MIN);
  table->SetWeight(0,0);
  box->AddChild(table);
  sLBParagraph *p0 = new sLBParagraph();
  markup->AddText(p0,L">\x00a0");
  p0->PrintFlags = 0;
  table->AddChild(p0);
  sLBTable2 *list = new sLBTable2(1,sLTF_OPT);
  table->AddChild(list);
  list->BackColor = markup->GlobalStyle.BackColor;

  sString<512> buffer;
  sString<512> buffer2;
  buffer = Command;
  buffer.Add(L"(\"");
  buffer.Add(pathstart,pathend-pathstart);
  buffer.Add(L"\");");
  buffer2.PrintF(L"pushtab(); newtab(); %s poptab();",buffer);

  MarkupStyle style(markup->GlobalStyle);
  style.TextColor = TextColor;
  style.FontStyle |= sF2C_UNDERLINE;
  style.UpdateFont();
  style.MousePointer = sMP_HAND;
  style.LeftAction = buffer;
  style.MiddleAction = buffer2;

  sLBParagraph *par = new sLBParagraph;
  if(e>s)
    markup->AddText(style,par,s,e);
  list->AddChild(par);

  return list;
}

/****************************************************************************/
/***                                                                      ***/
/***   Tables and Blocks                                                  ***/
/***                                                                      ***/
/****************************************************************************/

MarkupTypeTable::MarkupTypeTable(const sChar *code,sInt border)
{
  Code = code;
  Border = border;
  SingleLine = 1;
}

sLayoutBox * MarkupTypeTable::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sInt xs = 0;
  sInt mode = sLTF_FULL;
  const sChar *s = block->Text.Start;
  const sChar *e = s + block->Text.Count;

  while(s<e && *s==' ') s++;
  while(s<e && sIsDigit(*s))
  {
    xs = xs*10;
    xs += *s - '0';
    s++;
  }
  while(s<e && *s==' ') s++;
  if(sCmpStringLen(s,L"min",3)==0)
  {
    s+=3;
    mode = sLTF_MIN;
  }
  else if(sCmpStringLen(s,L"opt",3)==0)
  {
    s+=3;
    mode = sLTF_OPT;
  }
  else if(sCmpStringLen(s,L"full",4)==0)
  {
    s+=4;
    mode = sLTF_FULL;
  }
  while(s<e && *s==' ') s++;

  sLBTable2 *table = new sLBTable2(xs,mode);
  sInt around = 0;
  switch(Border)
  {
  case 0:
    table->InnerBorder = 0;
    table->OuterBorder = 0;
    around = 4;
    break;
  case 1:
    table->InnerBorder = 1;
    table->OuterBorder = 1;
    around = 4;
    break;
  case 2:
    table->InnerBorder = 1;
    table->OuterBorder = 0;
    around = 0;
    break;
  }
  table->BackColor = markup->GlobalStyle.BackColor;
  table->BorderColor = markup->GlobalStyle.LineColor;
  box->AddChild(new sLBBorder(around,markup->GlobalStyle.BackColor,table));

  if(*s==':')
  {
    s++;
    sInt n = 0;
    while(n<xs)
    {
      sInt val;
      if(!GetInt(s,val))
        break;
      table->SetWeight(n,val);
      n++;
    }
  }

  return table;
}

/****************************************************************************/

MarkupTypeItem::MarkupTypeItem(const sChar *code)
{
  Code = code;
}

sLayoutBox * MarkupTypeItem::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBTable2 *table = new sLBTable2(1);
  table->ClickString = block->Text.Start;
  table->BackColor = markup->GlobalStyle.BackColor;
  box->AddChild(table);

  if(block->Text.Count>0) 
  {
    sLBParagraph *par = new sLBParagraph();
    markup->AddText(par,block->Text);
    table->AddChild(par);
  }

  return table;
}

/****************************************************************************/

MarkupTypeItems::MarkupTypeItems(const sChar *code)
{
  Code = code;
}

sLayoutBox * MarkupTypeItems::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  const sChar *str = block->Text.Start;
  const sChar *end = str+block->Text.Count;

  while(str<end)
  {
    while(sIsSpace(*str) && str<end) str++;
    sInt n = 0;
    while(str+n<end && str[n]!='|') n++;
    sInt m = n;
    while(m>0 && sIsSpace(str[m-1])) m--;
//    if(m>0)
    {
      sLBTable2 *table = new sLBTable2(1);
      table->ClickString = str;
      table->BackColor = markup->GlobalStyle.BackColor;
      box->AddChild(table);
      sLBParagraph *par = new sLBParagraph();
      markup->AddText(par,str,str+m);
      table->AddChild(par);
    }
    str += n;
    if(*str=='|')
      str++;
  }

  if(box->Kind==sLBK_TABLE2)
  {
    sLBTable2 *boxt = (sLBTable2 *)box;
    if(boxt->Columns>0)
    {
      sInt n = boxt->GetChildCount();
      while(n % boxt->Columns)
      {
        sLBTable2 *table = new sLBTable2(1);
        table->ClickString = str;
        table->BackColor = markup->GlobalStyle.BackColor;
        box->AddChild(table);
/*
        sLBParagraph *par = new sLBParagraph();
        markup->AddText(par,str,str);
        table->AddChild(par);
        */
        n++;
      }
    }
  }

  return box;
}

/****************************************************************************/

MarkupTypeBox::MarkupTypeBox(const sChar *code)
{
  Code = code;
}

sLayoutBox * MarkupTypeBox::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBTable2 *table = new sLBTable2(1);
  sLBBorder *b0 = new sLBBorder(1,markup->GlobalStyle.LineColor,table);
  sLBBorder *b1 = new sLBBorder(4,markup->GlobalStyle.BackColor,b0);
  box->AddChild(b1);
  table->BackColor = markup->GlobalStyle.BackColor;
  sLBParagraph *par = new sLBParagraph();
  markup->AddText(par,block->Text);
  table->AddChild(par);

  return table;
}


/****************************************************************************/

MarkupTypeRuler::MarkupTypeRuler(const sChar *code)
{
  Code = code;
  NoChilds = 1;
  SingleLine = 1;
}

sLayoutBox * MarkupTypeRuler::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBBorder *b0 = new sLBBorder(1,markup->GlobalStyle.HeadColor,0);
  b0->Extend.Init(0,1,0,1);
  sLBBorder *b1 = new sLBBorder(4,markup->GlobalStyle.BackColor,b0);
  b1->Extend.Init(0,4,0,4);
  box->AddChild(b1);

  return box;
}

/****************************************************************************/

MarkupTypeImage::MarkupTypeImage(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeImage::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sString<256> name;
  sInt hasscale = 0;
  sInt scale = 0;
  sInt percent = 0;

  const sChar *t = block->Text.Start;

  if(GetInt(t,scale))
  {
    hasscale = 1;
    if(t[0]=='%')
    {
      t++;
      percent = 1;
    }
  }

  if(GetString(t,name))
  {
    // put image

    sLBImage *img = new sLBImage(name);
    img->BackColor = markup->GlobalStyle.BackColor;

    if(hasscale)
    {
      if(scale==0)
      {
        img->ScaledX = 0;
        img->ScaledY = 0;
      }
      else if(percent)
      {
        img->ScaledX = sMax(img->ScaledX*scale/100,1);
        img->ScaledY = sMax(img->ScaledY*scale/100,1);
      }
      else
      {
        img->ScaledY = sMax(scale*img->ScaledY/img->ScaledX,1);
        img->ScaledX = sMax(scale,1);
      }
    }

    while(*t)
    {
      while(*t==' ') t++;
      if(sCmpStringLen(t,L"left",4)==0)
      {
        img->Align |= 1;
        t+=4;
      }
      else if(sCmpStringLen(t,L"right",5)==0)
      {
        img->Align |= 2;
        t+=5;
      }
      else if(sCmpStringLen(t,L"top",3)==0)
      {
        img->Align |= 4;
        t+=3;
      }
      else if(sCmpStringLen(t,L"bottom",6)==0)
      {
        img->Align |= 8;
        t+=6;
      }
      else if(sCmpStringLen(t,L"center",6)==0)
      {
        img->Align |= 16;
        t+=6;
      }
      else
      {
        break;
      }
    }
    box->AddChild(img);
  }
  return box;
}

/****************************************************************************/

MarkupTypeEmpty::MarkupTypeEmpty(const sChar *code)
{
  Code = code;
  NoChilds = 1;
  SingleLine = 1;
}

sLayoutBox * MarkupTypeEmpty::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBSpacer *space = new sLBSpacer(1,markup->GlobalStyle.Font->Font->GetHeight(),markup->GlobalStyle.BackColor); box->AddChild(space);

  return box;
}

/****************************************************************************/

MarkupTypeSvnConflict::MarkupTypeSvnConflict(const sChar *code,sInt mode)
{
  Mode = mode;
  Code = code;
  NoChilds = 1;
  SingleLine = 1;
}

sLayoutBox * MarkupTypeSvnConflict::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBSpacer *space;
  switch(Mode)
  {
  case 1:
    markup->GlobalStyle.BackColor = sGC_GREEN;
    markup->GlobalStyle.HeadColor = sGC_GREEN;
    space = new sLBSpacer(1,markup->GlobalStyle.Font->Font->GetHeight(),markup->GlobalStyle.BackColor); box->AddChild(space);
    break;
  case 2:
    space = new sLBSpacer(1,markup->GlobalStyle.Font->Font->GetHeight(),markup->GlobalStyle.BackColor); box->AddChild(space);
    markup->GlobalStyle.BackColor = sGC_RED;
    markup->GlobalStyle.HeadColor = sGC_RED;
    space = new sLBSpacer(1,markup->GlobalStyle.Font->Font->GetHeight(),markup->GlobalStyle.BackColor); box->AddChild(space);
    break;
  case 3:
    space = new sLBSpacer(1,markup->GlobalStyle.Font->Font->GetHeight(),markup->GlobalStyle.BackColor); box->AddChild(space);
    markup->GlobalStyle.BackColor = sGC_WHITE;
    markup->GlobalStyle.HeadColor = sGC_LTGRAY;
    break;
  }

  return box;
}

/****************************************************************************/


MarkupTypeCode::MarkupTypeCode(const sChar *code,sInt minwidth)
{
  Code = code;
  Verbatim = 1;
  MinWidth = minwidth;
}

MarkupTypeCode::~MarkupTypeCode()
{
}

sLayoutBox *MarkupTypeCode::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBTable2 *table = new sLBTable2(1);
  table->BackColor = markup->GlobalStyle.BackColor;
  sLBBorder *b0 = new sLBBorder(1,sGC_DRAW,table);
  sLBBorder *b1 = new sLBBorder(4,markup->GlobalStyle.BackColor,b0);
  b1->FullWidth = 0;

  MarkupStyle style(markup->GlobalStyle);
  style.FontFamily = sRFF_FIXED;
  style.BackColor = sGC_LTGRAY;
  style.UpdateFont();

  table->BackColor = style.BackColor;
  table->AddChild(new sLBSpacer(b1->OptX = (MinWidth+1)*style.Font->Font->GetWidth(L" "),0));

  sInt indent = block->Text.Start - block->Line.Start;
  if(block->Text.Start)
  {
    const sChar *t = block->Text.Start;
    const sChar *e = t+block->Text.Count;
    while(t<e)
    {
      sInt n = 0;
      while(t[n]!='\n' && t+n<e) n++;      
      sLBParagraph *par = new sLBParagraph();
      sLBWord *word = new sLBWord(t,n,style.Font); 
      if(style.Dict) word->WrongSpelling = !markup->Check(style.Dict,word->Text,word->Length);
      markup->ChainWord(word);
      par->BackColor = style.BackColor;
      par->AddChild(word);
      word->BackColor = style.BackColor;
      word->TextColor = style.TextColor;
      style.SetBox(word);

      table->AddChild(par);
      t+=n;
      const sChar *gluestart = t;
      if(t<e)
      {
        t++;
        sInt i=0;
        while(i<indent && t<e && *t==' ')
        {
          t++;
          i++;
        }
      }
      sLBGlue *glue = new sLBGlue(gluestart,t-gluestart,style.Font);
      markup->ChainWord(glue);
      glue->BackColor = style.BackColor;
      glue->BorderColor = style.BackColor;
      style.SetBox(glue);
      par->AddChild(glue);
    }
  }
  sLBTable2 *otable = new sLBTable2(2,sLTF_MIN);
  otable->SetWeight(0,0);
  otable->BackColor = markup->GlobalStyle.BackColor;
  sLBSpacer *space = new sLBSpacer(1,1,markup->GlobalStyle.BackColor);
  space->OptX = 0;
  otable->AddChild(b1);
  otable->AddChild(space);
  box->AddChild(otable);

  return table;
}

sBool Markup::Check(SpellChecker *d,const sChar *word,sInt length)
{
  if(d->Check(word,length)) return 1;
  if(PageDict.Check(word,length)) return 1;
  return 0;
}


/****************************************************************************/
/***                                                                      ***/
/***   Book Layout                                                        ***/
/***                                                                      ***/
/****************************************************************************/

MarkupTypeBreak::MarkupTypeBreak(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeBreak::BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *)
{
  sLBSpacer *space = new sLBSpacer(0,0);
  space->PageBreak = 1;
  box->AddChild(space);

  return box;
}

/****************************************************************************/

MarkupTypeTitle::MarkupTypeTitle(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeTitle::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  sLBBorder *b;
  sLBParagraph *p;

  markup->Title = sPoolString(block->Text.Start,block->Text.Count);

  MarkupStyle style(markup->GlobalStyle);
  style.FontSize = sRFS_MAX-1;
  style.AlignBlock = 0;
  style.UpdateFont();

  if(markup->PageMode)
    box->AddChild(new sLBSpacer(1,100));
  else
    box->AddChild(new sLBSpacer(1,4));

  b = new sLBBorder(1,markup->GlobalStyle.LineColor,0);
  b->Extend.Init(0,1,0,1);
  box->AddChild(b);

  p = new sLBParagraph;
  markup->AddText(style,p,block->Text);
  box->AddChild(p);

  b = new sLBBorder(1,markup->GlobalStyle.LineColor,0);
  b->Extend.Init(0,1,0,1);
  box->AddChild(b);

  return box;
}

/****************************************************************************/

MarkupTypeToc::MarkupTypeToc(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeToc::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  if(!markup->Toc)
  {
    markup->ChapterLeftStyle.Init(markup->GlobalStyle);
    markup->ChapterRightStyle.Init(markup->GlobalStyle);
    markup->ChapterRightStyle.AlignBlock = sF2P_RIGHT;
    markup->StartChapters();
    markup->Toc = new sLBTable2(1);

    markup->TocTable = new sLBTable2(3);
    markup->TocTable->SetWeight(0,0);
    markup->Toc->AddChild(markup->TocTable);

    box->AddChild(markup->Toc);
  }

  return box;
}

/****************************************************************************/

MarkupTypeDict::MarkupTypeDict(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeDict::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  markup->PageDict.AddWords(block->Text.Start,block->Text.Count);

  return box;
}

/****************************************************************************/

MarkupTypeChapterFormfeed::MarkupTypeChapterFormfeed(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeChapterFormfeed::BeginBoxes(MarkupBlock *block,sLayoutBox *box,Markup *markup)
{
  const sChar *str = block->Text.Start;
  const sChar *end = block->Text.Start+block->Text.Count;
  while(str<end && *str==' ') str++;
  if(str<end && *str=='0') markup->GlobalStyle.ChapterFormfeed = 0;
  else markup->GlobalStyle.ChapterFormfeed = 1;
  return box;
}

/****************************************************************************/

MarkupTypeLandscape::MarkupTypeLandscape(const sChar *code)
{
  Code = code;
}

sLayoutBox *MarkupTypeLandscape::BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *markup)
{
  markup->Landscape = 1;
  return box;
}

/****************************************************************************/
/***                                                                      ***/
/***   Misc                                                               ***/
/***                                                                      ***/
/****************************************************************************/

void MarkupWordImage::ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *par,Markup *markup) 
{
  sString<256> name;
  sInt hasscale = 0;
  sInt scale = 0;
  sInt percent = 0;

  if(GetInt(t,scale))
  {
    hasscale = 1;
    if(t[0]=='%')
    {
      t++;
      percent = 1;
    }
  }

  if(GetString(t,name) && par)
  {
    markup->AddGlue(t,par);

    // put image

    sLBImage *img = new sLBImage(name);
    img->BackColor = s.BackColor;

    if(hasscale)
    {
      if(scale==0)
      {
        img->ScaledX = 0;
        img->ScaledY = 0;
      }
      else if(percent)
      {
        img->ScaledX = sMax(img->ScaledX*scale/100,1);
        img->ScaledY = sMax(img->ScaledY*scale/100,1);
      }
      else
      {
        img->ScaledY = sMax(scale*img->ScaledY/img->ScaledX,1);
        img->ScaledX = sMax(scale,1);
      }
    }

    par->AddChild(img);
  }
}
/*
void MarkupWordLink::ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *par,Markup *markup) 
{
  sString<512> name;
  const sChar *t0;
  const sChar *t1;

  if(GetString(t,name) && par)
  {
    sString<512> buffer;
    buffer.PrintF(L"%s(\"%s\");",Command,name);
    s.LeftAction = buffer;
    s.FontStyle |= sF2C_UNDERLINE;
    s.TextColor = TextColor;
  }
  else
  {
    const sChar *text = t;
    while(*text==' ') text++;
    const sChar *t0 = text;
    while(sIsDigit(*text) || sIsLetter(*text)) text++;
    const sChar *t1 = text;
    if(t0<t1)
    {
      sString<512> name;
      name.Add(t0,t1-t0);
      sString<512> buffer;
      buffer.PrintF(L"%s(\"%s\");",Command,name);
      s.LeftAction = buffer;
      s.FontStyle |= sF2C_UNDERLINE;
      s.TextColor = TextColor;
    }
  }
}
*/

void MarkupWordLink::ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *par,Markup *markup) 
{
  sString<512> name;

  if(par)
  {
    if(!GetString(t,name))
    {
      const sChar *text = t;
      while(*text==' ') text++;
      const sChar *t0 = text;
      while(sIsDigit(*text) || sIsLetter(*text)) text++;
      const sChar *t1 = text;
      if(t0<t1)
        name.Add(t0,t1-t0);
      else
        return;
    }

    sString<512> buffer;
    sString<512> buffer2;
    buffer.PrintF(L"%s(\"%s\");",Command,name);
    buffer2.PrintF(L"pushtab(); newtab(); %s poptab();",buffer);

    s.LeftAction = buffer;
    s.MiddleAction = buffer2;
    s.FontStyle |= sF2C_UNDERLINE;
    s.TextColor = TextColor;
    s.MousePointer = sMP_HAND;
  }
}


void MarkupWordDict::ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *m)
{
  sString<16> label;
  sString<256> filename;

  if(Code[1]==0 || Code[2]==0)
  {
    s.Dict = 0;
    return;
  }

  label[0] = Code[1];
  label[1] = Code[2];
  label[2] = 0;

  SpellChecker *d;
  sFORALL(m->Dicts,d)
  {
    if(sCmpStringI(d->Label,label)==0)
    {
      s.Dict = d;
      return;
    }
  }

  d = new SpellChecker;
  d->Label = label;

  filename.PrintF(L"spelldict/dict_%s.txt",label);
  d->AddDict(filename);

  filename.PrintF(L"spelldict/user_%s.txt",label);
  d->AddDict(filename);

  filename.PrintF(L"spelldict/user.txt",label);
  d->AddDict(filename);

  m->Dicts.AddTail(d);
  s.Dict = d;
}
