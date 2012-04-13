// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "doc.hpp"
#include "_gui.hpp"
#include "win_para.hpp"

/****************************************************************************/

void GenPara::Init()
{
  sSetMem(this,0,sizeof(*this));
  Count = 1;
}

GenPara GenPara::MakeLabel(const sChar *name)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_LABEL;
  gp.Name = name;
  gp.Flags = GPF_LABEL;

  return gp;
}

GenPara GenPara::MakeChoice(sInt id,const sChar *name,const sChar *choices,sInt def,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_CHOICE;
  gp.ParaId = id;
  gp.Name = name;
  gp.Choices = choices;
  gp.Flags = flags;
  gp.Default[0] = def;

  return gp;
}

GenPara GenPara::MakeFlags(sInt id,const sChar *name,const sChar *choices,sInt def,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_FLAGS;
  gp.ParaId = id;
  gp.Name = name;
  gp.Choices = choices;
  gp.Flags = flags;
  gp.Default[0] = def;

  return gp;
}

GenPara GenPara::MakeInt(sInt id,const sChar *name,sInt max,sInt min,sInt def,sF32 step,sInt flags)
{
  return MakeIntV(id,name,1,max,min,def,step,flags);
}

GenPara GenPara::MakeIntV(sInt id,const sChar *name,sInt count,sInt max,sInt min,sInt def,sF32 step,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_INT;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = count;
  gp.Step = step;
  for(sInt i=0;i<4;i++)
  {
    gp.Min[i] = min;
    gp.Max[i] = max;
    gp.Default[i] = def;
  }
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeFloat(sInt id,const sChar *name,sInt max,sInt min,sInt def,sF32 step,sInt flags)
{
  return MakeFloatV(id,name,1,max,min,def,step,flags);
}

GenPara GenPara::MakeFloatV(sInt id,const sChar *name,sInt count,sInt max,sInt min,sF32 def,sF32 step,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_FLOAT;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = count;
  gp.Step = step;
  for(sInt i=0;i<4;i++)
  {
    gp.Min[i] = min;
    gp.Max[i] = max;
    gp.Default[i] = def;
  }
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeColor(sInt id,const sChar *name,sU32 def,sInt flags)
{
  GenPara gp;
  static sInt shift[4] = { 16,8,0,24 };

  gp.Init();
  gp.Type = GPT_COLOR;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = 4;
  gp.Step = 0.25f;
  for(sInt i=0;i<4;i++)
  {
    gp.Min[i] = 0;
    gp.Max[i] = 1;
    gp.Default[i] = ((def>>(shift[i]))&0xff)/255.0f;
  }
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeString(sInt id,const sChar *name,sInt size,const sChar *def,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_STRING;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = size;
  gp.Choices = def;
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeFilename(sInt id,const sChar *name,const sChar *def,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_FILENAME;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = 512;
  gp.Choices = def;
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeLink(sInt id,const sChar *name,class GenType *type,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_LINK;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = 32;
  gp.Choices = "";
  gp.OutputType = type;
  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeContrast(sInt id,const sChar *name,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_CONTRAST;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = 4;
  gp.Step = 0.01f;

  gp.Min[0] = -1; gp.Max[0] =  1; gp.Default[0] =  0;
  gp.Min[1] =  0; gp.Max[1] = 16; gp.Default[1] =  1;
  gp.Min[2] =  0; gp.Max[2] = 16; gp.Default[2] =  1;
  gp.Min[3] =  0; gp.Max[3] = 16; gp.Default[3] =  1;

  gp.Flags = flags;

  return gp;
}

GenPara GenPara::MakeText(sInt id,const sChar *name,const sChar *def,sInt flags)
{
  GenPara gp;

  gp.Init();
  gp.Type = GPT_TEXT;
  gp.ParaId = id;
  gp.Name = name;
  gp.Count = 1024;
  gp.Choices = def;
  gp.Flags = flags;

  return gp;
}


/****************************************************************************/

GenValue *Werkkzeug3Textures::CreateValue(GenPara *gp)
{
  GenValue *gv=0;
  switch(gp->Type)
  {
  case GPT_LABEL:
    return 0;

  case GPT_CHOICE:   gv = new GenValueInt;      break;
  case GPT_FLAGS:    gv = new GenValueInt;      break;
  case GPT_INT:      gv = new GenValueIntV;     break;
  case GPT_FLOAT:    gv = new GenValueFloatV;   break;
  case GPT_COLOR:    gv = new GenValueColor;    break;
  case GPT_STRING:   gv = new GenValueString;   break;
  case GPT_FILENAME: gv = new GenValueFilename; break;
  case GPT_LINK:     gv = new GenValueLink;     break;
  case GPT_CONTRAST: gv = new GenValueContrast; break;
  case GPT_TEXT:     gv = new GenValueText;     break;

  default:
    sFatal("unknown GenPara::Type %d",gp->Type);
    break;
  }
  if(gv)
  {
    gv->Para = gp;
    gv->ParaId = gp->ParaId;
  }

  return gv;
}

/****************************************************************************/
/****************************************************************************/

GenValue::GenValue()
{
  Para = 0;
  ParaId = 0;
}

GenValue::~GenValue()
{
}

/****************************************************************************/

// sInt Value;

GenValue *GenValueInt::Copy()
{
  GenValueInt *val;
  val = new GenValueInt;
  val->Para = Para;
  val->ParaId = ParaId;

  val->Value = Value;

  return val;
}

void GenValueInt::Default(GenPara *gp)
{
  Value = gp->Default[0];
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueInt::Read(GenPara *gp,sU32 *&data)
{
  Value = *data++;
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueInt::Write(sU32 *&data)
{
  *data++ = Value;
}

void GenValueInt::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  sControlTemplate temp;

  con = new sControl;
  switch(Para->Type)
  {
  case GPT_CHOICE:
    con->EditChoice(CMD_PARA_CHANGE,&Value,0,Para->Choices);
    con->LayoutInfo.Init(x0,y,x1,y+1);
    grid->AddChild(con);
    break;
  case GPT_FLAGS:
    temp.Init();
    temp.Type = sCT_CYCLE;
    temp.Cycle = Para->Choices;
    temp.YPos = y;
    temp.XPos = x0-2;
    temp.AddFlags(grid,CMD_PARA_CHANGE,&Value,0);
    break;
  default:
    con->EditInt(CMD_PARA_CHANGE,&Value,0);
    con->LayoutInfo.Init(x0,y,x1,y+1);
    grid->AddChild(con);
    break;
  }
}

void GenValueInt::StorePara(GenOp *,sInt *d) 
{ 
  d[0] = Value; 
}

/****************************************************************************/

GenValue *GenValueIntV::Copy()
{
  GenValueIntV *val;
  val = new GenValueIntV;
  val->Para = Para;
  val->ParaId = ParaId;

  for(sInt i=0;i<4;i++)
    val->Values[i] = Values[i];

  return val;
}

void GenValueIntV::Default(GenPara *gp)
{
  for(sInt i=0;i<4;i++)
    Values[i] = gp->Default[i];
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueIntV::Read(GenPara *gp,sU32 *&data)
{
  sInt max = *data++;
  sVERIFY(max>=1 && max<=4);
  sVERIFY(max==gp->Count);
  for(sInt i=0;i<max;i++)
    Values[i] = *data++;
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueIntV::Write(sU32 *&data)
{
  *data++ = Para->Count;
  for(sInt i=0;i<Para->Count;i++)
    *data++ = Values[i];
}

void GenValueIntV::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditInt(CMD_PARA_CHANGE,Values,0);
  con->InitNum(Para->Min,Para->Max,Para->Step,Para->Default,Para->Count);
  con->Style |= sCS_ZONES;
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);
}

void GenValueIntV::StorePara(GenOp *,sInt *d) 
{ 
  for(sInt i=0;i<Para->Count;i++) 
    d[i] = Values[i]; 
}

/****************************************************************************/

GenValue *GenValueFloatV::Copy()
{
  GenValueFloatV *val;
  val = new GenValueFloatV;
  val->Para = Para;
  val->ParaId = ParaId;

  for(sInt i=0;i<4;i++)
    val->Values[i] = Values[i];

  return val;
}

void GenValueFloatV::Default(GenPara *gp)
{
  for(sInt i=0;i<4;i++)
    Values[i] = gp->Default[i];
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueFloatV::Read(GenPara *gp,sU32 *&data)
{
  sInt max = *data++;
  sVERIFY(max>=1 && max<=4);
  sVERIFY(max==gp->Count);
  for(sInt i=0;i<max;i++)
  {
    Values[i] = *(sF32 *)data;
    data++;
  }
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueFloatV::Write(sU32 *&data)
{
  *data++ = Para->Count;
  for(sInt i=0;i<Para->Count;i++)
  {
    *(sF32 *)data = Values[i];
    data++;
  }
}

void GenValueFloatV::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditFloat(CMD_PARA_CHANGE,Values,0);
  con->InitNum(Para->Min,Para->Max,Para->Step,Para->Default,Para->Count);
  con->Style |= sCS_ZONES;
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);
}

void GenValueFloatV::StorePara(GenOp *,sInt *d) 
{ 
  for(sInt i=0;i<Para->Count;i++) 
    d[i] = (sInt)(Values[i]*0x8000); 
}

/****************************************************************************/

GenValue *GenValueColor::Copy()
{
  GenValueColor *val;
  val = new GenValueColor;
  val->Para = Para;
  val->ParaId = ParaId;

  for(sInt i=0;i<4;i++)
    val->Values[i] = Values[i];

  return val;
}

void GenValueColor::Default(GenPara *gp)
{
  for(sInt i=0;i<4;i++)
    Values[i] = gp->Default[i];
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueColor::Read(GenPara *gp,sU32 *&data)
{
  for(sInt i=0;i<4;i++)
  {
    Values[i] = *(sF32 *)data;
    data++;
  }
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueColor::Write(sU32 *&data)
{
  for(sInt i=0;i<4;i++)
  {
    *(sF32 *)data = Values[i];
    data++;
  }
}

void GenValueColor::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditFRGBA(CMD_PARA_CHANGE,Values,0);
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);
}

void GenValueColor::StorePara(GenOp *,sInt *d) 
{ 
  d[0] = (sRange<sInt>((sInt)(Values[0]*255),255,0)<<16)
       | (sRange<sInt>((sInt)(Values[1]*255),255,0)<< 8)
       | (sRange<sInt>((sInt)(Values[2]*255),255,0)<< 0)
       | (sRange<sInt>((sInt)(Values[3]*255),255,0)<<24);
}

/****************************************************************************/

GenValueString::GenValueString()
{
  Size = 0;
  Buffer = 0;
}

GenValueString::~GenValueString()
{
  delete[] Buffer;
}

GenValue *GenValueString::Copy()
{
  GenValueString *val;
  val = new GenValueString;
  val->Para = Para;
  val->ParaId = ParaId;
  val->Default(val->Para);
  sCopyString(val->Buffer,Buffer,val->Size);

  return val;
}

void GenValueString::Default(GenPara *para)
{
  delete[] Buffer;
  Size = para->Count;
  sVERIFY(Size>1);
  Buffer = new sChar[Size];
  sCopyString(Buffer,para->Choices,Size);
}

void GenValueString::Read(GenPara *para,sU32 *&data)
{
  Default(para);
  sInt len = *data++;
  sCopyString(Buffer,(sChar *)data,Size);
  data+= (len+4)/4;
}

void GenValueString::Write(sU32 *&data)
{
  sInt len = sGetStringLen(Buffer);
  *data++ = len;
  sCopyMem(data,Buffer,len+1);
  data+=(len+4)/4;
}

void GenValueString::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditString(CMD_PARA_CHANGE,Buffer,0,Size);
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);
}

sInt GenValueString::GetParaSize()
{
  return 1+(sGetStringLen(Buffer)+2)/2;
}

void GenValueString::StorePara(GenOp *,sInt *d)
{
  sInt len = sGetStringLen(Buffer);
  *d++ = (len+2)/2;
  
  for(sInt i=0;i<len;i++)
    ((sU16 *)d)[i] = Buffer[i];
  ((sU16 *)d)[len+0] = 0;
  ((sU16 *)d)[len+1] = 0;

  d += (len+2)/2;
}

/****************************************************************************/

GenValue *GenValueFilename::Copy()
{
  GenValueFilename *val;
  val = new GenValueFilename;
  val->Para = Para;
  val->ParaId = ParaId;
  val->Default(val->Para);
  sCopyString(val->Buffer,Buffer,val->Size);

  return val;
}

void GenValueFilename::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditString(CMD_PARA_CHANGE,Buffer,0,Size);
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);

  parawin->AddBox("..",0,CMD_PARA_FILEREQ+Para->ParaId);
}

/****************************************************************************/

GenValue *GenValueLink::Copy()
{
  GenValueLink *val;
  val = new GenValueLink;
  val->Para = Para;
  val->ParaId = ParaId;
  val->Default(val->Para);
  sCopyString(val->Buffer,Buffer,val->Size);

  return val;
}

void GenValueLink::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditString(CMD_PARA_CHANGE,Buffer,0,Size);
  con->LayoutInfo.Init(x0,y,sMin(x1,10),y+1);
  grid->AddChild(con);

  parawin->AddBox("->",0,CMD_PARA_FOLLOWLINK+Para->ParaId);
  parawin->AddBox("..",1,CMD_PARA_OPLIST+Para->ParaId);
  parawin->AddBox("...",2,CMD_PARA_PAGELIST+Para->ParaId);
}

/****************************************************************************/

GenValue *GenValueContrast::Copy()
{
  GenValueFloatV *val;
  val = new GenValueFloatV;
  val->Para = Para;
  val->ParaId = ParaId;

  for(sInt i=0;i<4;i++)
    val->Values[i] = Values[i];

  return val;
}

void GenValueContrast::Default(GenPara *gp)
{
  for(sInt i=0;i<4;i++)
    Values[i] = gp->Default[i];
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueContrast::Read(GenPara *gp,sU32 *&data)
{
  sInt max = *data++;
  sVERIFY(max>=1 && max<=4);
  sVERIFY(max==gp->Count);
  for(sInt i=0;i<max;i++)
  {
    Values[i] = *(sF32 *)data;
    data++;
  }
  Para = gp;
  ParaId = gp->ParaId;
}

void GenValueContrast::Write(sU32 *&data)
{
  *data++ = Para->Count;
  for(sInt i=0;i<Para->Count;i++)
  {
    *(sF32 *)data = Values[i];
    data++;
  }
}

void GenValueContrast::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sControl *con;
  con = new sControl;
  con->EditFloat(CMD_PARA_CHANGE,Values,0);
  con->InitNum(Para->Min,Para->Max,Para->Step,Para->Default,Para->Count);
  con->Style |= sCS_ZONES;
  con->LayoutInfo.Init(x0,y,x1,y+1);
  grid->AddChild(con);
}

void GenValueContrast::StorePara(GenOp *,sInt *d) 
{ 
  for(sInt i=0;i<Para->Count;i++) 
    d[i] = (sInt)(Values[i]*0x8000); 
}

/****************************************************************************/

GenValueText::GenValueText()
{
  Text = new sText();
  sBroker->AddRoot(Text);
}

GenValueText::~GenValueText()
{
  sBroker->RemRoot(Text);
}

GenValue *GenValueText::Copy()
{
  GenValueText *val;
  val = new GenValueText;
  val->Para = Para;
  val->ParaId = ParaId;
  val->Default(val->Para);
  val->Text->Copy(Text);

  return val;
}

void GenValueText::Default(GenPara *para)
{
  Text->Init(para->Choices);
}

void GenValueText::Read(GenPara *para,sU32 *&data)
{
  Default(para);
  sInt len = *data++;
  Text->Init((const sChar *)data,len);
  data+= (len+4)/4;
}

void GenValueText::Write(sU32 *&data)
{
  sInt len = Text->Used;
  Text->Text[len] = 0;
  *data++ = len;
  sCopyMem(data,Text->Text,len+1);
  data+=(len+4)/4;
}

void GenValueText::MakeGui(sGridFrame *grid,sInt x0,sInt x1,sInt y,ParaWin_ *parawin)
{
  sTextControl *con;
  con = new sTextControl;
  con->SetText(Text);
  con->LayoutInfo.Init(x0,y,x1+1,y+10);
  con->AddBorder(new sThinBorder);
  con->AddScrolling(1,1);
  grid->AddChild(con);
}

sInt GenValueText::GetParaSize()
{
  return 1+(Text->Used+2)/2;
}

void GenValueText::StorePara(GenOp *,sInt *d)
{
  sInt len = Text->Used;
  *d++ = (len+2)/2;
  
  for(sInt i=0;i<len;i++)
    ((sU16 *)d)[i] = Text->Text[i];
  ((sU16 *)d)[len+0] = 0;
  ((sU16 *)d)[len+1] = 0;

  d += (len+2)/2;
}

/****************************************************************************/
/****************************************************************************/

#define SCRAMBLE(x) (sChar((x)^0xea))
extern const sChar DemoString[] =
{
  SCRAMBLE('d'),
  SCRAMBLE('e'),
  SCRAMBLE('m'),
  SCRAMBLE('o'),
  SCRAMBLE('-'),
  SCRAMBLE('v'),
  SCRAMBLE('e'),
  SCRAMBLE('r'),
  SCRAMBLE('s'),
  SCRAMBLE('i'),
  SCRAMBLE('o'),
  SCRAMBLE('n'),
  SCRAMBLE(' '),
  SCRAMBLE('f'),
  SCRAMBLE('o'),
  SCRAMBLE('r'),
  SCRAMBLE(' '),
/*
  SCRAMBLE('c'),
  SCRAMBLE('o'),
  SCRAMBLE('n'),
  SCRAMBLE('t'),
  SCRAMBLE('e'),
  SCRAMBLE('n'),
  SCRAMBLE('t'),
  SCRAMBLE(' '),
  SCRAMBLE('l'),
  SCRAMBLE('o'),
  SCRAMBLE('g'),
  SCRAMBLE('i'),
  SCRAMBLE('c'),
*/
  SCRAMBLE('4'),
  SCRAMBLE('9'),
  SCRAMBLE('G'),
  SCRAMBLE('a'),
  SCRAMBLE('m'),
  SCRAMBLE('e'),
  SCRAMBLE('s'),
  0
};

