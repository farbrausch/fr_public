/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_UTIL_JSON_HPP
#define FILE_UTIL_JSON_HPP

#include "base/types.hpp"
#include "base/types2.hpp"
#include "util/scanner.hpp"

/****************************************************************************/

class sJSON
{
public:
  enum DataType
  {
    NUMBER,
    STRING,
    ARRAY,
    OBJECT,
  };

  class TypeBase
  {
  public:
    TypeBase(DataType dt) : Type(dt) {}
    virtual ~TypeBase() {}
    const DataType Type;

    template<class T> T* As() 
    {
      if (!this) return 0;
      if (Type != T::MyType) return 0;
      return (T*)this;
    }
  };

  class Number : public TypeBase
  {
  public:
    enum { MyType=NUMBER, };
    Number() : TypeBase(NUMBER) {}
    sF64 Value;
  };

  class String : public TypeBase
  {
  public:
    enum { MyType=STRING, };
    String() : TypeBase(STRING) {}
    sPoolString Value;
  };

  class Array : public TypeBase
  {
  public:
    enum { MyType=ARRAY, };
    Array() : TypeBase(ARRAY) {}
    ~Array() { sDeleteAll(Items); } 
    sArray<TypeBase*> Items;
  };

  class Object : public TypeBase
  {
  public:
    enum { MyType=OBJECT, };
    Object() : TypeBase(OBJECT) {}
   
    struct ItemList
    {
      struct Item
      {
        sPoolString Name;
        TypeBase *Value;
        sDNode Node;

        Item() : Value(0) {}
        ~Item() { delete Value; }
      };

      ~ItemList() { sDeleteAll(Items); }
       
      sDList<Item,&Item::Node> Items;

      TypeBase *Get(const sChar *name)
      {
        Item *i;
        sFORALL_LIST(Items, i)
        {
          if (!sCmpString(i->Name,name))
            return i->Value;
        }
        return 0;
      }

    };

    ItemList Items, Attributes;
  };


  TypeBase *Parse(const sChar *text)
  {
    sScanner scan;
    scan.DefaultTokens();
    scan.AddTokens(L"{[]},.-;=:");
    scan.Start(text);

    return ParseR(scan);
  }

  template<class T> static T* As(TypeBase *obj)
  {
    if (obj==0) return 0;
    if (obj->Type != T::MyType) return 0;
    return (T*)obj;
  }

private:
  TypeBase *ParseR(sScanner &scan)
  {
    switch (scan.Token)
    {
    case sTOK_INT:
      {
        Number *n = new Number;
        n->Value = scan.ScanInt();
        return n;
      }
    case sTOK_FLOAT:
      {
        Number *n = new Number;
        n->Value = scan.ScanFloat();
        return n;
      }  
    case sTOK_STRING:
      {
        String *s = new String;
        scan.ScanString(s->Value);
        return s;
      }  
    case '[':
      {
        scan.Scan();
        Array *a = new Array;
        while (!scan.Errors && scan.Token != sTOK_END && scan.Token!=']')
        {
          a->Items.AddTail(ParseR(scan));
          scan.IfToken(',');
        }
        scan.Match(']');
        return a;
      }
    case '{':
      {
        scan.Scan();
        Object *o = new Object;
        while (!scan.Errors && scan.Token != sTOK_END && scan.Token!='}')
        {
          sPoolString name;
          scan.ScanString(name);
          scan.Match(':');
          Object::ItemList::Item *item = new Object::ItemList::Item;
          item->Name = name;
          item->Value = ParseR(scan);
          if (name == L"@attributes") // for php XML->JSON translation
          {
            Object *o2 = item->Value->As<Object>();
            while (!o2->Items.Items.IsEmpty()) 
              o->Attributes.Items.AddTail(o2->Items.Items.RemHead());
            delete item;
          }
          else
            o->Items.Items.AddTail(item);
          scan.IfToken(',');
        }
        scan.Match('}');
        return o;
      }
    }
    scan.Error(L"syntax error");
    return 0;
  }

};


/****************************************************************************/

#endif // FILE_UTIL_JSON_HPP

