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

#include "main.hpp"
#include "base/math.hpp"
#include "base/serialize.hpp"

/****************************************************************************/

#define CHECK(x) if(!(x)) sCheck(sTXT(__FILE__),__LINE__);

static sInt ErrorCount;

void sCheck(const sChar *file,sInt line)
{
  sPrintF(L"%s(%d) failed!\n",file,line);
  sDPrintF(L"%s(%d) failed!\n",file,line);
  ErrorCount++;
}

void sCheckModule(const sChar *mod)
{
  sPrintF(L"Testing %s...\n",mod);
}

/****************************************************************************/

void TestSimpleFunctions()
{
  sCheckModule(L"Simple Functions");

  CHECK(sFindLowerPower(0)==0);
  CHECK(sFindLowerPower(1)==0);
  CHECK(sFindLowerPower(2)==1);
  CHECK(sFindLowerPower(3)==1);  // **
  CHECK(sFindLowerPower(4)==2);
  CHECK(sFindLowerPower(5)==2);  // **
  CHECK(sFindLowerPower(6)==2);  // **
  CHECK(sFindLowerPower(7)==2);  // **
  CHECK(sFindLowerPower(8)==3);
  CHECK(sFindLowerPower(9)==3);  // **

  CHECK(sFindHigherPower(0)==0);
  CHECK(sFindHigherPower(1)==0);
  CHECK(sFindHigherPower(2)==1);
  CHECK(sFindHigherPower(3)==2);  // **
  CHECK(sFindHigherPower(4)==2);
  CHECK(sFindHigherPower(5)==3);  // **
  CHECK(sFindHigherPower(6)==3);  // **
  CHECK(sFindHigherPower(7)==3);  // **
  CHECK(sFindHigherPower(8)==3);
  CHECK(sFindHigherPower(9)==4);  // **

  CHECK(sIsPower2(0)==1);
  CHECK(sIsPower2(1)==1);
  CHECK(sIsPower2(2)==1);
  CHECK(sIsPower2(3)==0);  // **
  CHECK(sIsPower2(4)==1);
  CHECK(sIsPower2(5)==0);  // **
  CHECK(sIsPower2(6)==0);  // **
  CHECK(sIsPower2(7)==0);  // **
  CHECK(sIsPower2(8)==1);
  CHECK(sIsPower2(9)==0);  // **
}

/****************************************************************************/

void TestMem()
{
  sU8 m0[256];
  sU8 m1[256];

  sCheckModule(L"Memory Functions");

  for(sInt i=0;i<256;i++)
  {
    m0[i] = '0';
    m1[i] = 'a';
  }

  CHECK(sCmpMem("0000","0000",3)==0);
  CHECK(sCmpMem("0001","0000",3)==0);
  CHECK(sCmpMem("001","000",3)!=0);
  CHECK(sCmpMem("010","000",3)!=0);
  CHECK(sCmpMem("100","000",3)!=0);
  CHECK(sCmpMem("001","002",3)<0);
  CHECK(sCmpMem("002","001",3)>0);
  CHECK(sCmpMem("021","012",3)>0);

  sSetMem(m0+4,'1',4);
  CHECK(sCmpMem("000011110000",m0,12)==0);
  sSetMem(m0+4,'2',0);
  CHECK(sCmpMem("000011110000",m0,12)==0);
  sCopyMem(m0+4,"abcd",4);
  CHECK(sCmpMem("0000abcd0000",m0,12)==0);
}

/****************************************************************************/

void TestStringFunctions()
{
  sChar b0[256];
  sChar b1[256];

  sCheckModule(L"String Functions");

  sClear(b0);
  sClear(b1);

  CHECK(sCmpString(L"0000",L"0000")==0);
  CHECK(sCmpString(L"0000",L"000")!=0);
  CHECK(sCmpString(L"000",L"0000")!=0);
  CHECK(sCmpString(L"000",L"100")!=0);
  CHECK(sCmpString(L"000",L"010")!=0);
  CHECK(sCmpString(L"000",L"001")!=0);
  CHECK(sCmpString(L"100",L"100")==0);
  CHECK(sCmpString(L"010",L"010")==0);
  CHECK(sCmpString(L"001",L"001")==0);
  CHECK(sCmpString(L"001",L"002")<0);
  CHECK(sCmpString(L"002",L"001")>0);
  CHECK(sCmpString(L"021",L"012")>0);

  sCopyString(b0,L"000000000000",256);
  sCopyString(b0+4,L"1111",256);
  CHECK(sCmpString(b0,L"00001111")==0);
  CHECK(sCmpString(b0+9,L"000")==0);

  sCopyString(b0,L"abcd",6);
  CHECK(sCmpString(b0,L"abcd")==0);
  sCopyString(b0,L"abcd",5);
  CHECK(sCmpString(b0,L"abcd")==0);
  sCopyString(b0,L"abcd",4);
  CHECK(sCmpString(b0,L"abc")==0);
  sCopyString(b0,L"abcd",3);
  CHECK(sCmpString(b0,L"ab")==0);

  sCopyString(b0,L"abcd",256); sAppendString(b0,L"1234",10);
  CHECK(sCmpString(b0,L"abcd1234")==0);
  sCopyString(b0,L"abcd",256); sAppendString(b0,L"1234",9);
  CHECK(sCmpString(b0,L"abcd1234")==0);
  sCopyString(b0,L"abcd",256); sAppendString(b0,L"1234",8);
  CHECK(sCmpString(b0,L"abcd123")==0);
  sCopyString(b0,L"abcd",256); sAppendString(b0,L"1234",7);
  CHECK(sCmpString(b0,L"abcd12")==0);

  sCopyString(b0,L"aBcDeFgHiJkLmNoPqRsTuVwXyZ \n\t_:;öÄü-",256);
  sCopyString(b1,L"AbCdEfGhIjKlMnOpQrStUvWxYz \n\t_:;ÖäÜ-",256);

  sMakeUpper(b0);
  sMakeLower(b1); 
  sMakeUpper(b1);
  CHECK(sCmpString(b0,b1)==0);
  sMakeLower(b0); 
  sMakeLower(b1);
  CHECK(sCmpString(b0,b1)==0);

  CHECK(sFindString(b0, L"klmn") == 10);
  CHECK(sFindString(b0, L"KlMn") == -1);
  CHECK(sFindString(b0, L"abcd") == 0);
}

/****************************************************************************/

void TestString()
{
  sCheckModule(L"sString");

  sString<512> t1(L"the quick brown fox jumps over the lazy dog");

  t1.CutLeft(4);          CHECK(sCmpString(t1, L"quick brown fox jumps over the lazy dog")==0);
  t1.CutRight(4);         CHECK(sCmpString(t1, L"quick brown fox jumps over the lazy")==0);
  t1.CutRange(6,6);       CHECK(sCmpString(t1, L"quick fox jumps over the lazy")==0);
  t1.CutRange(0,5);       CHECK(sCmpString(t1, L" fox jumps over the lazy")==0);
  t1.StripLeft();         CHECK(sCmpString(t1, L"fox jumps over the lazy")==0);
  t1.CutRange(19,4+666);  CHECK(sCmpString(t1, L"fox jumps over the ")==0);
  t1.StripRight();        CHECK(sCmpString(t1, L"fox jumps over the")==0);
  t1.StripLeft(L"fox");   CHECK(sCmpString(t1, L" jumps over the")==0);
  t1.StripRight(L"the");  CHECK(sCmpString(t1, L" jumps over ")==0);
  t1.Strip(L"\n\t");      CHECK(sCmpString(t1, L" jumps over ")==0);
  t1.Strip(L"  ");        CHECK(sCmpString(t1, L"jumps over")==0);
  t1.CutRange(-2,4);      CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRange(-2,2);      CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRange(10,2);      CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRange( 3,0);      CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRight( 0);        CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRight(-10);       CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutLeft( 0);         CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutLeft(-10);        CHECK(sCmpString(t1, L"mps over")==0);
  t1.CutRange( 2,-3);     CHECK(sCmpString(t1, L"s over")==0);
  t1.CutRange( 4,-2);     CHECK(sCmpString(t1, L"s er")==0);
  t1.CutRange(66,-200);   CHECK(sCmpString(t1, L"")==0);
  t1.PadRight(3, L'a');   CHECK(sCmpString(t1, L"aaa")==0);
  t1.PadRight(4);         CHECK(sCmpString(t1, L"aaa ")==0);
  t1.PadRight(2);         CHECK(sCmpString(t1, L"aaa ")==0);
  t1.PadRight(-2);        CHECK(sCmpString(t1, L"aaa ")==0);
  t1.PadLeft(-2);         CHECK(sCmpString(t1, L"aaa ")==0);
  t1.PadLeft(0);          CHECK(sCmpString(t1, L"aaa ")==0);
  t1.PadLeft(5);          CHECK(sCmpString(t1, L" aaa ")==0);
  t1.PadLeft(6, L'\n');   CHECK(sCmpString(t1, L"\n aaa ")==0);
  t1.PadLeft(9, L'*');    CHECK(sCmpString(t1, L"***\n aaa ")==0);
  t1.Strip();             CHECK(sCmpString(t1, L"***\n aaa")==0);
  t1.Strip(L"*a");        CHECK(sCmpString(t1, L"\n ")==0);
  t1.Strip();             CHECK(sCmpString(t1, L"")==0);
}

/****************************************************************************/

void TestFloat(sF32 null=0)
{
  sCheckModule(L"Float Stuff");
  CHECK(sInt(0.9f)==0);
  CHECK(sInt(1.0f)==1);
  CHECK(sInt(1.1f)==1);
  CHECK(sInt(1.5f)==1);
  CHECK(sInt(1.7f)==1);
  CHECK(sInt(-0.9f)==0);
  CHECK(sInt(-1.0f)==-1);
  CHECK(sInt(-1.1f)==-1);
  CHECK(sInt(-1.5f)==-1);
  CHECK(sInt(-1.7f)==-1);

  CHECK(!sIsInf(1.0f));
  CHECK(!sIsInf(0.0f));
  CHECK(!sIsNan(1.0f));
  CHECK(!sIsNan(0.0f));
  CHECK(sIsInf(1.0f/null));
  CHECK(sIsNan(sSqrt(-1+null)));
}

/****************************************************************************/

void TestFormat()
{
  sCheckModule(L"String Formatting");

  // some tests fail because of inaccuracy and rounding for float output!

  sString<64> str;
  sString<64> a;
  sString<6> five;

  a = L"123";

  sSPrintF(str,L"|%x|",0xabc);                      CHECK(str==L"|abc|");
  sSPrintF(str,L"|%x|",-0xabc);                     CHECK(str==L"|-abc|");
  sSPrintF(str,L"|%x|",sU32(-0xabc));               CHECK(str==L"|fffff544|");
  sSPrintF(str,L"|%d|",123);                        CHECK(str==L"|123|");
  sSPrintF(str,L"|%d|",-123);                       CHECK(str==L"|-123|");
  sSPrintF(str,L"|%_|",3);                          CHECK(str==L"|   |");

  sSPrintF(str,L"|%x|",sU64(0x0123456789abcdefULL)); CHECK(str==L"|123456789abcdef|");
  sSPrintF(str,L"|%x|",sS64(0x0123456789abcdefULL)); CHECK(str==L"|123456789abcdef|");
  sSPrintF(str,L"|%x|",sS64(-0x0123456789abcdefLL)); CHECK(str==L"|-123456789abcdef|");

  sSPrintF(str,L"|%d|",sU64(1234567812345678ULL)); CHECK(str==L"|1234567812345678|");
  sSPrintF(str,L"|%d|",sS64(1234567812345678ULL)); CHECK(str==L"|1234567812345678|");
  sSPrintF(str,L"|%d|",sS64(-1234567812345678LL)); CHECK(str==L"|-1234567812345678|");

  sSPrintF(str,L"|%6d|",123);                       CHECK(str==L"|   123|");
  sSPrintF(str,L"|%6d|",-123);                      CHECK(str==L"|  -123|");
  sSPrintF(str,L"|%06d|",123);                      CHECK(str==L"|000123|");
  sSPrintF(str,L"|%06d|",-123);                     CHECK(str==L"|-00123|");
  sSPrintF(str,L"|%-6d|",123);                      CHECK(str==L"|123   |");
  sSPrintF(str,L"|%-6d|",-123);                     CHECK(str==L"|-123  |");
  sSPrintF(str,L"|%-06d|",123);                     CHECK(str==L"|000123|");
  sSPrintF(str,L"|%-06d|",-123);                    CHECK(str==L"|-00123|");

  sSPrintF(str,L"|%5.2r|",1   );                    CHECK(str==L"| 0.01|");
  sSPrintF(str,L"|%5.2r|",12  );                    CHECK(str==L"| 0.12|");
  sSPrintF(str,L"|%5.2r|",123 );                    CHECK(str==L"| 1.23|");
  sSPrintF(str,L"|%5.2r|",1234);                    CHECK(str==L"|12.34|");
  sSPrintF(str,L"|%05.2r|",1   );                   CHECK(str==L"|00.01|");
  sSPrintF(str,L"|%05.2r|",12  );                   CHECK(str==L"|00.12|");
  sSPrintF(str,L"|%05.2r|",123 );                   CHECK(str==L"|01.23|");
  sSPrintF(str,L"|%05.2r|",1234);                   CHECK(str==L"|12.34|");
  sSPrintF(str,L"|%.2r|",1   );                     CHECK(str==L"|0.01|");
  sSPrintF(str,L"|%.2r|",12  );                     CHECK(str==L"|0.12|");
  sSPrintF(str,L"|%.2r|",123 );                     CHECK(str==L"|1.23|");
  sSPrintF(str,L"|%.2r|",1234);                     CHECK(str==L"|12.34|");

  sSPrintF(str,L"|%8.3f|",12.3f);                   CHECK(str==L"|  12.300|");
  sSPrintF(str,L"|%8.3f|",-12.3f);                  CHECK(str==L"| -12.300|");
  sSPrintF(str,L"|%8.3f|",0.123f);                  CHECK(str==L"|   0.123|");
  sSPrintF(str,L"|%8.3f|",123.0f);                  CHECK(str==L"| 123.000|");
//  sSPrintF(str,L"|%24.8f|",12.3456f   );           CHECK(str==L"|         12.34560000|");
//  sSPrintF(str,L"|%24.8f|",12.3456789f);           CHECK(str==L"|         12.34567890|");
//  sSPrintF(str,L"|%24.8f|",12.3f);                 CHECK(str==L"|         12.30000000|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.004f,1.004f);     CHECK(str==L"|    1.00|  1.0040|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.005f,1.005f);     CHECK(str==L"|    1.01|  1.0050|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.006f,1.006f);     CHECK(str==L"|    1.01|  1.0060|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.009f,1.009f);     CHECK(str==L"|    1.01|  1.0090|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.010f,1.010f);     CHECK(str==L"|    1.01|  1.0100|");
  sSPrintF(str,L"|%8.2f|%8.4f|",1.011f,1.011f);     CHECK(str==L"|    1.01|  1.0110|");
  sSPrintF(str,L"|%8f|",12.345f);                   CHECK(str==L"|12.34500|");
  sSPrintF(str,L"|%6f|",12.345f);                   CHECK(str==L"|12.345|");
  sSPrintF(str,L"|%5f|",12.345f);                   CHECK(str==L"|12.35|");
  sSPrintF(str,L"|%8f|",-12.34f);                   CHECK(str==L"|-12.3400|");
  sSPrintF(str,L"|%6f|",-12.34f);                   CHECK(str==L"|-12.34|");
  sSPrintF(str,L"|%5f|",-12.34f);                   CHECK(str==L"|-12.3|");

  sSPrintF(str,L"|%8.3f|",64.0f);                   CHECK(str==L"|  64.000|");
  sSPrintF(str,L"|%8.3f|",99.0f-0.0001f);           CHECK(str==L"|  99.000|");
  sSPrintF(str,L"|%8.3f|",100.0f-0.0001f);          CHECK(str==L"| 100.000|");

   
  sSPrintF(str,L"|%5f|",100000.0f);                  CHECK(str==L"|100000|");
  sSPrintF(str,L"|%5f|",10000.0f);                   CHECK(str==L"|10000|");
  sSPrintF(str,L"|%5f|",1000.0f);                    CHECK(str==L"| 1000|");
  sSPrintF(str,L"|%5f|",100.0f);                     CHECK(str==L"|100.0|");
  sSPrintF(str,L"|%5f|",10.0f);                      CHECK(str==L"|10.00|");
  sSPrintF(str,L"|%5f|",1.0f);                       CHECK(str==L"|1.000|");
  sSPrintF(str,L"|%5f|",0.1f);                       CHECK(str==L"|0.100|");
  sSPrintF(str,L"|%5f|",0.01f);                      CHECK(str==L"|0.010|");
  sSPrintF(str,L"|%5f|",0.001f);                     CHECK(str==L"|0.001|");
  sSPrintF(str,L"|%5f|",0.0001f);                    CHECK(str==L"|0.000|");
  sSPrintF(str,L"|%5f|",0.00001f);                   CHECK(str==L"|0.000|");
  sSPrintF(str,L"|%5f|",0.000001f);                  CHECK(str==L"|0.000|");

  sSPrintF(str,L"|%5.2f|",100000.0f);                  CHECK(str==L"|100000.00|");
  sSPrintF(str,L"|%5.2f|",10000.0f);                   CHECK(str==L"|10000.00|");
  sSPrintF(str,L"|%5.2f|",1000.0f);                    CHECK(str==L"|1000.00|");
  sSPrintF(str,L"|%5.2f|",100.0f);                     CHECK(str==L"|100.00|");
  sSPrintF(str,L"|%5.2f|",10.0f);                      CHECK(str==L"|10.00|");
  sSPrintF(str,L"|%5.2f|",1.0f);                       CHECK(str==L"| 1.00|");
  sSPrintF(str,L"|%5.2f|",0.1f);                       CHECK(str==L"| 0.10|");
  sSPrintF(str,L"|%5.2f|",0.01f);                      CHECK(str==L"| 0.01|");
  sSPrintF(str,L"|%5.2f|",0.001f);                     CHECK(str==L"| 0.00|");


  sSPrintF(str,L"|%s|",L"abc");                     CHECK(str==L"|abc|");
  sSPrintF(str,L"|%6s|",L"abc");                    CHECK(str==L"|   abc|");
  sSPrintF(str,L"|%-6s|",L"abc");                   CHECK(str==L"|abc   |");
  sSPrintF(str,L"|%s|",a);                          CHECK(str==L"|123|");
  sSPrintF(str,L"|%6s|",a);                         CHECK(str==L"|   123|");
  sSPrintF(str,L"|%-6s|",a);                        CHECK(str==L"|123   |");

  sVector30 v30; v30.Init(1.42f,1.52f,1.25f);
  sVector31 v31; v31.Init(1.42f,1.52f,1.25f);
  sVector4  v4;  v4 .Init(1.42f,1.52f,1.25f,1.26f);

  sSPrintF(str,L"|%6.3f|",v30);                     CHECK(str==L"|[ 1.420, 1.520, 1.250,0]|");
  sSPrintF(str,L"|%6.3f|",v31);                     CHECK(str==L"|[ 1.420, 1.520, 1.250,1]|");
  sSPrintF(str,L"|%6.3f|",v4);                      CHECK(str==L"|[ 1.420, 1.520, 1.250, 1.260]|");

  // buffer overflow tests

  five.PrintF(L"abcd");                   CHECK(five==L"abcd");
  five.PrintF(L"abcde");                  CHECK(five==L"abcde");
  five.PrintF(L"abcdef");                 CHECK(five==L"abcd?");
  five.PrintF(L"a%db",10);                CHECK(five==L"a10b");
  five.PrintF(L"a%db",100);               CHECK(five==L"a100b");
  five.PrintF(L"a%db",1000);              CHECK(five==L"a100?");
  five.PrintF(L"a%db%dc",1,2);            CHECK(five==L"a1b2c");
  five.PrintF(L"a%db%dc",10,2);           CHECK(five==L"a10b?");
  five.PrintF(L"a%db%dc",10,20);          CHECK(five==L"a10b?");
}

void TestWildcard()
{
  sCheckModule(L"sMatchWildcard()");

  // match

  CHECK(sMatchWildcard(L"abc",L"abc"));             
  CHECK(sMatchWildcard(L"?bc",L"abc"));
  CHECK(sMatchWildcard(L"a?c",L"abc"));
  CHECK(sMatchWildcard(L"ab?",L"abc"));
  CHECK(sMatchWildcard(L"*bcd",L"abcd"));
  CHECK(sMatchWildcard(L"a*cd",L"abcd"));
  CHECK(sMatchWildcard(L"ab*d",L"abcd"));
  CHECK(sMatchWildcard(L"abc*",L"abcd"));
  CHECK(sMatchWildcard(L"*cd",L"abcd"));
  CHECK(sMatchWildcard(L"a*d",L"abcd"));
  CHECK(sMatchWildcard(L"ab*",L"abcd"));
  CHECK(sMatchWildcard(L"*",L"abcd"));
  CHECK(sMatchWildcard(L"[*1*2*]",L"[xxx1yyy2zzz]"));
  CHECK(sMatchWildcard(L"[*1*2*]",L"[x1x1y1y2z1z]"));

  CHECK(sMatchWildcard(L"abcd*",L"abcd"));

  // wildcard on wrong side

  CHECK(!sMatchWildcard(L"abcd",L"*"));           
  CHECK(!sMatchWildcard(L"abc",L"a?c"));

  // no match

  CHECK(!sMatchWildcard(L"abc",L"1bc"));          
  CHECK(!sMatchWildcard(L"abc",L"a1c"));
  CHECK(!sMatchWildcard(L"abc",L"ab1"));
  CHECK(!sMatchWildcard(L"a*c",L"1bc"));
  CHECK(!sMatchWildcard(L"a*c",L"ab1"));
  CHECK(!sMatchWildcard(L"[*1*2*]",L"[xxx2yyy1zzz]"));
}

void TestStringPool()
{
  sCheckModule(L"StringPool");

  sString<64> buffer;
  sPoolString a[10000];
  sPoolString b;
  sRandom rnd;

  for(sInt i=0;i<sCOUNTOF(a);i++)
  {
    sSPrintF(buffer,L"%d",i);
    a[i] = buffer;
    CHECK(sCmpString(a[i],buffer)==0);
  }

  for(sInt i=0;i<sCOUNTOF(a);i++)
  {
    for(sInt j=0;j<100;j++)
    {
      sInt r = rnd.Int(10000);
      if((a[i]==a[r])==!!sCmpString(a[i],a[r]))
      {
        CHECK(0);
        return;
      }
    }
  }

  for(sInt i=0;i<100;i++)
  {
    sInt r = rnd.Int(10000);
    sSPrintF(buffer,L"%d",r);
    b = buffer;
    for(sInt j=0;j<sCOUNTOF(a);j++)
    {
      if((b==a[j])==!!sCmpString(b,a[j]))
      {
        CHECK(0);
        return;
      }
    }
  }
}

/****************************************************************************/

struct TAE
{
  sInt Symbol;
};

sBool ArrayEngine(const sChar *cmd)
{
  sArray<TAE *> a;
  TAE *e;
  sInt p;

  while(*cmd)
  {
    if(*cmd=='[' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new TAE;
      e->Symbol = *cmd++;
      a.AddHead(e);
    }
    if(*cmd==']' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new TAE;
      e->Symbol = *cmd++;
      a.AddTail(e);
    }
    else if(*cmd=='(' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='0'&&cmd[2]<='9')
    {
      cmd++;
      e = new TAE;
      e->Symbol = *cmd++;
      p = 0;
      while(*cmd>='0'&&*cmd<='9')
        p = p*10 + (*cmd++)-'0';
      a.AddBefore(e,p);
    }
    else if(*cmd==')' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='0'&&cmd[2]<='9')
    {
      cmd++;
      e = new TAE;
      e->Symbol = *cmd++;
      p = 0;
      while(*cmd>='0'&&*cmd<='9')
        p = p*10 + (*cmd++)-'0';
      a.AddAfter(e,p);
    }
    else if(*cmd=='=')
    {
      cmd++;
      sFORALL(a,e)
        if(e->Symbol!=*cmd++)
          return 0;
      if(*cmd!=0)
        return 0;
    }
    else if(*cmd=='-')
    {
      cmd++;
      if(*cmd>='0'&&*cmd<='9')
      {
        p = 0;
        while(*cmd>='0'&&*cmd<='9')
          p = p*10 + (*cmd++)-'0';
        e = a[p];
        a.RemAt(p);
        delete e;
      }
      else if(*cmd>='a'&&*cmd<='z')
      {
        e = sFind(a,&TAE::Symbol,*cmd++);
        sVERIFY(e);
        a.Rem(e);
        delete e;
      }
      else
      {
        sVERIFYFALSE;
      }
    }
    else
    {
      sVERIFYFALSE;
    }

  }
  sDeleteAll(a);
  return 1;
}

void TestArray()
{
  sCheckModule(L"sArray");

  CHECK(ArrayEngine(L"="));
  CHECK(ArrayEngine(L"]a=a"));
  CHECK(ArrayEngine(L"]a]b=ab"));
  CHECK(ArrayEngine(L"]a]b-0=b"));
  CHECK(ArrayEngine(L"]a]b-1=a"));
  CHECK(ArrayEngine(L"]a]b-0-0="));
  CHECK(ArrayEngine(L"]a]b-1-0="));

  CHECK(ArrayEngine(L"[a]b]c=abc"));
  CHECK(ArrayEngine(L"]b[a]c=abc"));
  CHECK(ArrayEngine(L"]b]c[a=abc"));

  CHECK(ArrayEngine(L"]a]b]c(x0=xabc"));
  CHECK(ArrayEngine(L"]a]b]c(x1=axbc"));
  CHECK(ArrayEngine(L"]a]b]c(x2=abxc"));
  CHECK(ArrayEngine(L"]a]b]c(x3=abcx"));
  CHECK(ArrayEngine(L"]a]b]c)x0=axbc"));
  CHECK(ArrayEngine(L"]a]b]c)x1=abxc"));
  CHECK(ArrayEngine(L"]a]b]c)x2=abcx"));

  // just check if the stack array does not crash or leak

  sStackArray<sInt,4> st1;
  st1.AddTail(11);
  st1.AddTail(12);
  st1.AddTail(13);
  st1.AddTail(14);
}

/****************************************************************************/

typedef struct testStruct_ {sInt i; sString<10> s; testStruct_(sInt i_, sString<10> s_) {i=i_;s=s_;};} testStruct;
void TestStringMap()
{
  sCheckModule(L"sStringMap");

  sStringMap<testStruct*,10> * map = new sStringMap<testStruct*,10>;

  testStruct * test;
  
  CHECK(map->Get(L"notset") == sNULL);

  for (sInt i = 0; i < 20; i++)
  {
    sString<10> kbuf; sSPrintF(kbuf, L"key%03d", i);
    sString<10> vbuf; sSPrintF(vbuf, L"value%03d", i);
    map->Set(kbuf, test = new testStruct(i,vbuf));
    CHECK(map->Get(kbuf) == test);
  }

  CHECK(map->GetCount() == 20);

  sStaticArray<sChar*> * keys = map->GetKeys();
  CHECK(keys->GetCount() == 20);
  sDelete(keys);

  sInt shuffle[20] = {13, 17, 4, 1, 12, 10, 8, 7, 3, 19, 2, 9, 11, 0, 14, 6, 15, 16, 5, 18};
  for (sInt i = 0; i < 20; i++)
  {
    sString<10> kbuf; sSPrintF(kbuf, L"key%03d", shuffle[i]);
    sString<10> vbuf; sSPrintF(vbuf, L"value%03d", shuffle[i]);

    test = map->Get(kbuf);
    CHECK(test->i == shuffle[i] && sCmpString(vbuf, test->s) == 0);
    delete test;
    map->Del(kbuf);

    CHECK(map->Get(kbuf) == sNULL);
    map->Del(kbuf);
  }

  map->Clear();

  for (sInt i = 0; i < 20; i++)
  {
    sString<10> kbuf; sSPrintF(kbuf, L"key%03d", i);
    CHECK(map->Get(kbuf) == sNULL);
  }

  delete map;
}

/****************************************************************************/

template <class ArrayType,class BaseType,class MemberType,class CompareType> 
BaseType *sFindX(ArrayType &a,MemberType BaseType::*o,const CompareType i)
{
  BaseType *e;
  sFORALL_LIST(a,e)
    if(e->*o == i)
      return e;
  return 0;
}
/****************************************************************************/

struct LN
{
  sInt Symbol;
  sDNode Node;
};

sBool DListEngine(const sChar *cmd)
{
  sDList<LN,&LN::Node> a;
  sDList<LN,&LN::Node> b;
  LN *e,*r;

  while(*cmd)
  {
    if(*cmd=='[' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      a.AddHead(e);
    }
    else if(*cmd==']' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      a.AddTail(e);
    }
    else if(*cmd=='>' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      b.AddTail(e);
    }
    else if(*cmd=='<' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      b.AddHead(e);
    }
    else if(*cmd=='(' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='a'&&cmd[2]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      r = sFindX(a,&LN::Symbol,*cmd++);
      a.AddBefore(e,r);
    }
    else if(*cmd==')' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='a'&&cmd[2]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      r = sFindX(a,&LN::Symbol,*cmd++);
      a.AddAfter(e,r);
    }
    else if(*cmd=='=')
    {
      cmd++;
      sFORALL_LIST(a,e)
        if(e->Symbol!=*cmd++)
          return 0;
      if(*cmd!=0)
        return 0;
    }
    else if(*cmd=='-' && cmd[1]>='a'&& cmd[1]<='z')
    {
      cmd++;
      e = sFindX(a,&LN::Symbol,*cmd++);
      sVERIFY(e);
      a.Rem(e);
      delete e;
    }
    else if(*cmd=='{') // addhead
    {
      cmd++;
      a.AddHead(b);
    }
    else if(*cmd=='}') // addtail (append)
    {
      cmd++;
      a.AddTail(b);
    }
    else
    {
      sVERIFYFALSE;
    }

  }

  while((e=a.RemHead())!=0)
    delete e;
  while((e=b.RemHead())!=0)
    delete e;
  return 1;
}

sBool DList2Engine(const sChar *cmd)
{
  sDList2<LN> a;
  sDList2<LN> b;
  LN *e,*r;

  while(*cmd)
  {
    if(*cmd=='[' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      a.AddHead(e);
    }
    else if(*cmd==']' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      a.AddTail(e);
    }
    else if(*cmd=='>' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      b.AddTail(e);
    }
    else if(*cmd=='<' && cmd[1]>='a'&&cmd[1]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      b.AddHead(e);
    }
    else if(*cmd=='(' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='a'&&cmd[2]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      r = sFindX(a,&LN::Symbol,*cmd++);
      a.AddBefore(e,r);
    }
    else if(*cmd==')' && cmd[1]>='a' && cmd[1]<='z' && cmd[2]>='a'&&cmd[2]<='z')
    {
      cmd++;
      e = new LN;
      e->Symbol = *cmd++;
      r = sFindX(a,&LN::Symbol,*cmd++);
      a.AddAfter(e,r);
    }
    else if(*cmd=='=')
    {
      cmd++;
      sFORALL_LIST(a,e)
        if(e->Symbol!=*cmd++)
          return 0;
      if(*cmd!=0)
        return 0;
    }
    else if(*cmd=='-' && cmd[1]>='a'&& cmd[1]<='z')
    {
      cmd++;
      e = sFindX(a,&LN::Symbol,*cmd++);
      sVERIFY(e);
      a.Rem(e);
      delete e;
    }
    else if(*cmd=='{') // addhead
    {
      cmd++;
      a.AddHead(b);
    }
    else if(*cmd=='}') // addtail (append)
    {
      cmd++;
      a.AddTail(b);
    }
    else
    {
      sVERIFYFALSE;
    }

  }

  while((e=a.RemHead())!=0)
    delete e;
  while((e=b.RemHead())!=0)
    delete e;
  return 1;
}


void TestDList()
{
  sCheckModule(L"sDList, sDList2");

  CHECK(DListEngine(L"="));
  CHECK(DListEngine(L"]a=a"));
  CHECK(DListEngine(L"]a]b=ab"));
  CHECK(DListEngine(L"]a]b-a=b"));
  CHECK(DListEngine(L"]a]b-b=a"));
  CHECK(DListEngine(L"]a]b-a-b="));
  CHECK(DListEngine(L"]a]b-b-a="));

  CHECK(DListEngine(L"[a]b]c=abc"));
  CHECK(DListEngine(L"]b[a]c=abc"));
  CHECK(DListEngine(L"]b]c[a=abc"));

  CHECK(DListEngine(L"]a]b]c(xa=xabc"));
  CHECK(DListEngine(L"]a]b]c(xb=axbc"));
  CHECK(DListEngine(L"]a]b]c(xc=abxc"));
  CHECK(DListEngine(L"]a]b]c)xa=axbc"));
  CHECK(DListEngine(L"]a]b]c)xb=abxc"));
  CHECK(DListEngine(L"]a]b]c)xc=abcx"));

  CHECK(DListEngine(L"]a>x}=ax"));
  CHECK(DListEngine(L"]a>x{=xa"));
  CHECK(DListEngine(L"]a]b>x>y}=abxy"));
  CHECK(DListEngine(L"]a]b>x>y{=xyab"));
  CHECK(DListEngine(L"]a]b}=ab"));
  CHECK(DListEngine(L"]a]b{=ab"));
  CHECK(DListEngine(L">x>y}=xy"));
  CHECK(DListEngine(L">x>y{=xy"));


  CHECK(DList2Engine(L"="));
  CHECK(DList2Engine(L"]a=a"));
  CHECK(DList2Engine(L"]a]b=ab"));
  CHECK(DList2Engine(L"]a]b-a=b"));
  CHECK(DList2Engine(L"]a]b-b=a"));
  CHECK(DList2Engine(L"]a]b-a-b="));
  CHECK(DList2Engine(L"]a]b-b-a="));

  CHECK(DList2Engine(L"[a]b]c=abc"));
  CHECK(DList2Engine(L"]b[a]c=abc"));
  CHECK(DList2Engine(L"]b]c[a=abc"));

  CHECK(DList2Engine(L"]a]b]c(xa=xabc"));
  CHECK(DList2Engine(L"]a]b]c(xb=axbc"));
  CHECK(DList2Engine(L"]a]b]c(xc=abxc"));
  CHECK(DList2Engine(L"]a]b]c)xa=axbc"));
  CHECK(DList2Engine(L"]a]b]c)xb=abxc"));
  CHECK(DList2Engine(L"]a]b]c)xc=abcx"));

  CHECK(DList2Engine(L"]a>x}=ax"));
  CHECK(DList2Engine(L"]a>x{=xa"));
  CHECK(DList2Engine(L"]a]b>x>y}=abxy"));
  CHECK(DList2Engine(L"]a]b>x>y{=xyab"));
  CHECK(DList2Engine(L"]a]b}=ab"));
  CHECK(DList2Engine(L"]a]b{=ab"));
  CHECK(DList2Engine(L">x>y}=xy"));
  CHECK(DList2Engine(L">x>y{=xy"));
}

/****************************************************************************/

sBool md5(sU32 h0,sU32 h1,sU32 h2,sU32 h3,const sChar8 *text)
{
  sChecksumMD5 md5;
  sInt i=0;
  while(text[i]) i++;

  md5.Calc((const sU8 *)text,i);
  if(md5.Hash[0]==h0 && md5.Hash[1]==h1 && md5.Hash[2]==h2 && md5.Hash[3]==h3)
    return 1;

  sString<256> s;
  sCopyString(s,text,256);
  sDPrintF(L"%08x %08x %08x %08x != %q\n",md5.Hash[0]==h0,md5.Hash[1]==h1,md5.Hash[2]==h2,md5.Hash[3],s);
  return 0;
}

void TestHash()
{
  sCheckModule(L"sChecksumMD5");

  // http://en.wikipedia.org/wiki/Md5

  CHECK(md5(0x9e107d9d,0x372bb682,0x6bd81d35,0x42a419d6,"The quick brown fox jumps over the lazy dog"));
  CHECK(md5(0xffd93f16,0x87604926,0x5fbaef4d,0xa268dd0e,"The quick brown fox jumps over the lazy eog"));
  CHECK(md5(0xd41d8cd9,0x8f00b204,0xe9800998,0xecf8427e,""));
}

/****************************************************************************/

sU32 hookmask;
void hook0a(sInt a,void *b,void *user) { hookmask |= 1; }
void hook1a(sInt a,void *b,void *user) { hookmask |= 2; }
void hook2a(sInt a,void *b,void *user) { hookmask |= 4; }
void hook3a(sInt a,void *b,void *user) { hookmask |= 8; }

void TestHooks()
{
  sCheckModule(L"sHooks");
  sHooks2<sInt,void *> hook;
  hook.Add(hook0a,0);
  hook.Add(hook1a,0);
  hook.Add(hook2a,0);
  hook.Add(hook3a,0);
  hook.Rem(hook2a);

  hookmask = 0;
  hook.Call(1,0);
  CHECK(hookmask==1+2+8);
}

/****************************************************************************/

void testwrite(sWriter &stream)
{
  sRandom rnd;
  sU32 data[8] = { 3,5,7,11,13,17,19,23 };
  sU32 *p1=data+1,*p2=data+2,*p3=data+3,*p4=0;
  sInt r1,r2,r3,r4;

  // basic test

  stream.Header(0xbaadf00d,1);
  stream.U32(0x12345678);
  stream.ArrayU32(data,8);

  // pointer test

  r1 = stream.RegisterPtr(p1);
  r2 = stream.RegisterPtr(p2);
  r3 = stream.RegisterPtr(p3); 
  r4 = stream.RegisterPtr(p4);

  stream.Ptr(p1);
  stream.Ptr(p2);
  stream.Ptr(p3);
  stream.Ptr(p4);
  stream.Ptr(p1);
  stream.Ptr(p4);
  stream.Ptr(p3);

  // mass test

  for(sInt i=0;i<1021;i++)
  {
    stream.Check();
    for(sInt j=0;j<1027;j++)
      stream.U32(rnd.Int32());
  }

  // done

  stream.Footer();
}

void testread(sReader &stream)
{
  sRandom rnd;
  sU32 data[8] = { 3,5,7,11,13,17,19,23 };
  sU32 buffer[8];
  sU32 u32;
  sU32 *p1=data+1,*p2=data+2,*p3=data+3,*p4=0;
  sU32 *q;

  // basic test

  CHECK(stream.Header(0xbaadf00d,1));
  stream.U32(u32);  CHECK(u32==0x12345678);
  stream.ArrayU32(buffer,8); CHECK(sCmpMem(data,buffer,8*4)==0);
  
  // pointer test

  stream.RegisterPtr(p1);
  stream.RegisterPtr(p2);
  stream.RegisterPtr(p3);
  stream.RegisterPtr(p4);

  stream.Ptr(q); CHECK(q==p1);
  stream.Ptr(q); CHECK(q==p2);
  stream.Ptr(q); CHECK(q==p3);
  stream.Ptr(q); CHECK(q==p4);
  stream.Ptr(q); CHECK(q==p1);
  stream.Ptr(q); CHECK(q==p4);
  stream.Ptr(q); CHECK(q==p3);

  // mass test

  for(sInt i=0;i<1021;i++)
  {
    stream.Check();
    for(sInt j=0;j<1027;j++)
    {
      stream.U32(u32);
      if(u32!=rnd.Int32())
      {
        CHECK(0);
        return;
      }
    }
  }

  // done

  stream.Footer();
}

void TestSerialize()
{
  sReader rd1,rd2;
  sWriter wr;
  sFile *file;

  sCheckModule(L"Serialize");
#if sPLATFORM==sPLAT_LINUX
  #define FILENAME L"~/test.bin"
#else
  #define FILENAME L"c:/temp/test.bin"
#endif

  file = sCreateFile(FILENAME,sFA_WRITE);
  CHECK(file);
  if(file)
  { 
    wr.Begin(file);
    testwrite(wr);
    CHECK(wr.End());
    CHECK(file->Close());
  }
  delete file;

  file = sCreateFile(FILENAME,sFA_READ);
  CHECK(file);
  if(file)
  { 
    rd1.Begin(file);
    testread(rd1);
    CHECK(rd1.End());
    CHECK(file->Close());
  }
  delete file;

  file = sCreateFile(FILENAME,sFA_READ);
  CHECK(file);
  if(file)
  { 
    rd2.DontMap = 1;
    rd2.Begin(file);
    testread(rd2);
    CHECK(rd2.End());
    CHECK(file->Close());
  }
  delete file;
}

/****************************************************************************/

// there seems to be a problem with writing large files on windows.

void TestFileNetwork()
{
  const sChar *filename = L"\\\\toaster4\\user\\dierk\\dump.bin";
  sRandomKISS rnd;

  sCheckModule(L"sFile (on network)");

  sU32 *data,*datar;
  sDInt sizer;
  sInt ok;

  for(sDInt size=1*1024*1024;size<256*1024*1024;size=size*2)
  {
    rnd.Seed(size);
    data = new sU32[size/4];
    for(sInt i=0;i<size/4;i++)
      data[i] = rnd.Int32();
    CHECK(sSaveFile(filename,data,size));

    rnd.Seed(size);
    datar = (sU32 *) sLoadFile(filename,sizer);
    CHECK(datar);
    CHECK(size==sizer);
    ok = 1;
    if(datar)
    {
      for(sInt i=0;i<size/4;i++)
        if(datar[i]!=rnd.Int32())
          ok = 0;
    }
    CHECK(ok);
    delete[] data;
    delete[] datar;
  }
}

void TestFile()
{
  const sChar *filename = L"c:\\dump.bin";
  sRandomKISS rnd;

  sCheckModule(L"sFile");

  sU32 *data,*datar;
  sDInt sizer;
  sInt ok;

  for(sDInt size=1*1024*1024;size<256*1024*1024;size=size*2)
  {
    rnd.Seed(size);
    data = new sU32[size/4];
    for(sInt i=0;i<size/4;i++)
      data[i] = rnd.Int32();
    CHECK(sSaveFile(filename,data,size));

    rnd.Seed(size);
    datar = (sU32 *) sLoadFile(filename,sizer);
    CHECK(datar);
    CHECK(size==sizer);
    ok = 1;
    if(datar)
    {
      for(sInt i=0;i<size/4;i++)
        if(datar[i]!=rnd.Int32())
          ok = 0;
    }
    CHECK(ok);
    delete[] data;
    delete[] datar;
  }
}

void TestFileSystem()
{
  const sChar *dirname0 = L"c:/altona_test_dir";
  const sChar *dirname1 = L"c:/altona_test_dir/subdir";
  const sChar *filename = L"c:/altona_test_dir/subdir/bla.bin";
  sRandomKISS rnd;

  sCheckModule(L"FileSystem");

  CHECK(sCheckDir(dirname0)==0);
  CHECK(sMakeDirAll(dirname1));
  CHECK(sCheckDir(dirname0));
  CHECK(sCheckDir(dirname1));
  CHECK(sSaveFile(filename,L"bla",4));
  CHECK(sCheckFile(filename));
  CHECK(sDeleteFile(filename));
  CHECK(sCheckFile(filename)==0);
  CHECK(sDeleteDir(dirname1));
  CHECK(sDeleteDir(dirname0));
  CHECK(sCheckDir(dirname0)==0);
}

void TestUTF8()
{
  const sChar *filename = L"c:/test.bin";
  sCheckModule(L"UTF8");

  sChar *text = new sChar[0x10000];
  for(sInt i=0;i<0x10000;i++)
    text[i] = i+1;
  for(sInt i=0;i<0x1f;i++)
    text[i] = ' ';

  CHECK(sSaveTextUTF8(filename,text));
  sChar *read = sLoadText(filename);
  CHECK(read);
  sBool ok = 1;
  for(sInt i=0;i<0x10000;i++)
    ok &= (read[i]==text[i]);
  CHECK(ok);
  delete[] text;
  delete[] read;
}

/****************************************************************************/

void TestMemoryHeap()
{
  sMemoryHeap h0;
  sGpuHeap h1;
  sRandom rnd;
  sInt percent;

  const sInt heapsize = 1024*1024;
  const sInt blocksize = 1024;
  const sInt blockcount = 1024;
  const sInt allocs = 16*1024;

  sInt used = 0;
  sU8 *mem = new sU8[heapsize];
  sU8 *blocks[blockcount];

  sCheckModule(L"sMemoryHeap");

  h0.Init(mem,heapsize);
  h0.SetDebug(1,0);
  h1.Init(mem,heapsize,allocs*2);

//  for(percent=20;percent<80;percent+=10)
  percent = 40;
  {
    used = 0;
    for(sInt i=0;i<allocs;i++)
    {
      if((rnd.Int(100)<percent && used>0) || used>=blockcount)
      {
        sInt n = rnd.Int(used);
        h0.Free(blocks[n]);
        blocks[n] = blocks[--used];
      }
      else
      {
        blocks[used] = (sU8 *)h0.Alloc(rnd.Int(blocksize-1)+1,1<<rnd.Int(10),rnd.Int(3)==0);
        if(blocks[used])
          used++;
      }

      h0.Validate();
    }
//    h0.DumpStats();
    for(sInt i=0;i<used;i++)
      h0.Free(blocks[i]);
    CHECK(h0.GetUsed()==0);
  }

//  for(percent=20;percent<80;percent+=10)
  percent = 40;
  {
    used = 0;
    for(sInt i=0;i<allocs;i++)
    {
      if((rnd.Int(100)<percent && used>0) || used>=blockcount)
      {
        sInt n = rnd.Int(used);
        h1.Free(blocks[n]);
        blocks[n] = blocks[--used];
      }
      else
      {
        blocks[used] = (sU8 *)h1.Alloc(rnd.Int(blocksize-1)+1,1<<rnd.Int(10),rnd.Int(3)==0);
        if(blocks[used])
          used++;
      }

      h1.Validate();
    }
//    h1.DumpStats();
    for(sInt i=0;i<used;i++)
      h1.Free(blocks[i]);
    CHECK(h1.GetUsed()==0);
  }

  delete [] mem;
}


/****************************************************************************/
/***                                                                      ***/
/***   Special Tests                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void tf(sThread *t,void *c)
{
  sFloatInfo fi;
  sInt id = *(sInt *)c;

  sInt errors=0;
  for(sU64 i=0;i<0x20000000;i+=0x1)
  {
    sU32 un = i+id*0x20000000;
//    sF32 n = *(sF32 *)&un;

    if((i&0xffffff)==0) sDPrintF(L"%d : %08x.\n",id,un);

    fi.FloatToAscii(un,9);
    sU32 um = fi.AsciiToFloat();
    if(um!=un)
    {
      sString<256> buf;
      sInt diff = sAbs(sInt(um)-sInt(un));
      fi.PrintE(buf);
      sDPrintF(L"%08x -> %s -> %08x   err %3d %s  \n",un,buf,um,diff,diff>2 ? L" ******" : L"");

//      fi.FloatToAscii(un,9); 
//      fi.PrintE(buf);    
//      sDPrintF(L"%s  %08x\n",buf,fi.AsciiToFloat()); 
//      sDPrintF(L"\n");

      errors++;
      if(errors>100) 
      {
        sDPrintF(L"break at %08x\n",i);
        break;
      }
    }
  }
  sDPrintF(L"%d: errors %d\n",id,errors);
  t->Terminate();
}

void td(sThread *t,void *c)
{
  sFloatInfo fi;
  sInt id = *(sInt *)c;
  sRandom rnd;
  rnd.Seed(id);

  sInt errors=0;
  for(sU64 i=0;i<0x20000000;i+=0x100)
  {
    sU64 un = rnd.Int32();
    un = (sU64(i+id*0x20000000)<<32)|rnd.Int32();
//    sF64 n = *(sF64 *)&un;

    if((i&0xffffff)==0) sDPrintF(L"%d : %016x.\n",id,un);

    fi.DoubleToAscii(un,17);
    sU64 um = fi.AsciiToDouble();
    sInt diff = sInt(um-un);
    if(diff<0) diff = -diff;

    if(diff>2)
    {
      sString<256> buf;
      sDPrintF(L"");
      fi.PrintE(buf);
      sDPrintF(L"%016x -> %s -> %016x   err %3d %s  \n",un,buf,um,diff,diff>2 ? L" ******" : L"");

      errors++;
      if(errors>100) 
      {
        sDPrintF(L"break at %08x\n",i);
        break;
      }
    }
  }
  sDPrintF(L"%d: errors %d\n",id,errors);
  t->Terminate();
}

void TestFloatAscii()
{
  sThread *t[4];
  sInt id[4];

  for(sInt i=0;i<4;i++)
  {
    id[i] = i;
    t[i] = new sThread(tf,0,16*1024,&id[i],0);
  }

  sBool running = 1;
  while(running)
  {
    sSleep(1000);
    running = 0;
    for(sInt i=0;i<4;i++)
      if(t[i]->CheckTerminate())
        running = 1;
  }
}

void TestDoubleAscii()
{
  sThread *t[4];
  sInt id[4];

  for(sInt i=0;i<4;i++)
  {
    id[i] = i;
    t[i] = new sThread(td,0,16*1024,&id[i],0);
  }

  sBool running = 1;
  while(running)
  {
    sSleep(1000);
    running = 0;
    for(sInt i=0;i<4;i++)
      if(t[i]->CheckTerminate())
        running = 1;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Random                                                             ***/
/***                                                                      ***/
/****************************************************************************/

template<class Rand> void TestRandom(const sChar *name)
{
  Rand rnd;
  sInt ok;

  sCheckModule(name);
  rnd.Seed(0);

  
  // test random numbers between 0..3 (Int(4))
  sInt randomNumber = 0;
  sInt numBins[4];
  sClear(numBins);
  sInt cSwitched;
  cSwitched = 0;
  //sDPrintF(L"Test random numbers: Call Int(4) ...\n");
  const sInt numbers = 4000;//4000;
  for (sU32 i0=0; i0<numbers; ++i0)
  {
    sInt randNo = rnd.Int(4);
    if (randNo != randomNumber)
    {
      ++cSwitched;
    }
    randomNumber = randNo;
    ++numBins[randNo];
    //sDPrintF(L"i=%d, rno=%d\n", i0, randNo);
  }
  for (sU32 i=0; i<4; ++i)
  {
    sF32 ratio = 4.0f * sF32(numBins[i])/sF32(numbers);
    CHECK(ratio>0.95f || ratio<1.05f);
    //sDPrintF(L"%d ratio: %f\n", i, ratio);
  }
  // statistics
  sF32 switchRatio = sF32(cSwitched)/sF32(numbers);
  sF32 idealValue = 0.75f;
  sF32 deviation = idealValue * 0.05; // 5 percent deviation
  CHECK((switchRatio > (idealValue - deviation))
    || (switchRatio < (idealValue + deviation)));
  //sDPrintF(L"0: %d, 1: %d, 2: %d, 3: %d, no. switched: %f\n", numBins[0], numBins[1], numBins[2], numBins[3], sF32(cSwitched)/sF32(numbers));
  // end test

  
  // check Int(max)

  sInt val;
  rnd.Int(128);
  sInt bins[128];
  for(sInt i=1;i<sCOUNTOF(bins);i++)
  {
    sClear(bins);
    for(sInt j=0;j<50000;j++)
    {
      val = rnd.Int(i);
      if(val<0 || val>=i)
      {
        CHECK(0);
        goto nexttest;
      }
      bins[val]++;
    }

    sInt s5=0;
    sInt s10=0;
    sInt s15=0;
    for(sInt k=0; k<i; k++)
    {
      sF32 ratio = sF32(bins[k])/(sF32(50000)/i);
      
      if(ratio<0.95f || ratio>1.05f) s5++;
      if(ratio<0.90f || ratio>1.10f) s10++;
      if(ratio<0.85f || ratio>1.15f) s15++;
    }
//    sDPrintF(L"%3d:%3d %3d %3d\n",i,s15,s10,s5);
    if(s5>60 || s10>15 || s15>2)
    {
      CHECK(0);
      break;
    }
  }
nexttest:

  // check if bits are random

  sInt bits[32];
  sClear(bits);
  for(sInt i=0;i<50000;i++)
  {
    sU32 val = rnd.Int32();
    for(sInt j=0;j<32;j++)
    {
      if(val&0x80000000)
        bits[j]++;
      val = val+val;
    }
  }
  ok = 1;
  for(sInt i=0;i<32;i++)
  {
    if(bits[i]<24500 || bits[i]>25500)
      ok = 0;
  }

  CHECK(ok);
}

/****************************************************************************/
/***                                                                      ***/
/***   Random                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sU64 Readout(sBitVector &bv)
{
  sU64 val = 0;
  for(sInt i=0;i<64;i++)
    val |= sU64(bv.Get(i))<<sU64(i);
  return val;
}

void TestBitVector()
{
  sCheckModule(L"sBitVector");

  sBitVector v0,v1,v2,v3,v4,v5,v6,v7;

  // check clearall / setall

  CHECK(Readout(v0)==0);
  v0.ClearAll();
  CHECK(Readout(v0)==0);
  v0.SetAll();
  CHECK(Readout(v0)==~0ULL);

  // simple checks

  v1.Set(0);
  v1.Set(1);
  v1.Set(2);
  v1.Set(3);
  CHECK(Readout(v1)==0x0f);
  v1.Set(7);
  CHECK(Readout(v1)==0x8f);
  v1.Set(128);
  v1.Set(130);
  CHECK(Readout(v1)==0x8f);

  // check growing

  CHECK(v1.Get(127)==0);
  CHECK(v1.Get(128)==1);
  CHECK(v1.Get(129)==0);
  CHECK(v1.Get(130)==1);
  CHECK(v1.Get(131)==0);
  v1.Clear(130);
  CHECK(v1.Get(127)==0);
  CHECK(v1.Get(128)==1);
  CHECK(v1.Get(129)==0);
  CHECK(v1.Get(130)==0);
  CHECK(v1.Get(131)==0);
  CHECK(v1.Get(256)==0);
  CHECK(v1.Get(512)==0);
  CHECK(v1.Get(1337)==0);
  v1.Set(1337);
  CHECK(v1.Get(1337)==1);
  v1.Assign(1337,0);
  CHECK(v1.Get(1337)==0);
  v1.Assign(1337,1);
  CHECK(v1.Get(1337)==1);

  // check iterating

  v2.Set(10);
  v2.Set(24);
  v2.Set(25);
  v2.Set(26);
  v2.Set(1337);
  v2.Set(415);
  v2.Set(256);
  v2.Set(511);
  v2.Set(512);
  v2.Set(513);

  sInt n = -1;
  CHECK(v2.NextBit(n));   CHECK(n==10);
  CHECK(v2.NextBit(n));   CHECK(n==24);
  CHECK(v2.NextBit(n));   CHECK(n==25);
  CHECK(v2.NextBit(n));   CHECK(n==26);
  CHECK(v2.NextBit(n));   CHECK(n==256);
  CHECK(v2.NextBit(n));   CHECK(n==415);
  CHECK(v2.NextBit(n));   CHECK(n==511);
  CHECK(v2.NextBit(n));   CHECK(n==512);
  CHECK(v2.NextBit(n));   CHECK(n==513);
  CHECK(v2.NextBit(n));   CHECK(n==1337);
  CHECK(!v2.NextBit(n));

  // check sBitVector::Assig()

  v3.Assign(0,1);
  v3.Assign(1,1);
  v3.Assign(2,1);
  v3.Assign(3,1);
  CHECK(Readout(v3)==0x0f);
  v3.Assign(2,0);
  v3.Assign(4,0);
  CHECK(Readout(v3)==0x0b);
}


sU64 Readout(sStaticBitArray<64> &bv)
{
  sU64 val = 0;
  for(sInt i=0;i<64;i++)
    val |= sU64(bv.Get(i))<<sU64(i);
  return val;
}

void TestBitArray()
{
  sCheckModule(L"sStaticBitArray");

  sStaticBitArray<64> v0,v1,v2,v3,v4,v5,v6,v7;

  // check clearall / setall

  CHECK(Readout(v0)==0);
  v0.ClearAll();
  CHECK(Readout(v0)==0);
  v0.SetAll();
  CHECK(Readout(v0)==~0ULL);

  // simple checks

  v1.Set(0);
  v1.Set(1);
  v1.Set(2);
  v1.Set(3);
  CHECK(Readout(v1)==0x0000000fU);
  v1.Set(7);
  CHECK(Readout(v1)==0x0000008fU);
  v1.Set(8);
  v1.Set(30);
  v1.Set(31);
  CHECK(Readout(v1)==0xc000018fU);
  v1.Clear(1);
  v1.Clear(30);
  CHECK(Readout(v1)==0x8000018dU);

  // check sBitVector::Assig()

  v3.Assign(0,1);
  v3.Assign(1,1);
  v3.Assign(2,1);
  v3.Assign(3,1);
  CHECK(Readout(v3)==0x0f);
  v3.Assign(2,0);
  v3.Assign(4,0);
  CHECK(Readout(v3)==0x0b);
}


/****************************************************************************/
/***                                                                      ***/
/***   FindEuler                                                          ***/
/***                                                                      ***/
/****************************************************************************/

void TestFindEuler()
{
  sRandomMT rnd;
  sF32 rx,ry,rz;
  sF32 qx,qy,qz;
  sMatrix34 mat0;
  sMatrix34 mat1;
  sCheckModule(L"sFindEulerXYZ");

  
  for(sInt i=0;i<10000;i++)
  {
    rx = (rnd.Float(2)-1)*(sPIF-0.01f);
    ry = (rnd.Float(2)-1)*(sPIF/2-0.01f); // 180° only. the other 180° are redundant
    rz = rnd.Float(1)*(sPIF/2-0.01f);     // 180° only. the other 180° are upside down

    mat0.EulerXYZ(rx,ry,rz);
    mat0.FindEulerXYZ2(qx,qy,qz);
    mat1.EulerXYZ(qx,qy,qz);

    sF32 delta = 0;
    for(sInt i=0;i<12;i++)
      delta += sAbs( (&mat0.i.x)[i]-(&mat1.i.x)[i] );
    CHECK(delta<0.01f);
    CHECK(sAbs(rx-qx)<0.01);
    CHECK(sAbs(ry-qy)<0.01);
    CHECK(sAbs(rz-qz)<0.01);
  }

  for(sInt i=0;i<10000;i++)
  {
    rx = rnd.Float(1)*sPI2F-sPIF;
    ry = rnd.Float(1)*sPI2F-sPIF;
    rz = rnd.Float(1)*sPI2F-sPIF;

    mat0.EulerXYZ(rx,ry,rz);
    mat0.FindEulerXYZ(qx,qy,qz);
    mat1.EulerXYZ(qx,qy,qz);

    sF32 delta = 0;
    for(sInt i=0;i<12;i++)
      delta += sAbs( (&mat0.i.x)[i]-(&mat1.i.x)[i] );
    CHECK(delta<0.02f);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   sMain                                                              ***/
/***                                                                      ***/
/****************************************************************************/

sINITMEM(sIMF_DEBUG|sIMF_NORTL,512*1024*1024);

void sMain()
{
  ErrorCount = 0;

  sPrintF(L"/****************************************************************************/\n");
  sPrintF(L"/***                                                                      ***/\n");
  sPrintF(L"/***   Starting Test                                                      ***/\n");
  sPrintF(L"/***                                                                      ***/\n");
  sPrintF(L"/****************************************************************************/\n");

  sPrintF(L"\n");

  if(0)     // very slow tests
  {
    TestFloatAscii();
    TestDoubleAscii();
    TestRandom<sRandom>(L"sRandom");
    TestRandom<sRandomMT>(L"sRandomMT");
    TestRandom<sRandomKISS>(L"sRandomKISS");
    TestMemoryHeap();
//    TestFileNetwork();
    TestFile();
  }


  TestFindEuler();
  TestString();
  TestSimpleFunctions();
  TestMem();
  TestStringFunctions();
  TestWildcard();
  TestFloat();
  TestFormat();
  TestStringPool();
  TestArray();
  TestHooks();
  TestStringMap();
  TestDList();
  TestHash();
  TestFileSystem();
  TestUTF8();
  TestSerialize();
  TestBitVector();
  TestBitArray();

  sPrintF(L"\n");

  if(ErrorCount!=0)
  {
    sPrintF(L"/****************************************************************************/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/***   there were %4d errors!                                            ***/\n",ErrorCount);
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/****************************************************************************/\n");
  }
  else
  {
    sPrintF(L"/****************************************************************************/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/***   all ok                                                             ***/\n");
    sPrintF(L"/***                                                                      ***/\n");
    sPrintF(L"/****************************************************************************/\n");
  }
}

/****************************************************************************/

