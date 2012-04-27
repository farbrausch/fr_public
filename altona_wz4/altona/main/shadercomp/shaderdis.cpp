/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#include "shadercomp/shaderdis.hpp"
#include "base/types2.hpp"
#include "base/graphics.hpp"

/****************************************************************************/

// register names for pixel shaders

static const sChar *psnames[32] = 
{
  L"r%d"  ,L"v%d"   ,L"c%d" ,L"t%d" ,
  0       ,0        ,0      ,0      ,
  L"oC%d" ,L"oDepth",L"s%d" ,L"?"   ,
};

// register names for vertex shaders

static const sChar *vsnames[32] = 
{
  L"r%d",L"v%d" ,L"c%d" ,L"a%d" ,
  0     ,L"oD%d",L"oT%d",0      ,
  0     ,0      ,0      ,0      ,
  0     ,0      ,L"b%d" ,L"aL"  ,
  0     ,0      ,0      ,L"p%d" ,
};

// vertex shader output registers (depending on NUM)

static const sChar *rastout[4] = 
{
  L"oPos",L"oFog",L"oPsize",L"?"
};

// opcode name table

static const sChar *opcodes[0x100] = 
{
  L"nop"   ,L"mov"   ,L"add"   ,L"sub"    ,L"mad"   ,L"mul"   ,L"rcp"   ,L"rsq"   ,
  L"dp3"   ,L"dp4"   ,L"min"   ,L"max"    ,L"slt"   ,L"sge"   ,L"exp"   ,L"log"   ,
  L"lit"   ,L"dst"   ,L"lrp"   ,L"frc"    ,L"m4x4"  ,L"m4x3"  ,L"m3x4"  ,L"m3x3"  ,
  L"m3x2"  ,L"call"  ,L"callnz",L"loop"   ,L"ret"   ,L"endloop",L"label",L"dcl"   ,

  L"pow"   ,L"crs"   ,L"sgn"   ,L"abs"    ,L"nrm"   ,L"sincos",L"rep"   ,L"endrep",
  L"if"    ,L"ifc"   ,L"else"  ,L"endif"  ,L"break" ,L"breakc",L"mova"  ,L"defb"  ,
  L"defi"  ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,

  L"texcoord"   ,L"texkill"     ,L"texld"       ,L"texbem"    ,L"texbeml"     ,L"texreg2ar"   ,L"texreg2bg",L"texm3x2pad",
  L"texm3x2tex" ,L"texm3x3pad"  ,L"texm3x3tex"  ,0            ,L"texm3x3spec" ,L"texm3x3vspec",L"expp"     ,L"logp",
  L"cnd"        ,L"def"         ,L"texreg2rgb"  ,L"texdp3tex" ,L"texm3x2depth",L"texdp3"      ,L"texm3x3"  ,L"texdepth",
  L"cmp"        ,L"bem"         ,L"dp2add"      ,L"dsx"       ,L"dsy"         ,L"texldd"      ,L"setp"     ,L"texldl",
  
  L"breaklp",0       ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,


  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,

  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,

  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,

  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  0        ,0        ,0        ,0         ,0        ,0        ,0        ,0        ,
  L"vs"    ,L"ps"    ,0        ,0         ,0        ,L"phase" ,L"comment",L"end"  ,
};

// opcode info table

static const sU8 outin[0x100]=
{
  0x00,0x11,0x12,0x12  ,0x13,0x12,0x11,0x11,
  0x12,0x12,0x12,0x12  ,0x12,0x12,0x11,0x11,    // 0x80 = label
  0x11,0x12,0x13,0x11  ,0x12,0x12,0x12,0x12,    // 0x90 = dcl
  0x12,0x80,0x80,0x80  ,0x00,0x00,0x80,0x90,    // 0xb0 = def

  0x12,0x12,0x11,0x11  ,0x11,0x13,0x01,0x00,
  0x01,0x02,0x00,0x00  ,0x00,0x02,0x11,0x90,
  0xb0,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

  0x10,0x01,0x10,0x11  ,0x11,0x11,0x11,0x11,
  0x11,0x11,0x11,0x00  ,0x12,0x11,0x11,0x11,
  0x13,0xb0,0x13,0x11  ,0x11,0x14,0x11,0x10,
  0x13,0x12,0x13,0x11  ,0x11,0x13,0x00,0x12,
  
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,


  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00  ,0x00,0x00,0x00,0x00,
  0xa0,0xa0,0x00,0x00  ,0x00,0x00,0x00,0x00,    // 0xa0 = version (faked!)
};

// vertex shader declarator

static const sChar *usage[16] =
{
  L"position"   ,L"blendweights",L"blendindices",L"normal",
  L"psize"      ,L"texcoord"    ,L"tangent"     ,L"binormal",
  L"tessfactor" ,L"positiont"   ,L"color"       ,L"fog",
  L"depth"      ,L"sample"      ,L"?"           ,L"?",
};

// pixel shader declarator

static const sChar *texkind[8] = 
{
  L"",L"_?",L"_2d",L"_cube",L"_volume",L"_?",L"_?",L"_?"
};

// writemask letters

static const sU8 swizzlebits[] = { 'x','y','z','w' };

// source modifier (not standard conform, but who cares about PS1.3 ?

static const sChar *sourcepre[16] = 
{
  L""     ,L"-"   ,L""    ,L"-"  ,
  L""     ,L"-"   ,L"1-"  ,L""   ,
  L"-"    ,L""    ,L""    ,L"abs(",
  L"-abs(",L"!"   ,L"?"   ,L"?"
};

static const sChar *sourcepost[16] = 
{
  L""     ,L""    ,L"_bias",L"_bias" ,
  L"_bx2" ,L"_bx2",L""    ,L"_x2"  ,
  L"_x2"  ,L"_dz" ,L"_dw" ,L")" ,
  L")"    ,L"!"   ,L"?"   ,L"?"
};

/****************************************************************************/

static void PrintReg(sU32 reg,sU32 version,sTextBuffer &tb)
{
  sInt type,num;
  const sChar *name;

  type = ((reg>>28)&7) + ((reg>>11)&3)*8;
  num = reg&0x7ff;
  if(version<0xffff0000)
  {
    if(type==4)
      name = rastout[sMin(num,3)];
    else
      name = vsnames[type];
  }
  else
  {
    name = psnames[type];
  }
  if(name)
  {
    const sChar* ptr = name;
    while (*ptr && *ptr != '%')
      ptr++;
    if (*ptr == '%')
      tb.PrintF(name,num);
    else
      tb.Print(name);
  }
  else
    tb.PrintChar('?');
}

static void printswizzle(sU32 value,sTextBuffer &tb)
{
  sInt j;
  sInt m[4];
  j = (value&0x00ff0000)>>16;
  if(j!=0xe4)
  {
    m[0] = (j>>0)&3;
    m[1] = (j>>2)&3;
    m[2] = (j>>4)&3;
    m[3] = (j>>6)&3;
    tb.PrintChar('.');
    tb.PrintChar(swizzlebits[m[0]]);
    if(!(m[0]==m[1] && m[0]==m[2] && m[0]==m[3]))
    {
      tb.PrintChar(swizzlebits[m[1]]);
      tb.PrintChar(swizzlebits[m[2]]);
      tb.PrintChar(swizzlebits[m[3]]);
    }
  }
}

/****************************************************************************/

void sPrintShader(const sU32 *data,sInt flags)
{
  sTextBuffer tb;
  sPrintShader(tb,data,flags);
  sDPrint(tb.Get());
}

void sPrintShader(sTextBuffer& tb, const sU32 *data,sInt flags)
{
  sInt code,in,out,def,dcl,label;
  sInt i,len,j;
  sU32 version=0;
  sU32 val;
  sInt komma;
  sInt line;
  sBool end = sFALSE;
  const sU32 *datastart;

  line = 1;
  while(!end)
  {
    val = *data;
 
    if((val&0x0000ffff)==0x0000fffe)
    {
      if(!(flags & sPSF_NOCOMMENTS))
      {
        if(flags & sPSF_CARRAY)
        {
          sChar buffer[6*4+1];
          sChar *bp;

          tb.Print(L"  ");
          bp = buffer;
          sInt clen = ((val&0xffff0000)>>16)+1;
          for(sInt i=0;i<clen;i++)
          {
            if((i%6)==0 && i!=0)
            {
              *bp++ = 0;
              tb.PrintF(L" // <%s>\n  ",buffer);
              bp = buffer;
            }
            tb.PrintF(L"0x%08x,",data[i]);
            for(sInt j=0;j<4;j++)
            {
              sChar c = (data[i]>>(j*8))&0xff;
              *bp++ = (c>=0x20 && c<=0x7f) ? c : '.';
            }
          }
          *bp++ = 0;
          tb.PrintF(L"%_ // <%s>\n",(6-(clen%6))%6*11,buffer);
        }
        else
        {
          tb.Print(L"      (Comment)\n");
        }
      }
      data = data + ((val&0xffff0000)>>16) + 1;
      continue;
    }

    switch(val&0xffff0000)
    {
    case 0xffff0000:
      code = 0xf9; 
      version = val;
      break;
    case 0xfffe0000:
      code = 0xf8; 
      version = val;
      break;
    default:
      code = val&0x00ff;
      break;
    }

    if(val == 0xffff)
      end = sTRUE;


    in = out = def = dcl = label = komma = 0;
    switch(outin[code]&0xf0)
    {
    case 0x80:
      label = 1;
      break;
    case 0x90:
      dcl = 1;
      out = 1;
      break;
    case 0xa0:
      break;
    case 0xb0:
      def = 4;
      out = 1;
      break;
    default:
      in = outin[code]&0x0f;
      out = (outin[code]&0x70)>>4;
      break;
    }

    // fix up opcodes
    if(code==0x42 && (version&0xffff)>=0x0200)
      in=2, out=1;
    if(code==0x25 && (version&0xffff)>=0x0300)  // sincos: 2 params since SM3
      in=1, out=1;

    len = val>>24;
    if((version&0xffff)>=0x0200 && len>0 && len<0x80)
      len = 1+len;
    else
      len = 1+in+out+dcl+label+def;

    datastart = data;
    

    if(flags & sPSF_CARRAY)
    {
      tb.Print(L"  ");
      for(i=0;i<len;i++)
        tb.PrintF(L"0x%08x,",data[i]);
      if(len<6)
      tb.PrintF(L"%_",(6-len)*11);
      tb.PrintF(L" // ");
    }
    else
    {
      tb.PrintF(L"%04d  ",line++);
      for(i=0;i<5;i++)
      {
        if(i<len)
        {
          tb.PrintF(L"%08x ",data[i]);
        }
        else
        {
          if(def && i==2)
            tb.PrintF(L"...      ");
          else
            tb.PrintF(L"         ");
        }
      }
    }
    data++;

    // combined codes

    if((val & 0x40000000) && ((val >> 17) != 0x7fff))
      tb.PrintF(L"+");
    else
      tb.PrintF(L" ");

    sString<32> opcode;
    if((outin[code]&0xf0)==0xa0)
    {
      sSPrintF(opcode,L"%s_%d_%d",opcodes[code],(version&0xff00)>>8,version&0xff);
    }
    else if(dcl)
    {
      val = *data++;
      if(version<0xffff0000) 
      {
        sInt num = (val&0xf0000)>>16;
        if(num!=0 || (val&0x0f)==5 || (val&0x0f)==10)
          sSPrintF(opcode,L"%s_%s%d",opcodes[code],usage[val&0x0f],num);
        else
          sSPrintF(opcode,L"%s_%s",opcodes[code],usage[val&0x0f]);
      }
      else
        sSPrintF(opcode,L"%s%s",opcodes[code],texkind[(val>>27)&7]);
    }
    else
    {
      if(opcodes[code])
        opcode = opcodes[code];
      else
        opcode = L"???";
    }
    tb.PrintF(L"%-16s ",(sChar *)opcode);

    // label parameter

    if(label)
    {
      if(komma) tb.PrintChar(','); komma=1;
      tb.PrintF(L"label");
      data++;
    }

    // destination register

    for(i=0;i<out;i++)
    {
      if(komma) tb.PrintChar(','); komma=1;
      val = *data++;
      PrintReg(val,version,tb);
      if(val&0x00100000) tb.PrintF(L"_sat");
      if(val&0x00200000) tb.PrintF(L"_pp");
      if(val&0x00400000) tb.PrintF(L"_msamp");
      j = (val&0xf0000)>>16;
      if(j!=15)
      {
        tb.PrintChar('.');
        if(j&1) tb.PrintChar('x');
        if(j&2) tb.PrintChar('y');
        if(j&4) tb.PrintChar('z');
        if(j&8) tb.PrintChar('w');
      }
    }

    // source register

    for(i=0;i<in;i++)
    {
      if(komma) tb.PrintChar(','); komma=1;
      val = *data++;
      tb.PrintF(L"%s",sourcepre[(val>>24)&15]);
      PrintReg(val,version,tb);
      tb.PrintF(L"%s",sourcepost[(val>>24)&15]);
      if(val&0x00002000)
      {
        if((version&0xffff)>=0x0200)
        {
          tb.PrintF(L"[");
          PrintReg(*data,version,tb);
          printswizzle(*data,tb);
          tb.PrintF(L"]");
          data++;
        }
        else
        {
          tb.PrintF(L"[a0.x]");
        }
      }
      printswizzle(val,tb);
    }

    // define constant

    for(i=0;i<def;i++)
    {
      if(komma) tb.PrintChar(','); komma=1;
      val = *data++;
      sF32 valf = sRawCast<sF32,sU32>(val);//*((sF32 *) &val);
      tb.PrintF(L"%7.3f",valf);
    }
    tb.PrintChar('\n');

    sVERIFY(datastart+len == data);
  }
  if(!(flags & sPSF_CARRAY))
    tb.PrintChar('\n');
}

/****************************************************************************/


void sPrintShader(class sTextBuffer& tb, sShaderBlob *blob, sInt flags)
{
  while(blob && (blob->Type&sSTF_PLATFORM)!=sSTF_HLSL23)
    blob = blob->Next();
  if(blob)
  {
    sPrintShader(tb,(const sU32*)blob->Data,flags);
  }
}

/****************************************************************************/
