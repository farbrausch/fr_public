/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_LAYOUT_MARKUP_HPP
#define FILE_LAYOUT_MARKUP_HPP

#include "base/types2.hpp"
#include "wiki/resources.hpp"
#include "wiki/layoutwindow.hpp"

class sLayoutBox;
class MarkupBlock;

/****************************************************************************/

class SpellChecker
{
  struct Word
  {
    Word *Next;
    const sChar *String;
    sInt Length;
    sInt Mode; // 0=lower, 1=first upper, 2=more upper
  };
  sInt HashSize;
  Word **HashTable;
  sBool IsLetter[256];
  sChar LowerLetter[256];

  sMemoryPool *Pool;

  sBool isletter(sChar x) { return x>=0 && x<0x100 && IsLetter[x]; }
  sU32 Hash(const sChar *s,sInt len);
public:
  SpellChecker();
  ~SpellChecker();
  sBool Check(const sChar *string,sInt len=-1);

  void Clear();
  void AddDict(const sChar *filename);
  void AddWords(const sChar *s,sInt len=-1);
  void AddWord(const sChar *word,sInt len);

  sString<16> Label;
};

/****************************************************************************/

struct MarkupFile
{
  sTextBuffer *Text;
  MarkupFile() { Text=0; }
};


class MarkupType
{
public:
  sPoolString Code;
  sBool Verbatim;        // CODE style layout
  sBool SingleLine;       // additinal lines of text will not be included in the body
  sBool ComplexCode;
  sBool Execute;
  sBool NoChilds;
  MarkupType();
  virtual ~MarkupType();
  virtual sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,class Markup *)=0;
  virtual void EndBoxes(MarkupBlock *,sLayoutBox *);
};

class MarkupWordType
{
public:
  sPoolString Code;

  MarkupWordType();
  virtual ~MarkupWordType();
  virtual void ChangeStyle(class MarkupStyle &style,const sChar *&t,sLBParagraph *par,Markup *markup);
};

sU32 GetColor(const sChar *&t);
sBool GetString(const sChar *&t,const sStringDesc &desc);
sBool GetInt(const sChar *&t,sInt &val);

/****************************************************************************/

struct MarkupText
{
  const sChar *Start;
  sInt Count;
  MarkupFile *File;

  MarkupText() { Start = 0; Count=0; File = 0; }
};

class MarkupLine
{
public:
  MarkupText Line;
  MarkupText Text;
  MarkupType *Type;
  sInt Newlines;
  sInt MarkupIndent;
  sInt ChildIndent;
};

class MarkupBlock
{
public:
  sArray<MarkupBlock *>Childs;
  MarkupType *Type;
  MarkupText Text;
  MarkupText Line;
  sInt Indent;        // indent of content, usefull for CODE style text layout.

  MarkupBlock();
  ~MarkupBlock();     // deletes childs! pure trees only!
};

class MarkupStyle
{
public:
  sU32 TextColor;
  sU32 BackColor;
  sU32 HeadColor;
  sU32 LineColor;
  sInt FontStyle;     // sF2C_ITALICS, sF2C_BOLD, sF2C_UNDERLINE, sF2C_STRIKEOUT
  sInt FontSize;      // sRFS_SMALL2 .. sRFS_NORMAL .. sRFS_BIG4
  sInt FontFamily;    // sRFF_PROP, sRFF_FIXED, sRFF_SYMBOL
  sFontResource *Font;
  sInt Verbatim;      // do not process markup
  sPoolString LeftAction;
  sPoolString RightAction;
  sPoolString MiddleAction;
  sInt AlignBlock;    // sF2P_LEFT, sF2P_RIGHT, sF2P_JUSTIFIED, 0=center
  sInt MousePointer;  // sMP_*
  SpellChecker *Dict;
  sBool ChapterFormfeed;

  MarkupStyle();
  MarkupStyle(const MarkupStyle &);
  void Init(const MarkupStyle &);
  ~MarkupStyle();
  void UpdateFont();
  void SetBox(sLayoutBox *) const;
};

/****************************************************************************/

class Markup
{
  sInt ScanRecursion;
  void Scan(MarkupFile *);
  sBool ParseR(MarkupBlock *parent,sInt &n,sInt indent);
  void GenerateR(MarkupBlock *block,sLayoutBox *box);
  sFontResource *StdFont;
  void AddTextR(const MarkupStyle &,const MarkupStyle *,sLBParagraph *,const sChar *start,const sChar *end);
  const sChar *AddTextSpace;
  sInt AddTextSpaceCount;
  sU32 AddTextBorderColor;
  const MarkupStyle *GlueStyle;

  void ChainWordBegin(const sChar *,const sChar *);
  void ChainWordEnd();
  sLBWord *ChainLastWord;
  const sChar *ChainLastChar;
  const sChar *ChainEndChar;
  MarkupFile *ChainFile;

public:
  Markup();
  ~Markup();
  void Clear();
  void Parse(MarkupFile *);
  void Dump();
  void Generate(sLayoutBox *);

  void AddText(const MarkupStyle &,sLBParagraph *,const sChar *start,const sChar *end);
  void AddText(const MarkupStyle &s,sLBParagraph *p,const sChar *start) { AddText(s,p,start,start+sGetStringLen(start)); }
  void AddText(const MarkupStyle &s,sLBParagraph *p,const MarkupText &text) { AddText(s,p,text.Start,text.Start+text.Count); }

  void AddText(sLBParagraph *p,const sChar *start,const sChar *end) { AddText(GlobalStyle,p,start,end); }
  void AddText(sLBParagraph *p,const sChar *start) { AddText(GlobalStyle,p,start,start+sGetStringLen(start)); }
  void AddText(sLBParagraph *p,const MarkupText &text) { AddText(GlobalStyle,p,text.Start,text.Start+text.Count); }

  void AddGlue(const sChar *p,sLBParagraph *par);
  void ChainWord(sLBWord *word);
  sBool Check(SpellChecker *d,const sChar *word,sInt length);

  MarkupType *PlainType;
  sArray<MarkupType *> Types;
  sArray<MarkupWordType *> WordTypes;
  sArray<MarkupLine *> Lines;
  sArray<MarkupFile *> TempFiles;
  MarkupBlock *Root;

  MarkupStyle GlobalStyle;
  MarkupStyle ChapterLeftStyle;
  MarkupStyle ChapterRightStyle;
  sBool ChapterFormfeed;
  sBool Landscape;

  const sChar *FindHeadline;
  sLayoutBox *FindHeadlineBox;
  const sChar *FindHeadlineCursor;

  // global infos

  sBool PageMode;
  sPoolString Title;
  sLayoutBox *Toc;
  sLBTable2 *TocTable;
  sInt Chapters[4];
  void StartChapters();
  void PrintChapter(const sStringDesc &desc,sInt nest);
  void AddChapterToToc(sInt nest,const sChar *number,const sChar *name,sLayoutBox *body);
  void IncrementChapter(sInt nest);
  sArray<SpellChecker *> Dicts;
  SpellChecker PageDict;

  // please set this

  void (*Script)(const sChar *code,sTextBuffer *output);
};

/****************************************************************************/

class MarkupTypePlain : public MarkupType
{
  sF32 Advance;
public:
  MarkupTypePlain() { Advance=1.0f/3; }
  MarkupTypePlain(const sChar *code,sF32 ad) { Code = code; Advance=ad; }
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeStyle : public MarkupType
{
public:
  MarkupTypeStyle(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeBullets : public MarkupType
{
public:
  MarkupTypeBullets(const sChar *code);
  ~MarkupTypeBullets();
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeHeadline : public MarkupType
{
public:
  MarkupTypeHeadline(const sChar *code);
  ~MarkupTypeHeadline();
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeTask : public MarkupType
{
public:
  MarkupTypeTask(const sChar *code);
  ~MarkupTypeTask();
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};


class MarkupTypeLink : public MarkupType
{
  const sChar *Command;
  sU32 TextColor;
public:
  MarkupTypeLink(const sChar *code,const sChar *cmd,sInt tc);
  ~MarkupTypeLink();
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeExecute : public MarkupType
{
public:
  MarkupTypeExecute(const sChar *code) { Execute=1; Verbatim=1; Code=code; }
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *) { return box; }
};


class MarkupTypeBreak : public MarkupType
{
public:
  MarkupTypeBreak(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeTitle : public MarkupType
{
public:
  MarkupTypeTitle(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeToc : public MarkupType
{
public:
  MarkupTypeToc(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeDict : public MarkupType
{
public:
  MarkupTypeDict(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeChapterFormfeed : public MarkupType
{
public:
  MarkupTypeChapterFormfeed(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

class MarkupTypeLandscape : public MarkupType
{
public:
  MarkupTypeLandscape(const sChar *code);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *box,Markup *);
};

/****************************************************************************/

class MarkupTypeTable : public MarkupType
{
  sInt Border;
public:
  MarkupTypeTable(const sChar *code,sInt border);
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};


class MarkupTypeItem : public MarkupType
{
public:
  MarkupTypeItem(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeItems : public MarkupType
{
public:
  MarkupTypeItems(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeBox : public MarkupType
{
public:
  MarkupTypeBox(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeRuler : public MarkupType
{
public:
  MarkupTypeRuler(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeImage : public MarkupType
{
public:
  MarkupTypeImage(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeEmpty : public MarkupType
{
public:
  MarkupTypeEmpty(const sChar *code);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeSvnConflict : public MarkupType
{
  sInt Mode;
public:
  MarkupTypeSvnConflict(const sChar *code,sInt mode);
  sLayoutBox *BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

class MarkupTypeCode : public MarkupType
{
  sFontResource *Font;
  sInt MinWidth;
public:
  MarkupTypeCode(const sChar *code,sInt minwidth);
  ~MarkupTypeCode();
  sLayoutBox * BeginBoxes(MarkupBlock *,sLayoutBox *,Markup *);
};

/****************************************************************************/

class MarkupWordStyle : public MarkupWordType
{
  sInt Set,Clr;
public:
  MarkupWordStyle(const sChar *code,sInt s,sInt c) { Code=code; Set=s; Clr=c; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.FontStyle|=Set; s.FontStyle&=~Clr; s.UpdateFont(); }
};

class MarkupWordSize : public MarkupWordType
{
  sInt Inc;
public:
  MarkupWordSize(const sChar *code,sInt i) { Code=code; Inc=i; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.FontSize=sClamp(s.FontSize+Inc,0,sRFS_MAX-1); s.UpdateFont(); }
};

class MarkupWordCode : public MarkupWordType
{
public:
  MarkupWordCode(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.FontFamily=sRFF_FIXED; s.UpdateFont(); s.Verbatim=1; s.Dict=0; }
};

class MarkupWordTextColor : public MarkupWordType
{
public:
  MarkupWordTextColor(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.TextColor = GetColor(t); }
};

class MarkupWordBackColor : public MarkupWordType
{
public:
  MarkupWordBackColor(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.BackColor = GetColor(t); }
};

class MarkupWordHeadColor : public MarkupWordType
{
public:
  MarkupWordHeadColor(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.HeadColor = GetColor(t); }
};

class MarkupWordLineColor : public MarkupWordType
{
public:
  MarkupWordLineColor(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.LineColor = GetColor(t); }
};

class MarkupWordFontFamily : public MarkupWordType
{
  sInt Family;
public:
  MarkupWordFontFamily(const sChar *code,sInt fam) { Code=code; Family = fam; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.FontFamily = Family; if(Family==sRFF_SYMBOL) s.FontStyle |= sF2C_SYMBOLS; s.UpdateFont(); }
};

class MarkupWordImage : public MarkupWordType
{
public:
  MarkupWordImage(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *);
};

class MarkupWordLink : public MarkupWordType
{
  const sChar *Command;
  sU32 TextColor;
public:
  MarkupWordLink(const sChar *code, const sChar *cmd,sU32 tc ) { Code=code; Command=cmd; TextColor=tc; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *);
};

class MarkupWordAlign : public MarkupWordType
{
  sInt Align;
public:
  MarkupWordAlign(const sChar *code,sInt align) { Code=code; Align=align; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *) { s.AlignBlock = Align; }
};

class MarkupWordDict : public MarkupWordType
{
public:
  MarkupWordDict(const sChar *code) { Code=code; }
  void ChangeStyle(MarkupStyle &s,const sChar *&t,sLBParagraph *,Markup *);
};

/****************************************************************************/

#endif // FILE_LAYOUT_MARKUP_HPP

