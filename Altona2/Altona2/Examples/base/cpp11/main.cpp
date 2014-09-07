/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "altona2/libs/base/base.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

template<typename T>
T *begin(sArray<T> &a) 
{
  return a.GetData();
}

template<typename T>
T *end(sArray<T> &a) 
{
  return a.GetData()+a.GetCount();
}

template<typename T>
int GetIndex(sArray<T> &a,T &i)
{
  return &i-a.GetData();
}

enum class Color : sU32 { Red,Green,Blue };
enum class Flags : sU32 { A=1,B=2,C=4 };

void Altona2::Main()
{
  // types

  int i = 2;
  auto j = 2*i;
  decltype(j) k = 3*j;
  sPrintF("2*2*3=%d\n",k);

  // enums

  Color c = Color::Red;
  /*  Flags f = Flags::A | Flags::B; */ // does not work, therefore scoped enums are useless.

  // ranged for, lambdas

  sArray<int> a;
  a.Add(2);
  a.Add(3);
  a.Add(5);
  a.Add(7);

  for(auto &i : a)
    sPrintF("%d:%d\n",GetIndex(a,i),i);
  sPrintF("\n");

  a.RemIf([](int &i){return i==3;});

  for(auto &i : a)
    sPrintF("%d:%d\n",GetIndex(a,i),i);
  sPrintF("\n");

  sArray<char *> ap;
  ap.Add("a");
  ap.Add("b");
  ap.Add("c");
  ap.Add("d");

  for(auto &s : ap)
    sPrintF("%d:%s\n",GetIndex(ap,s),s);
  sPrintF("\n");

  ap.RemIf([](char *i){return i[0]=='b';});

  for(auto &s : ap)
    sPrintF("%d:%s\n",GetIndex(ap,s),s);
  sPrintF("\n");
}


/****************************************************************************/

