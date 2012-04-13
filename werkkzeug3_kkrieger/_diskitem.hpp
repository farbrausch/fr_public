// This file is distributed under a BSD license. See LICENSE.txt for details.

#ifndef __DISKITEM_HPP__
#define __DISKITEM_HPP__
#if sLINK_DISKITEM

#include "_types.hpp"
#include "_start.hpp"

/****************************************************************************/

extern class sDiskItem *sDiskRoot;

/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   File Services                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

#define sDI_MAXKEY  32

class sDiskItem : public sObject
{
protected:
  sDiskItem *Parent;                                  // backlink to parent
  sList<sDiskItem> *List;                             // list of childs
  sU32 UserData[sDI_MAXKEY];                          // array of userdata

public:
  sDiskItem();                                        // common object    
  ~sDiskItem();
  sU32 GetClass() { return sCID_DISKITEM; }
  void Tag();

  // general processing

  sDiskItem *GetParent();                             // get parent
  sDiskItem *GetChild(sInt i);                        // enumerate directory. 0 = no more items
  sInt GetChildCount();                               // if you want to know in advance...
  sInt GetAttrType(sInt attr);                        // attributes and thier types are standartised
  sDiskItem *Find(sChar *path,sInt mode=0);           // do path-parsing
  void GetPath(sChar *buffer,sInt size);              // calculate absolute path (without root)
  sU8 *Load();                                        // load file
  sChar *LoadText();                                  // load file
  sBool Save(sPtr mem,sInt data);                     // save file
  void Sort();

  // access

  sBool GetBool(sInt attr);                           // get boolean. uses 32 bit int.
  void SetBool(sInt attr,sInt value);                 // set boolean. must be 0 or 1.
  sInt GetInt(sInt attr);                             // get int
  void SetInt(sInt attr,sInt value);                  // set int
  sF32 GetFloat(sInt attr);                           // get 32 bit float
  void SetFloat(sInt attr,sF32 value);                // set 32 bit float
  void GetString(sInt attr,sChar *buffer,sInt max);   // reads string into buffer. if buffer is too small, truncates and return sFALSE
  void SetString(sInt attr,sChar *buffer);            // set string. if the string is too long, it is silently truncated
  sInt GetBinarySize(sInt attr);                      // get size of blob. may be 0
  sBool GetBinary(sInt attr,sU8 *buffer,sInt max);    // read data into buffer. if buffer is too small, truncates and return sFALSE
  sBool SetBinary(sInt attr,sU8 *buffer,sInt size);   // write data. if data is too large, return sFALSE and nothing is written.

  // user data

  sU32 AllocKey();                                    // get a key valid for ALL sDiskItems. 
  void FreeKey(sU32 key);                             // key is not needed any more
  void ClearKeyR(sInt key);                           // clear key recursive
  void SetUserData(sU32 key,sU32 value);              // set userdata for THIS sDiskItem
  sU32 GetUserData(sU32 key);                         // retreaves userdata for THIS sDiskItem. return random if not set

  // overload these to implement a directory

  virtual sBool Init(sChar *,sDiskItem *,void *)=0;   // initialize.
  virtual sBool GetAttr(sInt attr,void *d,sInt &s)=0; // get attribute. return FALSE if unsupported or size is wrong.
  virtual sBool SetAttr(sInt attr,void *d,sInt s)=0;  // set attribute. return FALSE if unsupported or size is wrong.
  virtual sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d)=0; // perform action on filesystem
  virtual sBool Support(sInt id)=0;                   // check if attribute/action is supported.
};

#define sDIT_NONE             0   // unused type
#define sDIT_BOOL             1   // 0 or 1
#define sDIT_INT              2   // 32 bit signed integer
#define sDIT_FLOAT            3   // 32 bit float
#define sDIT_STRING           4   // null terminated string, maybe limited in size
#define sDIT_BINARY           5   // binary data of any size

#define sDIA_NONE         0x0000  // NONE     unused attribute      
#define sDIA_NAME         0x0001  // STRING   name[128]
#define sDIA_DATA         0x0002  // BINARY   the content of the file
#define sDIA_ISDIR        0x0003  // BOOL     can be used as directory
#define sDIA_WRITEPROT    0x0004  // BOOL     write protection
#define sDIA_INCOMPLETE   0x0005  // BOOL     directory not loaded completly
#define sDIA_OUTOFDATE    0x0006  // BOOL     there may be inconsistencies...
 
#define sDIC_NONE         0x8000  //          nop
#define sDIC_CREATEDIR    0x8001  // STRING   create new directory
#define sDIC_CREATEFILE   0x8002  // STRING   create new (empty) file
#define sDIC_DELETE       0x8003  //          delete from disk
#define sDIC_MOVE         0x8004  // DISKITEM move to directory
#define sDIC_FIND         0x8005  // STRING   in a directory, create sDiskItem for one file. no path allowed here!
#define sDIC_FINDALL      0x8006  //          in a directory, create sDiskItem for all files. can also be used to update
#define sDIC_EXECUTE      0x8007  //          execute file. whatever that means...
#define sDIC_CUSTOMWINDOW 0x8008  //          create customwindow and return ptr to it as result
#define sDIC_RELOAD       0x8009  //          reload (for directories)

#define sDIF_EXISTING         0   // Find() only existing
#define sDIF_CREATE           1   // Find() will create if needed
#define sDIF_NEW              2   // Find() will create, error if already existing

#define sDI_NAMESIZE      128     // size of a string for names
#define sDI_PATHSIZE      2048    // siez of a string for paths

/****************************************************************************/

class sDIRoot : public sDiskItem
{
public:
  sDIRoot();
  ~sDIRoot();
  sU32 GetClass() { return sCID_DIROOT; }
  void Tag();

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};

/****************************************************************************/

class sDIFile : public sDiskItem
{
  sChar Name[128];
  sInt WriteProtect;
  sInt Size;

  void MakePath(sChar *dest,sChar *name=0);
public:
  sDIFile();
  ~sDIFile();
  sU32 GetClass() { return sCID_DIFILE; }
  void Tag();

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};

/****************************************************************************/

class sDIDir : public sDiskItem
{
  friend class sDIFile;
  sChar Path[2048];
  sInt Complete;
  void LoadDir();
public:
  sDIDir();
  ~sDIDir();
  sU32 GetClass() { return sCID_DIDIR; }
  void Tag();

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};

/****************************************************************************/

class sDIFolder : public sDiskItem
{
  sChar Name[128];
public:
  sDIFolder();
  ~sDIFolder();
  sU32 GetClass() { return sCID_DIFOLDER; }
  void Tag();

  void Add(sDiskItem *di)  { List->Add(di); }

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};

/****************************************************************************/

class sDIApp : public sDiskItem
{
  sChar Name[128];
  void (*Start)();
public:
  sDIApp();
  ~sDIApp();
  sU32 GetClass() { return sCID_DIAPP; }
  void Tag();

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};

/****************************************************************************/
/****************************************************************************/

#endif
#endif
