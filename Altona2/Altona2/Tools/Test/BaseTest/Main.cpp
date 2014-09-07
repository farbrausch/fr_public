/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/Containers.hpp"
#include "Altona2/Libs/Util/Compression.hpp"
#include "Altona2/Libs/Base/Types2.hpp"

#include <vector>

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***   Test Base                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#define CHECK(x) Check(x,__FILE__,__LINE__);
#define FAIL(x)  x;

sInt AllErrors;
sInt AllErrorSections;
sInt Errors;
sInt InSection;

void Begin(const sChar *name)
{
    sASSERT(!InSection);
    InSection = 1;
    Errors = 0;
    sPrintF("Section %s",name);
}

void Check(sBool ok,const sChar *file,sInt line)
{
    sASSERT(InSection);
    if(!ok)
    {
        Errors++;
        sDPrintF("%s(%d): fail\n",file,line);
    }
}

void End()
{
    sASSERT(InSection);
    InSection = 0;
    if(Errors==0)
    {
        sPrintF(" - OK\n");
    }
    else
    {
        AllErrorSections++;
        sPrintF(" - %d Errors\n",Errors);
    }
    AllErrors += Errors;
    Errors = 0;
}

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   The Tests                                                          ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

struct Test
{
    const char *name;
    Test() { name = "???"; sDPrintF("construct %q\n",name); }
    Test(const char *n) { name = n; sDPrintF("construct %q\n",name); }
    Test(const Test &e) { name = e.name; if(*name) name++; sDPrintF("construct %q\n",name); }
    ~Test() { sDPrintF("destruct %q\n",name); }
    Test &operator=(const Test &e) { name = e.name; sDPrintF("assign\n"); return *this; }
    Test &operator=(Test &&e) { sSwap(name,e.name); sDPrintF("move\n");  return *this; }
};

__declspec(noinline) void Playground()
{
    Begin("Playground");
    sDPrintF("\n");

    {
        sDPrintF("<\n");
        Test q("qqq");
        {
            sDPrintF("<\n");
            sDynArray<Test> n;
            n.Add(Test("aaa"));
            n.Add(Test("bbb"));
            n.RemAt(0);
            n.Add(q);
            sDPrintF(">\n");
        }
        sDPrintF(">\n");
    }
    sDPrintF("x\n");

    End();
}

/****************************************************************************/
/***                                                                      ***/
/***   Containers                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void SList()
{
    struct Node
    {
        Node *Next;
        sChar c;

        Node(sChar c_) { c=c_; Next=0; }
        static sBool CheckList(const sSList<Node> &l,const sChar *str)
        {
            Node *n = l.GetFirst();
            while(n && *str)
            {
                if(n->c != *str) return 0;
                n = n->Next;
                str++;
            }
            if(n!=0) return 0;
            if(*str!=0) return 0;
            return 1;
        }
    };

    Begin("sSList<T>");

    sSList<Node> l;
    Node a('a'),b('b'),c('c'),d('d');

    l.Clear();
    CHECK(Node::CheckList(l,""));
    l.AddTail(&a);
    l.AddTail(&b);
    l.AddTail(&c);
    CHECK(Node::CheckList(l,"abc"));      // check if list is assembled correctly
    CHECK(!Node::CheckList(l,"acb"));     // check if the checking actually works
    CHECK(!Node::CheckList(l,"ab"));
    CHECK(!Node::CheckList(l,"abcd"));

    l.RemHead();
    CHECK(Node::CheckList(l,"bc"));
    l.RemHead();
    CHECK(Node::CheckList(l,"c"));
    l.RemHead();
    CHECK(Node::CheckList(l,""));         // removing last element is tricky!
    l.AddTail(&d);
    l.AddTail(&a);
    CHECK(Node::CheckList(l,"da"));
    l.RemHead();
    l.AddTail(&b);
    CHECK(Node::CheckList(l,"ab"));

    l.Clear();                            // addhead on first is tricky
    l.AddHead(&b);
    l.AddTail(&c);
    l.AddHead(&a);
    l.AddTail(&d);
    CHECK(Node::CheckList(l,"abcd"));

    l.Clear();                            // check the forall
    l.AddTail(&a);
    l.AddTail(&b);
    l.AddTail(&c);
    Node *p;
    const sChar *str="abc";
    sFORLIST(l,p)
        CHECK(p->c==*str++);
    CHECK(*str==0);

    End();
}

/****************************************************************************/

void SListRev()
{
    struct Node
    {
        Node *Next;
        sChar c;

        Node(sChar c_) { c=c_; Next=0; }
        static sBool CheckList(const sSListRev<Node> &l,const sChar *str)
        {
            Node *n = l.GetFirst();
            while(n && *str)
            {
                if(n->c != *str) return 0;
                n = n->Next;
                str++;
            }
            if(n!=0) return 0;
            if(*str!=0) return 0;
            return 1;
        }
    };

    Begin("sSListRev<T>");

    sSListRev<Node> l;
    Node a('a'),b('b'),c('c'),d('d');

    l.Clear();
    CHECK(Node::CheckList(l,""));
    l.AddHead(&c);
    l.AddHead(&b);
    l.AddHead(&a);
    CHECK(Node::CheckList(l,"abc"));      // check if list is assembled correctly
    CHECK(!Node::CheckList(l,"acb"));     // check if the checking actually works
    CHECK(!Node::CheckList(l,"ab"));
    CHECK(!Node::CheckList(l,"abcd"));

    l.RemHead();
    CHECK(Node::CheckList(l,"bc"));
    l.RemHead();
    CHECK(Node::CheckList(l,"c"));
    l.RemHead();
    CHECK(Node::CheckList(l,""));         // removing last element is tricky!
    l.AddHead(&d);
    l.AddHead(&a);
    CHECK(Node::CheckList(l,"ad"));

    l.Clear();                            // check the forall
    l.AddHead(&c);
    l.AddHead(&b);
    l.AddHead(&a);
    Node *p;
    const sChar *str="abc";
    sFORLIST(l,p)
        CHECK(p->c==*str++);
    CHECK(*str==0);

    End();
}

/****************************************************************************/
/***                                                                      ***/
/***   Delegates                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sInt val;
void addval1() { val+=1; }
struct setvalstruct
{
    static void addval2() { val+=2; }
    void addval3() { val+=3; }
    void addval(sInt n) { val+=n; }
    void addval(sInt n,sBool doit) { if(doit) val+=n; }
};

void Delegate()
{
    Begin("sDelegate");

    setvalstruct t;
    sDelegate<void> d1(addval1);
    sDelegate<void> d2(setvalstruct::addval2);
    sDelegate<void> d3(&t,&setvalstruct::addval3);
    sDelegate1<void,sInt> d4(&t,&setvalstruct::addval);
    sDelegate2<void,sInt,sBool> d5(&t,&setvalstruct::addval);

    val = 0; d1.Call(); CHECK(val==1);
    val = 0; d2.Call(); CHECK(val==2);
    val = 0; d3.Call(); CHECK(val==3);
    val = 0; d4.Call(4); CHECK(val==4);
    val = 0; d5.Call(5,1); CHECK(val==5);

    End();
}

void Hook()
{
    Begin("sHook");

    setvalstruct t;
    sHook h;
    h.Add(sDelegate<void>(addval1));
    h.Add(sDelegate<void>(setvalstruct::addval2));
    h.Add(sDelegate<void>(&t,&setvalstruct::addval3));

    val = 0; h.Call(); CHECK(val==6);
    h.Rem(sDelegate<void>(addval1));
    val = 0; h.Call(); CHECK(val==5);

    sHook1<sInt> h1;
    h1.Add(sDelegate1<void,sInt>(&t,&setvalstruct::addval));
    val = 0; h1.Call(11); CHECK(val==11);

    sHook2<sInt,sBool> h2;
    h2.Add(sDelegate2<void,sInt,sBool>(&t,&setvalstruct::addval));
    val = 0; h2.Call(13,1); CHECK(val==13);

    End();
}

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection                                                 ***/
/***                                                                      ***/
/****************************************************************************/

class TestObject : public sGCObject
{
public:
    sArray<sGCObject *> Childs;
    void Tag()
    {
        Childs.NeedAll();
    }
};

void GarbageCollector()
{
    TestObject *a,*b,*c;

    Begin("sGarbageCollector");

    CHECK(sGC->GetObjectCount()==0);

    a = new TestObject;
    a->Childs.Add(new TestObject);
    a->Childs.Add(new TestObject);
    sGC->AddRoot(a);
    CHECK(sGC->GetObjectCount()==3);
    sGC->CollectNow();
    CHECK(sGC->GetObjectCount()==3);
    sGC->RemRoot(a);
    sGC->CollectNow();
    CHECK(sGC->GetObjectCount()==0);


    a = new TestObject;
    b = new TestObject;
    c = new TestObject;
    a->Childs.Add(b);
    a->Childs.Add(c);
    b->Childs.Add(c);
    c->Childs.Add(b);
    sGC->AddRoot(a);
    CHECK(sGC->GetObjectCount()==3);
    sGC->CollectNow();
    CHECK(sGC->GetObjectCount()==3);
    sGC->RemRoot(a);
    sGC->CollectNow();
    CHECK(sGC->GetObjectCount()==0);

    End();
}

/****************************************************************************/
/***                                                                      ***/
/***   Runtime Type Information                                           ***/
/***                                                                      ***/
/****************************************************************************/

class A
{
public:
    sRTTIBASE(A);
};

class B : public A
{
public:
    sRTTI(B,A);
};

class C : public A
{
public:
    sRTTI(C,A);
};


void Rtti()
{
    Begin("Rtti");

    auto a = new A();
    auto b = new B();
    auto c = new C();
    A *bx = b;

    CHECK(a->GetRtti()->Atomic==1);
    CHECK(b->GetRtti()->Atomic==2);
    CHECK(c->GetRtti()->Atomic==3);

    CHECK(sCmpString(a->GetRtti()->Name,"A")==0);
    CHECK(sCmpString(b->GetRtti()->Name,"B")==0);
    CHECK(sCmpString(c->GetRtti()->Name,"C")==0);
    CHECK(a->GetRtti()->Base==0);
    CHECK(b->GetRtti()->Base==a->GetRtti());
    CHECK(c->GetRtti()->Base==a->GetRtti());

    CHECK(!sCanCast<B>(a));
    CHECK(!sCanCast<C>(a));

    CHECK(sCanCast<A>(a));
    CHECK(sCanCast<B>(b));
    CHECK(sCanCast<C>(c));

    CHECK(sCanCast<A>(b));
    CHECK(sCanCast<A>(c));

    CHECK(!sCanCast<C>(b));
    CHECK(!sCanCast<B>(c));

    CHECK(sCanCast<A>(bx));
    CHECK(sCanCast<B>(bx));

    sCast<A>(b);

    delete b;
    delete a;
    delete c;

    End();
}

/****************************************************************************/
/***                                                                      ***/
/***   Arrays                                                             ***/
/***                                                                      ***/
/****************************************************************************/

struct ArrayTestData
{
    sU32 Value;
};

static ArrayTestData ArrayTestDataPool[16];

/****************************************************************************/

void SetArray(sArray<sU32> &a,const sChar *v)
{
    a.Clear();
    for(sInt i=0;v[i];i++)
        a.Add(v[i]=='*'?0:v[i]);
}

void SetArray(sArray<const sChar *> &a,const sChar *v)
{
    a.Clear();
    for(sInt i=0;v[i];i++)
        a.Add(v[i]=='*' ? 0 : v+i);
}

void SetArray(sArray<ArrayTestData> &a,const sChar *v)
{
    a.Clear();
    for(sInt i=0;v[i];i++)
    {
        ArrayTestData d;
        d.Value = (v[i]=='*' ? 0 : v[i]);
        a.Add(d);
    }
}

void SetArray(sArray<ArrayTestData *> &a,const sChar *v)
{
    a.Clear();
    for(sInt i=0;v[i];i++)
    {
        ArrayTestDataPool[i].Value = (v[i]=='*' ? 0 : v[i]);
        a.Add(&ArrayTestDataPool[i]);
    }
}

/****************************************************************************/

sBool TestArray(const sArray<sU32> &a,const sChar *v)
{
    if(sGetStringLen(v)!=a.GetCount())
        return 0;
    for(sInt i=0;i<a.GetCount();i++)
        if(a[i]!=v[i])
            return 0;
    return 1;
}

sBool TestArray(const sArray<const sChar *> &a,const sChar *v)
{
    if(sGetStringLen(v)!=a.GetCount())
        return 0;
    for(sInt i=0;i<a.GetCount();i++)
        if(*a[i]!=v[i])
            return 0;
    return 1;
}

sBool TestArray(const sArray<ArrayTestData> &a,const sChar *v)
{
    if(sGetStringLen(v)!=a.GetCount())
        return 0;
    for(sInt i=0;i<a.GetCount();i++)
        if(a[i].Value!=v[i])
            return 0;
    return 1;
}

sBool TestArray(const sArray<ArrayTestData *> &a,const sChar *v)
{
    if(sGetStringLen(v)!=a.GetCount())
        return 0;
    for(sInt i=0;i<a.GetCount();i++)
        if(a[i]->Value!=v[i])
            return 0;
    return 1;
}

/****************************************************************************/

void ArrayBase()
{
    Begin("sArray");

    sArray<sU32> a,b;
    sU32 *p;

    // do we have an array at all?

    a.Clear();
    CHECK(TestArray(a,""));
    a.AddTail('a');
    a.AddTail('b');
    a.AddTail('c');
    CHECK(TestArray(a,"abc"));
    a.AddHead('0');
    CHECK(TestArray(a,"0abc"));
    a.RemAtOrder(2);
    CHECK(TestArray(a,"0ac"));
    a.RemOrder('a');
    CHECK(TestArray(a,"0c"));

    // add and remove

    SetArray(a,"ab");  SetArray(b,"01");  a.AddTail(b);  CHECK(TestArray(a,"ab01"));

    SetArray(a,"abcd"); a.AddAt('0',0); CHECK(TestArray(a,"0abcd"));
    SetArray(a,"abcd"); a.AddAt('0',1); CHECK(TestArray(a,"a0bcd"));
    SetArray(a,"abcd"); a.AddAt('0',4); CHECK(TestArray(a,"abcd0"));
    SetArray(a,"0abcd"); a.RemAtOrder(0); CHECK(TestArray(a,"abcd"));
    SetArray(a,"a0bcd"); a.RemAtOrder(1); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abcd0"); a.RemAtOrder(4); CHECK(TestArray(a,"abcd"));
    SetArray(a,"0abcd"); a.RemOrder('0'); CHECK(TestArray(a,"abcd"));
    SetArray(a,"a0bcd"); a.RemOrder('0'); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abcd0"); a.RemOrder('0'); CHECK(TestArray(a,"abcd"));
    SetArray(a,"0abcd"); a.RemAt(0); CHECK(TestArray(a,"dabc"));
    SetArray(a,"a0bcd"); a.RemAt(1); CHECK(TestArray(a,"adbc"));
    SetArray(a,"abcd0"); a.RemAt(4); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abcd"); a.RemTail(); CHECK(TestArray(a,"abc"));
    SetArray(a,"a"); a.RemTail(); CHECK(TestArray(a,""));

    SetArray(a,"abcd"); p=a.AddMany(2); p[0]='0'; p[1]='1'; CHECK(TestArray(a,"abcd01"));
    SetArray(a,""); p=a.AddMany(2); p[0]='0'; p[1]='1'; CHECK(TestArray(a,"01"));

    // other simple ops

    SetArray(a,"abcd"); a.Swap(1,2); CHECK(TestArray(a,"acbd"));
    SetArray(a,"abcd"); a.Swap(0,3); CHECK(TestArray(a,"dbca"));

    // find

    SetArray(a,"abcd"); CHECK(a.FindIndex([](const sU32 &i){return i=='a';})==0);
    SetArray(a,"abcd"); CHECK(a.FindIndex([](const sU32 &i){return i=='d';})==3);
    SetArray(a,"abcd"); CHECK(a.FindIndex([](const sU32 &i){return i=='e';})==-1);
    SetArray(a,"abcd"); CHECK(a.Find([](const sU32 &i){return i=='a';})=='a');
    SetArray(a,"abcd"); CHECK(a.Find([](const sU32 &i){return i=='d';})=='d');
    SetArray(a,"abcd"); CHECK(a.Find([](const sU32 &i){return i=='e';})==0);

    // sorting

    SetArray(a,"abcd"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"acbd"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"dbca"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"adbc"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abab"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"aabb"));
    SetArray(a,"abcd"); a.InsertionSort([](const sU32 &a,const sU32 &b){return a>b;}); CHECK(TestArray(a,"dcba"));

    // actually, these tests are meaningless since merge and quick sort call insertion sort for small arrays.

    SetArray(a,"abcd"); a.MergeSort([](const sU32 &a,const sU32 &b){return a<b;},b); CHECK(TestArray(a,"abcd"));
    SetArray(a,"acbd"); a.MergeSort([](const sU32 &a,const sU32 &b){return a<b;},b); CHECK(TestArray(a,"abcd"));
    SetArray(a,"dbca"); a.MergeSort([](const sU32 &a,const sU32 &b){return a<b;},b); CHECK(TestArray(a,"abcd"));
    SetArray(a,"adbc"); a.MergeSort([](const sU32 &a,const sU32 &b){return a<b;},b); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abab"); a.MergeSort([](const sU32 &a,const sU32 &b){return a<b;},b); CHECK(TestArray(a,"aabb"));
    SetArray(a,"abcd"); a.MergeSort([](const sU32 &a,const sU32 &b){return a>b;},b); CHECK(TestArray(a,"dcba"));

    SetArray(a,"abcd"); a.QuickSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"acbd"); a.QuickSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"dbca"); a.QuickSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"adbc"); a.QuickSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"abab"); a.QuickSort([](const sU32 &a,const sU32 &b){return a<b;}); CHECK(TestArray(a,"aabb"));
    SetArray(a,"abcd"); a.QuickSort([](const sU32 &a,const sU32 &b){return a>b;}); CHECK(TestArray(a,"dcba"));

    // done

    End();
}

/****************************************************************************/

void ArrayValue()
{
    Begin("sArray Value");

    sArray<sU32> a;
    sArray<ArrayTestData> m;

    SetArray(a,"abcd"); 
    CHECK(a.FindIndex([](sU32 &i){return i=='a';})==0);
    CHECK(a.FindIndex([](sU32 &i){return i=='d';})==3);
    CHECK(a.FindIndex([](sU32 &i){return i=='e';})==-1);

    CHECK(a.Find([](sU32 &i){return i=='a';})=='a');
    CHECK(a.Find([](sU32 &i){return i=='d';})=='d');
    CHECK(a.Find([](sU32 &i){return i=='e';})==0);

    for(sU32 &i : a)
    {
        sSPtr _i = sGetIndex(a,i);
        CHECK(i==a[_i]);
    }

    SetArray(a,"a*b*cd");  a.RemIfOrder([](sU32 i){return i== 0 ;}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"a*b*cd");  a.RemIf     ([](sU32 i){return i== 0 ;}); CHECK(TestArray(a,"adbc"));
    SetArray(a,"axbxcd");  a.RemIfOrder([](sU32 i){return i=='x';}); CHECK(TestArray(a,"abcd"));


    SetArray(m,"abcd"); 
    CHECK(m.FindIndex([](ArrayTestData &i){return i.Value=='a';})==0);
    CHECK(m.FindIndex([](ArrayTestData &i){return i.Value=='d';})==3);
    CHECK(m.FindIndex([](ArrayTestData &i){return i.Value=='e';})==-1);

    CHECK(m.FindValue([](ArrayTestData &i){return i.Value=='a';})->Value=='a');
    CHECK(m.FindValue([](ArrayTestData &i){return i.Value=='d';})->Value=='d');
    CHECK(m.FindValue([](ArrayTestData &i){return i.Value=='e';})==0);

    for(ArrayTestData &i : m)
    {
        sSPtr _i = sGetIndex(m,i);
        CHECK(i.Value==m[_i].Value);
    }

    SetArray(m,"a*b*cd");  m.RemIfOrder([](ArrayTestData &i){return i.Value==0;}); CHECK(TestArray(m,"abcd"));
    SetArray(m,"a*b*cd");  m.RemIf([](ArrayTestData &i){return i.Value==0;}); CHECK(TestArray(m,"adbc"));
    SetArray(m,"axbxcd");  m.RemIfOrder([](ArrayTestData &i){return i.Value=='x';}); CHECK(TestArray(m,"abcd"));

    End();
}

/****************************************************************************/

void ArrayPtr()
{
    Begin("sArray Ptr");

    sArray<const sChar *> a;
    sArray<ArrayTestData *> m;

    SetArray(a,"abcd");   
    CHECK(a.FindIndex([](const sChar *i){return *i=='a';})==0);
    CHECK(a.FindIndex([](const sChar *i){return *i=='d';})==3);
    CHECK(a.FindIndex([](const sChar *i){return *i=='e';})==-1);

    CHECK(*a.Find([](const sChar *i){return *i=='a';})=='a');
    CHECK(*a.Find([](const sChar *i){return *i=='d';})=='d');
    CHECK(a.Find([](const sChar *i){return *i=='e';})==0);


    for(const sChar *&i : a)
    {
        sSPtr _i = sGetIndex(a,i);
        CHECK(*i==*a[_i]);
    }

    SetArray(a,"a#b#cd");  a.RemIfOrder([](const sChar *i){return *i=='#';}); CHECK(TestArray(a,"abcd"));
    SetArray(a,"a#b#cd");  a.RemIf([](const sChar *i){return *i=='#';}); CHECK(TestArray(a,"adbc"));


    SetArray(m,"abcd"); 
    CHECK(m.FindIndex([](const ArrayTestData *i){return i->Value=='a';})==0);
    CHECK(m.FindIndex([](const ArrayTestData *i){return i->Value=='d';})==3);
    CHECK(m.FindIndex([](const ArrayTestData *i){return i->Value=='e';})==-1);

    CHECK(m.Find([](const ArrayTestData *i){return i->Value=='a';})->Value=='a');
    CHECK(m.Find([](const ArrayTestData *i){return i->Value=='d';})->Value=='d');
    CHECK(m.Find([](const ArrayTestData *i){return i->Value=='e';})==0);

    for(ArrayTestData *&i : m)
    {
        sSPtr _i = sGetIndex(m,i);
        CHECK(i->Value==m[_i]->Value);
    }

    SetArray(m,"a*b*cd");  m.RemIfOrder([](const ArrayTestData *i){return i->Value==0;}); CHECK(TestArray(m,"abcd"));
    SetArray(m,"a*b*cd");  m.RemIf     ([](const ArrayTestData *i){return i->Value==0;}); CHECK(TestArray(m,"adbc"));
    SetArray(m,"axbxcd");  m.RemIfOrder([](const ArrayTestData *i){return i->Value=='x';}); CHECK(TestArray(m,"abcd"));

    End();
}

/****************************************************************************/

void Solve()
{
    Begin("sSolve");

    {
        sMatrix33 m;
        sVector3 a;
        sVector3 r;

        m.i.Set(1,3,2);
        m.j.Set(3,5,4);
        m.k.Set(-2,6,3);
        a.Set(5,7,8);
        r = sSolve(m,a);

        CHECK(r.x == -15);
        CHECK(r.y == 8);
        CHECK(r.z == 2);
    }
    {
        sMatrix22 m;
        sVector2 a;
        sVector2 r;

        m.i.Set(2,4);
        m.j.Set(3,9);
        a.Set(6,15);
        r = sSolve(m,a);

        CHECK(r.x == 1.5f);
        CHECK(r.y == 1);
    }

    End();
}

/****************************************************************************/

void RCString()
{
    Begin("sRCString");

    sRCString *a;
    
    a = new sRCString("hello"); CHECK(*a=="hello"); a->Release();
    a = new sRCString("hello",4); CHECK(*a=="hell"); a->Release();
    a = new sRCString(); a->PrintF("hello"); CHECK(*a=="hello"); a->Release();

    End();
}


/****************************************************************************/
/***                                                                      ***/
/***   Huffman                                                            ***/
/***                                                                      ***/
/****************************************************************************/

void Huffman()
{
    sHuffman huf;
    for(int i=0;i<4;i++)
        huf.AddContext(i,10);

    uint Src[0x10000];
    uint Dest[0x10000];

    sRandomKISS rnd;

    Begin("Huffman");

    for(int round=0;round<32;round++)
    {
        int count = rnd.Int(0xc000)+0x4000;
        int max = rnd.Int(1023)+1;
        int distribution = rnd.Int(2);

        if(distribution)
        {
            for(int i=0;i<count;i++)
                Src[i] = (rnd.Int(max)+rnd.Int(max)+rnd.Int(max))/3;
        }
        else
        {
            for(int i=0;i<count;i++)
                Src[i] = rnd.Int(max);
        }


        huf.BeginPack();
        for(int i=0;i<count;i++)
            huf.HistPack(0,Src[i]);
        huf.PreparePack();
        for(int i=0;i<count;i++)
            huf.Pack(0,Src[i]);
        huf.EndPack();

        huf.BeginUnpack();
        for(int i=0;i<count;i++)
            Dest[i] = huf.Unpack(0);
        huf.EndUnpack();

        CHECK(sCmpMem(Src,Dest,count*sizeof(uint))==0);
    }

    End();
}


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   Run the tests                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

void Altona2::Main()
{
    Playground();

    Huffman();
    SList();
    SListRev();
    Delegate();
    Hook();
    GarbageCollector();
    Rtti();
    ArrayValue();
    ArrayPtr();
    ArrayBase();
    Solve();
    RCString();

    if(AllErrors>0)
    {
        sPrintF("%d errors in %d sections\n",AllErrors,AllErrorSections);
    }
    else
    {
        sPrintF("All OK\n");
    }
}

/****************************************************************************/
