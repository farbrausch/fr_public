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

#include "base/types2.hpp"
#include "base/system.hpp"
#include "base/serialize.hpp"

/****************************************************************************/
/***                                                                      ***/
/***   Garbage Collection Memory Management                               ***/
/***                                                                      ***/
/****************************************************************************/

static sInt GCFlag=0;
static sInt GCCollecting=0;
static sArray<sObject *> *GCObjects;
static sArray<sObject *> *GCRoots;
static sThreadLock *GCLock=0;       // internal lock for safe mulithreaded access to gc data
static sThreadLock *GCUserLock=0;   // external lock for exclusive gc collection or application access

void sGCLock()
{
  GCUserLock->Lock();
}

void sGCUnlock()
{
  GCUserLock->Unlock();
}

void sAddRoot(sObject *o)
{
  if(o)
  {
    GCLock->Lock();
    GCRoots->AddTail(o);
    GCLock->Unlock();
  }
}

void sRemRoot(sObject *o)
{
  sObject *f;
  if(o)
  {
    GCLock->Lock();
    sFORALL(*GCRoots,f)        // be careful, sArray::Rem(object) will remove ALL occurences!
    {
      if(f==o)
      {
        GCRoots->RemAt(_i);
        break;
      }
    }
    sCollect();
    GCLock->Unlock();
  }
}

sInt sGetRootCount()
{
  return GCRoots->GetCount();
}

void sCollect()
{
  GCFlag = 1;
}

// exit==sTRUE: application exit, forces collection of all objects
// iterates collection until all objects are destroyed or gives warning if not all roots are removed
// without this flag in the case of sRemRoot in collected objects or in root object dtors
// objects are not collected before shutdown
void sCollector(sBool exit=sFALSE)   
{
  if(!exit)
  {
    if(!GCUserLock->TryLock())
      return; // locked by other thread, so skip collection
  }
  else
  {
    GCUserLock->Lock();
    GCFlag = 1;
  }

  sInt lastcount = 0;
  do
  {
    lastcount = GCRoots->GetCount();
    GCLock->Lock();
    sObject *o;
    if(GCFlag && GCObjects && GCRoots)
    {
      GCCollecting = 1;

      sFORALL(*GCObjects,o)
        o->NeedFlag = 0;
      sFORALL(*GCRoots,o)
        o->Need();
      sFORALL(*GCObjects,o)
        if(!o->NeedFlag)
          o->Finalize();
      for(sInt i=0;i<GCObjects->GetCount();)
      {
        o = (*GCObjects)[i];
        if(!o->NeedFlag)
        {
          GCObjects->RemAt(i);
          delete o;
        }
        else
        {
          i++;
        }
      }
      GCCollecting = 0;
    }
  } while(exit && GCRoots->GetCount()<lastcount);
  GCFlag = 0;

  GCLock->Unlock();
  sGCUnlock();
}

void sInitGC()
{
  GCLock = new sThreadLock;
  GCUserLock= new sThreadLock;
  GCObjects = new sArray<sObject *>;
  GCRoots = new sArray<sObject *>;
  GCRoots->HintSize(1024);
}

void sExitGC()
{
  sCollect();
  sCollector(sTRUE);

  if(GCRoots->GetCount())
    sLogF(L"mem",L"GC warning: memory leak, not all root objects freed\n");

  sDelete(GCLock);
  sDelete(GCUserLock);
  sDelete(GCObjects);
  sDelete(GCRoots);
}

sADDSUBSYSTEM(GarbageCollector,0x10,sInitGC,sExitGC);

/****************************************************************************/

sObject::sObject()
{
  sObject *o = this;
  GCLock->Lock();
  GCObjects->AddTail(o);
  sCollect();
  GCLock->Unlock();
}

sObject::~sObject()
{
  if(!GCCollecting)
  {
    GCLock->Lock();
    GCRoots->Rem(this);
    GCObjects->Rem(this);
    GCLock->Unlock();
  }
}

void sObject::Need() 
{ 
  if(this && !NeedFlag) 
  { 
    NeedFlag=1; 
    Tag(); 
  }
}

void sObject::Finalize()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   A message system for sObject                                       ***/
/***                                                                      ***/
/****************************************************************************/

//static sMessage sMessagePool[0x4000];
static sLockQueue<sMessage,0x4000> *sMessageQueue;
//static sInt sMessageUsed=0;
//static sThreadLock *sMessageLock;

void sInitMessage()
{
  sMessageQueue = new sLockQueue<sMessage,0x4000>;
//  sMessageLock = new sThreadLock;
}

void sExitMessage()
{
  delete sMessageQueue;
//  delete sMessageLock;
}

sADDSUBSYSTEM(Message,0x11,sInitMessage,sExitMessage);

void sMessage::Send() const
{
  if(this)
  {
    switch(Type)
    {
    case 0x01: 
      if(Target)
        (Target->*Func1)(); 
      break;
    case 0x02: 
      if(Target) 
        (Target->*Func2)(Code); 
      break;
    case 0x03: 
      (*Func3)(Target);
      break;
    case 0x04: 
      (*Func4)(Target,Code); 
      break;
    case 0x11: 
    case 0x12:
      sFatal(L"can't send a drag-message (type 0x11 or 0x12)");
      break;
    }
  }
}

void sMessage::Post() const
{
  if(this && Target)
  {
    sMessageQueue->AddTail(*this);
//    sVERIFY(sMessageUsed < sCOUNTOF(sMessagePool));
//    sMessagePool[sMessageUsed++] = *this;
  }
}

void sMessage::PostASync() const
{
//  sMessageLock->Lock();
  Post();
//  sMessageLock->Unlock();
  sTriggerEvent();
}

void sMessage::Pump()
{
  sMessage msg;
  while(sMessageQueue->RemHead(msg))
    msg.Send();
    /*
  sMessageLock->Lock();
  for(sInt i=0;i<sMessageUsed;i++)
    sMessagePool[i].Send();
  sMessageUsed = 0;
  sMessageLock->Unlock();
  */
}

void sMessage::Drag(const struct sWindowDrag &dd) const
{
  switch(Type)
  {
  case 0x11:
    (Target->*Func11)(dd);
    break;
  case 0x12:
    (Target->*Func12)(dd,Code);
    break;
  default:
    sVERIFYFALSE;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Repeatedly send a specific message.                                ***/
/***                                                                      ***/
/****************************************************************************/

void sMessageTimerThread(sThread *t,void *v)
{
  sMessageTimer *timer = (sMessageTimer *) v;
  do
  {
    sSleep(timer->Delay);
    if(timer->Msg.Target)
      timer->Msg.PostASync();
  }
  while(t->CheckTerminate() && timer->Loop);
}

sMessageTimer::sMessageTimer(const sMessage &msg,sInt delay,sInt loop)
{
  Msg = msg;
  Delay = delay;
  Loop = loop;
  Thread = new sThread(sMessageTimerThread,0,0x4000,this,0);
}

sMessageTimer::~sMessageTimer()
{
  Msg = sMessage();
  delete Thread;
}

/****************************************************************************/
/***                                                                      ***/
/***   Text Buffer Class (dynamic string of some kind)                    ***/
/***                                                                      ***/
/****************************************************************************/

sTextBuffer::sTextBuffer()
{
  Init();
}

sTextBuffer::sTextBuffer(const sTextBuffer& tb)
{
  Init();
  Print(tb.Get());
}

sTextBuffer::sTextBuffer(const sChar* t)
{
  Init();
  Print(t);
}

sTextBuffer::~sTextBuffer()
{
  delete[] Buffer;
}

sTextBuffer& sTextBuffer::operator=(const sTextBuffer& tb)
{
  if(this != &tb)
  {
    Clear();
    Print(tb.Get());
  }
  return *this;
}

sTextBuffer& sTextBuffer::operator=(const sChar* t)
{
  if (t < Buffer || t >= Buffer+Alloc)
  {
    Clear();
    Print(t);
  }
  return *this;
}


template <class streamer> void sTextBuffer::Serialize_(streamer &s)
{
  sVERIFY(sizeof(sChar)==sizeof(sU16));
  sInt len = Used;
  s | len;
  if(s.IsReading())
  {
    SetSize(len+1);
    Used = len;
  }
  s.ArrayU16((sU16*)Buffer, Used);
  s.Align();
  Buffer[Used] = 0;
}

void sTextBuffer::Serialize(sReader &stream)
{
  Serialize_(stream);
}

void sTextBuffer::Serialize(sWriter &stream)
{
  Serialize_(stream);
}

void sTextBuffer::Load(sChar *filename)
{
  const sChar *text = sLoadText(filename);
  if(text)
  {
    *this = text;
    delete[] text;
  }
}


void sTextBuffer::Init()
{
  Alloc = 1024;
  Used = 0;
  Buffer = new sChar[Alloc];
}

void sTextBuffer::Grow(sInt add)
{
  if(Used+add+1 >= Alloc)
  {
    SetSize(sMax(Alloc+add+1,Alloc*2));
  }
}

void sTextBuffer::Clear()
{
  Used = 0;
}

void sTextBuffer::SetSize(sInt count)
{
  if(Alloc<count)
  {
    sChar *n = new sChar[count];
    sCopyMem(n,Buffer,Used*sizeof(sChar));
    delete[] Buffer;

    Buffer = n;
    Alloc = count;
  }
}

const sChar *sTextBuffer::Get() const
{
  sVERIFY(Used<Alloc);
  const_cast<sChar*>(Buffer)[Used] = 0;
  return Buffer;
}

sChar *sTextBuffer::Get()
{
  static sChar empty[] = L"";
  if(Used==0 && Buffer==0) return empty;
  sVERIFY(Used<Alloc);
  const_cast<sChar*>(Buffer)[Used] = 0;
  return Buffer;
}

void sTextBuffer::Insert(sInt pos,sChar c)
{
  sVERIFY(pos>=0 && pos<=Used);
  Grow(1);

  Used++;
  for(sInt i=Used;i>pos;i--)
    Buffer[i] = Buffer[i-1];
  Buffer[pos] = c;
}

void sTextBuffer::Insert(sInt pos,const sChar *c,sInt len)
{
  sVERIFY(pos>=0 && pos<=Used);
  if(len==-1) len = sGetStringLen(c);

  Grow(len);

  Used+=len;
  for(sInt i=Used;i>pos;i--)
    Buffer[i] = Buffer[i-len];
  for(sInt i=0;i<len;i++)
    Buffer[pos+i] = c[i];
}


void sTextBuffer::Delete(sInt pos)
{
  sVERIFY(pos>=0 && pos<Used);
  Used--;
  for(sInt i=pos;i<Used;i++)
    Buffer[i] = Buffer[i+1];
}

void sTextBuffer::Delete(sInt pos,sInt count)
{
  sVERIFY(pos>=0 && pos<Used);
  Used-=count;
  for(sInt i=pos;i<Used;i++)
    Buffer[i] = Buffer[i+count];
}

void sTextBuffer::Set(sInt pos,sChar c)
{
  sVERIFY(pos>=0 && pos<Used);
  Buffer[pos] = c;
}

/****************************************************************************/

void sTextBuffer::Indent(sInt count)
{
  sInt pos = Used;
  while(pos>=0 && Buffer[pos]!='\n')
    pos--;
  pos++;
  sInt spaces = Used-pos;
  if(spaces<count)
    PrintF(L"%_",count-spaces);
}

void sTextBuffer::PrintChar(sInt c)
{
  Grow(1);
  Buffer[Used++] = c;
}

void sTextBuffer::Print(const sChar *text)
{
  Print(text,sGetStringLen(text));
}

void sTextBuffer::Print(const sChar *text,sInt c)
{
  Grow(c);
  sCopyMem(Buffer+Used,text,c*sizeof(sChar));
  Used+=c;
}

void sTextBuffer::PrintListing(const sChar *text,sInt line)
{
  while(*text)
  {
    PrintF(L"%04d ",line);
    sInt n = 0;
    while(text[n] && text[n]!='\n')
      n++;
    Print(text,n);
    Print(L"\n");
    line++;
    text+=n;
    if(*text=='\n')
      text++;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   TextFileWriter                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sTextFileWriter::sTextFileWriter()
{
  Target = 0;
}

sTextFileWriter::sTextFileWriter(sFile *target)
{
  Target = 0;
  Begin(target);
}

sTextFileWriter::sTextFileWriter(const sChar *filename)
{
  Target = 0;
  Begin(filename);
}

sTextFileWriter::~sTextFileWriter()
{
  End();
}

void sTextFileWriter::Begin(sFile *target)
{
  End();
  sDelete(Target);
  Target = target;
}

void sTextFileWriter::Begin(const sChar *filename)
{
  Begin(sCreateFile(filename,sFA_WRITE));
}

void sTextFileWriter::Flush()
{
  const sInt BUFFER_SIZE = 2048;
  sU8 buffer[BUFFER_SIZE];
  sBool lastWasCR = sFALSE;

  // convert in small chunks
  const sChar *data = Buffer.Get();
  while(*data)
  {
    sInt pos = 0;
    while(*data && pos < BUFFER_SIZE - 2)
    {
      if(sCONFIG_SYSTEM_WINDOWS)
      {
        if(*data == '\n' && !lastWasCR) // convert LF to CRLF on windows
          buffer[pos++] = '\r';

        lastWasCR = *data == '\r';
      }
      buffer[pos++] = *data++ & 0xff;
    }

    if(Target && !Target->Write(buffer,pos))
      sDelete(Target);
  }

  Buffer.Clear();
}

void sTextFileWriter::End()
{
  Flush();
  sDelete(Target);
}

/****************************************************************************/
/***                                                                      ***/
/***   StringPool                                                         ***/
/***                                                                      ***/
/****************************************************************************/

class sStringPool_
{  
  enum enums
  {
    HashSize = 4096,        // must be power of 2!
  };

  struct sStringPoolEntry
  {
    const sChar *Data;
    sStringPoolEntry *Next;
  };

  sMemoryPool *Strings;
  sMemoryPool *Entries;

  sStringPoolEntry *HashTable[HashSize];

public:
  sStringPool_()
  {
    Strings = new sMemoryPool();
    Entries = new sMemoryPool();
    sClear(HashTable);
  }

  ~sStringPool_()
  {
    delete Strings;
    delete Entries;
  }

  const sChar *Add(const sChar *s,sInt len, sBool *isnew)
  {
    sStringPoolEntry *e;
    sStringPoolEntry **hp;

    sU32 hash = sHashString(s,len);

    hp = &HashTable[hash & (HashSize-1)]; 
    e = *hp;
    while(e)
    {
      if(sCmpMem(e->Data,s,len*sizeof(sChar))==0 && e->Data[len]==0)
      {
        if (isnew) *isnew=sFALSE;
        return e->Data;
      }
      e = e->Next;
    }

    e = Entries->Alloc<sStringPoolEntry>();
    sChar *data = Strings->Alloc<sChar>(len+1);
    e->Data = data;
    sCopyMem(data,s,sizeof(sChar)*len);
    data[len] = 0;
    e->Next = *hp;
    *hp = e;

    if (isnew) *isnew=sTRUE;
    return e->Data;
  }
};

static sStringPool_ *sStringPool;
const sChar *sPoolStringEmpty;

const sChar *sAddToStringPool(const sChar *s,sInt len, sBool *isnew)
{
  return sStringPool->Add(s,len,isnew);
}

const sChar *sAddToStringPool2(const sChar *a,sInt al,const sChar *b,sInt bl)
{
  sChar *s = sALLOCSTACK(sChar,al+bl);
  for(sInt i=0;i<al;i++)
    s[i] = a[i];
  for(sInt i=0;i<bl;i++)
    s[al+i] = b[i];
  sPoolString p(s,al+bl);
  return p;
}

void sInitStringPool()
{
  if(!sStringPool)
  {
    sStringPool = new sStringPool_;
    sPoolStringEmpty = sAddToStringPool(L"",0);
  }
}

void sClearStringPool()
{
  sDelete(sStringPool);
}

sADDSUBSYSTEM(StringPool,0x11,sInitStringPool,sClearStringPool)

/****************************************************************************/
/***                                                                      ***/
/***   Hashtable                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sHashTableBase::sHashTableBase(sInt size,sInt nodesperblock)
{
  HashSize = size;
  HashMask = size-1;
  HashTable = new Node*[HashSize];
  for(sInt i=0;i<HashSize;i++)
    HashTable[i] = 0;

  NodesPerBlock = nodesperblock;
  CurrentNodeBlock = 0;
  CurrentNode = NodesPerBlock;
  FreeNodes = 0;
}

sHashTableBase::~sHashTableBase()
{
  Clear();
  delete HashTable;
}

void sHashTableBase::Clear()
{
  for(sInt i=0;i<HashSize;i++)
    HashTable[i] = 0;
  sDeleteAll(NodeBlocks);
  FreeNodes = 0;
  CurrentNode = NodesPerBlock;
  CurrentNodeBlock = 0;
}

sHashTableBase::Node *sHashTableBase::AllocNode()
{
  // (a): use free list

  if(FreeNodes)
  {
    Node *n = FreeNodes;
    FreeNodes = n->Next;
    return n;
  }

  // (b): check blocks

  if(CurrentNode==NodesPerBlock)
  {
    CurrentNodeBlock = new Node[NodesPerBlock];
    NodeBlocks.AddTail(CurrentNodeBlock);
    CurrentNode = 0;
  }

  // (c): get next node from block

  return &CurrentNodeBlock[CurrentNode++];
}

void sHashTableBase::Add(const void *key,void *value)
{
  Node *n = AllocNode();
  n->Value = value;
  n->Key = key;
  sU32 hash = HashKey(key) & HashMask;
  n->Next = HashTable[hash];
  HashTable[hash] = n;
}

void *sHashTableBase::Find(const void *key)
{
  sU32 hash = HashKey(key) & HashMask;
  Node *n = HashTable[hash];
  while(n)
  {
    if(CompareKey(n->Key,key))
      return n->Value;
    n = n->Next;
  }
  return 0;
}

void *sHashTableBase::Rem(const void *key)
{
  sU32 hash = HashKey(key) & HashMask;
  Node *n = HashTable[hash];
  Node **l = &HashTable[hash];
  while(n)
  {
    if(CompareKey(n->Key,key))
    {
      *l = n->Next;         // remove from hashtable
      n->Next = FreeNodes;  // insert in free nodes list
      FreeNodes = n;
      return n->Value;      // return value
    }
    l = &n->Next;           // maintain last ptr
    n = n->Next;            // next element
  }
  return 0;
}

void sHashTableBase::ClearAndDeleteValues()
{
  Node *n;
  for(sInt i=0;i<HashSize;i++)
  {
    n = HashTable[i];
    while(n)
    {
      delete (sU8 *)n->Value;
      n = n->Next;
    }
  }
  Clear();
}

void sHashTableBase::ClearAndDeleteKeys()
{
  Node *n;
  for(sInt i=0;i<HashSize;i++)
  {
    n = HashTable[i];
    while(n)
    {
      delete (sU8 *)n->Key;
      n = n->Next;
    }
  }
  Clear();
}

void sHashTableBase::ClearAndDelete()
{
  Node *n;
  for(sInt i=0;i<HashSize;i++)
  {
    n = HashTable[i];
    while(n)
    {
      delete (sU8 *)n->Value;
      delete (sU8 *)n->Key;
      n = n->Next;
    }
  }
  Clear();
}

void sHashTableBase::GetAll(sArray<void *> *a)
{
  for(sInt i=0;i<HashSize;i++)
  {
    Node *n = HashTable[i];
    while(n)
    {
      a->AddTail(n->Value);
      n = n->Next;
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Rectangular Regions                                                ***/
/***                                                                      ***/
/****************************************************************************/

void sRectRegion::Clear()
{
  Rects.Clear();
}

void sRectRegion::Add(const sRect &add)
{
  Sub(add);
  Rects.AddTail(add);
}

void sRectRegion::Sub(const sRect &sub)
{
  sRect r;
  sInt i;

  for(i=0;i<Rects.GetCount();)
  {
    r = Rects[i];

    if(sub.IsInside(r))
    {
      Rects.RemAt(i);
      AddParts(r,sub);
    }
    else
    {
      i++;
    }
  }
}

void sRectRegion::And(const sRect &clip)
{
  sRect r;

  sInt max = Rects.GetCount();
  sInt write = 0;

  for(sInt i=0;i<max;i++)
  {
    r = Rects[i];

    if(clip.IsInside(r))
    {
      r.And(clip);
      Rects[write++] = r;
    }
  }
  Rects.Resize(write);
}

void sRectRegion::AddParts(sRect old,const sRect &sub)
{
  sRect r;
  if(old.y1>sub.y0 && old.y0<sub.y0)      // something above?
  {
    r = old;
    r.y1 = sub.y0;
    Rects.AddTail(r);
    old.y0 = sub.y0;
  }
  if(old.y0<sub.y1 && old.y1>sub.y1)      // something below?
  {
    r = old;
    r.y0 = sub.y1;
    Rects.AddTail(r);
    old.y1 = sub.y1;
  }

  if(old.y0>=old.y1) return;              // is there something left?

  if(old.x1>sub.x0 && old.x0<sub.x0)      // something left?
  {
    r = old;
    r.x1 = sub.x0;
    Rects.AddTail(r);
    old.x0 = sub.x0;
  }
  if(old.x0<sub.x1 && old.x1>sub.x1)      // something right?
  {
    r = old;
    r.x0 = sub.x1;
    Rects.AddTail(r);
    old.x1 = sub.x1;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   String Map                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sStringMap_::sStringMap_ (sInt numSlots)
{
  Size = numSlots;
  Slots = new Slot*[Size];
  sSetMem(Slots, 0, Size * sizeof(Slot*));
}

/****************************************************************************/

sStringMap_::~sStringMap_ ()
{
  Clear();
  sDeleteArray(Slots);
}

/****************************************************************************/

void sStringMap_::Clear ()
{
  for (sInt i = 0; i < Size; i++)
  {
    Slot * slot;
    while ((slot = Slots[i])!=0)
    {
      Del(slot->key);
    }
  }
}

/****************************************************************************/

sU32 sStringMap_::Hash (const sChar * key) const
{
  sU32 hash = 0;

  for (sInt i = 0; key[i]; i++)
  {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash % Size;
}

/****************************************************************************/

void sStringMap_::Del (const sChar * key)
{
  sInt slotNum = Hash(key);
  Slot * slot = Slots[slotNum];
  Slot * last = sNULL;

  if (slot)
  {
    while (1)
    {
      if (sCmpString(slot->key, key) == 0)
      {
        if (slot->next)
        {
          if (last) last->next = slot->next;
          else Slots[slotNum] = slot->next;
        }
        else
        {
          if (last) last->next = sNULL;
          else Slots[slotNum] = slot->next;
        }

        delete[] slot->key;
        sDelete(slot);
        return;
      }
      if (!slot->next) return; // key not found
      last = slot;
      slot = slot->next;
    }
  }
}

/****************************************************************************/

void * sStringMap_::Get (const sChar * key) const
{
  sInt slotNum = Hash(key);
  Slot * slot = Slots[slotNum];
  if (slot)
  {
    while (1)
    {
      if (sCmpString(slot->key, key) == 0)
        return slot->value;
      if (!slot->next) break; // key not found
      slot = slot->next;
    }
  }

  return sNULL;
}

/****************************************************************************/

void sStringMap_::Set (const sChar * key, void * value)
{
  sInt slotNum = Hash(key);

  Slot * slot = Slots[slotNum];
  if (slot)
  {
    while (1)
    {
      if (sCmpString(slot->key, key) == 0)
      { 
        sDelete (slot->key);
        break; // key already inserted
      }
      else if (!slot->next)
      {
        slot->next = new Slot; 
        slot = slot->next; 
        slot->next = sNULL;
        break;
      }

      slot = slot->next;
    }
  }
  else
  {
    Slots[slotNum] = slot = new Slot;
    slot->next = sNULL;
  }

  slot->value = value;  
  sInt keyLen = sGetStringLen(key)+1;
  slot->key = new sChar[keyLen];
  sCopyString(slot->key, key, keyLen);
}

/****************************************************************************/

sInt sStringMap_::GetCount () const
{
  sInt count = 0;
  for (sInt slotNum = 0; slotNum < Size; slotNum++)
  {
    if (Slots[slotNum])
    {
      count++;
      Slot * slot = Slots[slotNum];
      while ((slot = slot->next)!=0) count++;
    }
  }
  return count;
}

/****************************************************************************/

sStaticArray<sChar *> * sStringMap_::GetKeys () const
{
  sStaticArray<sChar *> * keys = new sStaticArray<sChar *>(GetCount());
  for (sInt slotNum = 0; slotNum < Size; slotNum++)
  {
    if (Slots[slotNum])
    {
      Slot * slot = Slots[slotNum];
      keys->AddTail(slot->key);
      while ((slot = slot->next)!=0) keys->AddTail(slot->key);
    }
  }
  return keys;
}

/****************************************************************************/

void sStringMap_::Dump () const
{
  for (sInt i = 0; i < Size; i++)
  {
    Slot * slot = Slots[i];
    sPrintF(L"%02d: ", i);
    if (slot)
    {
      while (1)
      {
        sPrintF(L"%s->%x ", slot->key, (sDInt)slot->value);

        if (!slot->next) 
        {
          sPrintF(L"\n");
          break;
        }

        slot = slot->next;
      }
    }
    else
    {
      sPrintF(L"\n");
    }
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   BitVector                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#define BITSHIFT 5
#define BITMASK 31
#define WORDSPERCHUNK 4
#define BYTESPERWORD (1<<(BITSHIFT-3))
#define BITSPERCHUNK ((1<<BITSHIFT)*WORDSPERCHUNK)

void sBitVector::Resize(sInt bits)
{
  sPtr words = ((bits+BITSPERCHUNK-1)/BITSPERCHUNK)*4;
  if(Words<words)
  {
    sU32 *nd = new sU32[words];
    sCopyMem(nd,Data,Words*BYTESPERWORD);
    sSetMem(nd+Words,NewVal,(words-Words)*BYTESPERWORD);
    delete[] Data;
    Data = nd;
    Words = words;
  }
}

void sBitVector::ClearAll()
{
  NewVal = 0;
  sSetMem(Data,NewVal,Words*BYTESPERWORD);
}

void sBitVector::SetAll()
{
  NewVal = 0; NewVal--;
  sSetMem(Data,NewVal,Words*BYTESPERWORD);
}

sBitVector::sBitVector()
{
  Words = WORDSPERCHUNK;
  NewVal = 0;
  Data = new sU32[Words];
  ClearAll();
}

sBitVector::~sBitVector()
{
  delete[] Data;
}

sBool sBitVector::Get(sInt n)
{
  sPtr b = n>>BITSHIFT;
  if(b<Words)
    return (Data[n>>BITSHIFT]>>(n&BITMASK))&1;
  else
    return NewVal & 1;
}

void sBitVector::Set(sInt n)
{
  Resize(n+1);
  Data[n>>BITSHIFT] |= (1<<(n&BITMASK));
}

void sBitVector::Clear(sInt n)
{
  Resize(n+1);
  Data[n>>BITSHIFT] &= ~(1<<(n&BITMASK));
}

void sBitVector::Assign(sInt n,sInt v)
{
  Resize(n+1);
  sInt mask = 1<<(n&BITMASK);
  sInt byte = n>>BITSHIFT;

  Data[byte] &= ~mask;
  Data[byte] |= ((v&1)<<(n&BITMASK));
}

sBool sBitVector::NextBit(sInt &n)
{
  sVERIFY(NewVal==0);

  n++;
  sPtr word = n>>BITSHIFT;
  sU32 bit = n&BITMASK;

  // check remaining bits of this byte

  if(bit>0 && Data[word])
  {
    while(bit<(1<<BITSHIFT))
    {
      if(Data[word] & (1<<bit))
      {
        n = (word<<BITSHIFT)+bit;
        return 1;
      }
      bit++;
    }
    bit = 0;
    word++;
  }

  // skip words..

  while(Data[word]==0 && word<Words)
    word++;
  if(word>=Words)
    return 0;

  // find bit

  sVERIFY(bit==0);
  sVERIFY(Data[word]!=0);

  while(!(Data[word] & (1<<bit)))
    bit++;
  n = (word<<BITSHIFT)+bit;
  return 1;
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

