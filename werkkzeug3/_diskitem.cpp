// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_start.hpp"
#include "_diskitem.hpp"
#if sLINK_GUI
#include "_gui.hpp"
#endif

/****************************************************************************/

class sDISystemInfo : public sDiskItem
{
  sChar Name[128];
public:
  sU32 GetClass() { return sCID_DISYSTEMINFO; }

  sBool Init(sChar *name,sDiskItem *parent,void *data);
  sBool GetAttr(sInt attr,void *d,sInt &s);
  sBool SetAttr(sInt attr,void *d,sInt s);
  sDInt Cmd(sInt cmd,sChar *s,sDiskItem *d);
  sBool Support(sInt id);
};


/****************************************************************************/
/****************************************************************************/
/***                                                                      ***/
/***   File Services                                                      ***/
/***                                                                      ***/
/****************************************************************************/
/****************************************************************************/

static sInt sDiskItemTypes[256][2] =
{
  { sDIT_NONE  ,0   },  // sDIA_NONE
  { sDIT_STRING,sDI_NAMESIZE },  // sDIA_NAME
  { sDIT_BINARY,0   },  // sDIA_DATA
  { sDIT_BOOL  ,4   },  // sDIA_ISDIR
  { sDIT_BOOL  ,4   },  // sDIA_WRITEPROT
  { sDIT_BOOL  ,4   },  // sDIA_INCOMPLETE
  { sDIT_BOOL  ,4   },  // sDIA_OUTOFDATE
};

/****************************************************************************/

sDiskItem::sDiskItem()
{
  List = new sList<sDiskItem>;
  Parent = 0;
  sSetMem4(UserData,0,sizeof(UserData)/4);
}

sDiskItem::~sDiskItem()
{
}

void sDiskItem::Tag()
{
  sBroker->Need(Parent);
  sBroker->Need(List);
}

/****************************************************************************/

sDiskItem *sDiskItem::GetParent()
{
  return Parent;
}

sDiskItem *sDiskItem::GetChild(sInt i)
{
  if(i>=0 && i<List->GetCount())
    return List->Get(i);
  else
    return 0;
}

sInt sDiskItem::GetChildCount()
{
  return List->GetCount();
}

sInt sDiskItem::GetAttrType(sInt attr)
{
  return sDiskItemTypes[attr][0];
}

sDiskItem *sDiskItem::Find(sChar *path,sInt mode)
{
  sChar name[sDI_NAMESIZE];
  sChar buffer[sDI_NAMESIZE];
  sChar *p;
  sDiskItem *di;
  sInt i;
  
  p = name;
  i = 0;
  while(*path && *path!='/')
  {
    *p++ = *path++;
    i++;
    if(i>sDI_NAMESIZE-1)
      return 0;
  }
  *p++ = 0;

  if(GetBool(sDIA_INCOMPLETE))
    Cmd(sDIC_FINDALL,0,0);

  for(i=0;;i++)
  {
    di = GetChild(i);
    if(!di)
      break;
    di->GetString(sDIA_NAME,buffer,sizeof(buffer));
    if(sCmpStringI(buffer,name)==0)
    {
      if(*path==0)
      {
        if(mode==sDIF_NEW)
          return 0;
        if(di->GetBool(sDIA_INCOMPLETE))
          di->Cmd(sDIC_FINDALL,0,0);
        return di;
      }
      else
        return di->Find(path+1,mode);
    }
  }

  if(mode==sDIF_CREATE)
  {
    if(*path==0)
    {
      if(Cmd(sDIC_CREATEFILE,name,0))
      {
        return Find(name,sDIF_EXISTING);
      }
    }
  }
  return 0;
}

void sDiskItem::GetPath(sChar *buffer,sInt size)
{
  if(Parent)
  {
    Parent->GetPath(buffer,size);
    if(*buffer)
    {
      while(*buffer)
      {
        buffer++;
        size--;
      }
      *buffer++ = '/';
      size--;
    }
    GetString(sDIA_NAME,buffer,size);
  }
  else
  {
    buffer[0] = 0;
  }
}

sU8 *sDiskItem::Load()
{
  sU8 *mem;
  sInt size;

  mem = 0;
  size = GetBinarySize(sDIA_DATA);
  if(size>0)
  {
    mem = new sU8[size];
    if(!GetBinary(sDIA_DATA,mem,size))
    {
      delete[] mem;
      mem = 0;
    }
  }

  return mem;
}

sChar *sDiskItem::LoadText()
{
  sChar *mem;
  sInt size;

  mem = 0;
  size = GetBinarySize(sDIA_DATA);
  if(size>0)
  {
    mem = new sChar[size+1];
    mem[size]=0;
    if(!GetBinary(sDIA_DATA,(sU8 *)mem,size))
    {
      delete[] mem;
      mem = 0;
    }
  }

  return mem;
}
sBool sDiskItem::Save(sPtr mem,sInt size)
{
  return SetBinary(sDIA_DATA,(sU8 *)mem,size);
}

void sDiskItem::Sort()
{
  sInt i,j,max;
  sDiskItem *idi,*jdi;
  sChar iname[sDI_NAMESIZE],jname[sDI_NAMESIZE];
  sInt idir,jdir;

  max = List->GetCount();
  for(i=0;i<max-1;i++)
  {
    for(j=i+1;j<max;j++)
    {
      idi = List->Get(i);
      idir = idi->GetBool(sDIA_ISDIR);
      idi->GetString(sDIA_NAME,iname,sDI_NAMESIZE);
      jdi = List->Get(j);
      jdir = jdi->GetBool(sDIA_ISDIR);
      jdi->GetString(sDIA_NAME,jname,sDI_NAMESIZE);

      if(jdir>idir || (jdir==idir && sCmpStringI(jname,iname)<0))
        List->Swap(i,j);
    }
  }
}

/****************************************************************************/

sBool sDiskItem::GetBool(sInt attr)
{
  sInt value;
  sInt size = 4;
  value = 0;
  if(sDiskItemTypes[attr][0]==sDIT_BOOL && Support(attr))
    GetAttr(attr,&value,size);
  return value;
}

void sDiskItem::SetBool(sInt attr,sInt value)
{
  if(sDiskItemTypes[attr][0]==sDIT_BOOL && Support(attr))
    SetAttr(attr,&value,4);
}

sInt sDiskItem::GetInt(sInt attr)
{
  sInt value;
  sInt size = 4;
  value = 0;
  if(sDiskItemTypes[attr][0]==sDIT_INT && Support(attr))
    GetAttr(attr,&value,size);
  return value;
}

void sDiskItem::SetInt(sInt attr,sInt value)
{
  if(sDiskItemTypes[attr][0]==sDIT_INT && Support(attr))
    SetAttr(attr,&value,4);
}

sF32 sDiskItem::GetFloat(sInt attr)
{
  sF32 value;
  sInt size = 4;
  value = 0;
  if(sDiskItemTypes[attr][0]==sDIT_FLOAT && Support(attr))
    GetAttr(attr,&value,size);
  return value;
}

void sDiskItem::SetFloat(sInt attr,sF32 value)
{
  if(sDiskItemTypes[attr][0]==sDIT_FLOAT && Support(attr))
    SetAttr(attr,&value,4);
}

void sDiskItem::GetString(sInt attr,sChar *buffer,sInt max)
{
  buffer[0] = 0;
  if(sDiskItemTypes[attr][0]==sDIT_STRING && Support(attr))
    GetAttr(attr,buffer,max);
}

void sDiskItem::SetString(sInt attr,sChar *buffer)
{
  if(sDiskItemTypes[attr][0]==sDIT_STRING && Support(attr))
    SetAttr(attr,buffer,sGetStringLen(buffer)+1);
}

sInt sDiskItem::GetBinarySize(sInt attr)
{
  sInt size;
  size = 0;
  if(sDiskItemTypes[attr][0]==sDIT_BINARY && Support(attr))
    GetAttr(attr,0,size);
  return size;
}

sBool sDiskItem::GetBinary(sInt attr,sU8 *buffer,sInt max)
{
  if(sDiskItemTypes[attr][0]==sDIT_BINARY && Support(attr))
    return GetAttr(attr,buffer,max);
  else
    return sFALSE;
}

sBool sDiskItem::SetBinary(sInt attr,sU8 *buffer,sInt size)
{
  if(sDiskItemTypes[attr][0]==sDIT_BINARY && Support(attr))
    return SetAttr(attr,buffer,size);
  else
    return sFALSE;
}

/****************************************************************************/

static sBool sDiskItemKeys[sDI_MAXKEY];


sU32 sDiskItem::AllocKey()
{
  sInt i;

  for(i=0;i<sDI_MAXKEY;i++)
  {
    if(sDiskItemKeys[i]==0)
    {
      sDiskItemKeys[i] = 1;
      sDiskRoot->ClearKeyR(i);
      return i;
    }
  }
#if sDEBUG
  sVERIFYFALSE;
#else
  return 0;
#endif
}

void sDiskItem::FreeKey(sU32 key)
{
  sDiskItemKeys[key] = 0;
}

void sDiskItem::ClearKeyR(sInt key)
{
  sInt i;
  sDiskItem *di;
  UserData[key] = 0;
  for(i=0;;i++)
  {
    di = GetChild(i);
    if(!di) break;
    di->ClearKeyR(key);
  }
}

void sDiskItem::SetUserData(sU32 key,sU32 value)
{
  UserData[key] = value;
}

sU32 sDiskItem::GetUserData(sU32 key)
{
  return UserData[key];
}

/****************************************************************************/
/****************************************************************************/

sDIRoot::sDIRoot()
{
  sDIFolder *df;
  sDiskItem *dd;
  sInt i;
  static sChar buf[3]=" :";
  sU32 mask;

  df = new sDIFolder;
  df->Init("Disks",this,0);
  List->Add(df);

  mask = sSystem->GetDriveMask();
  for(i=0;i<26;i++)
  {
    if(mask & (1<<i))
    {
      buf[0] = 'a' + i;
      dd = new sDIDir; if(dd->Init(buf,df,0)) df->Add(dd);
    }
  }

//  dd = new sDIDir; if(dd->Init("\\\\chaos-linux\\stuff",df,0)) df->Add(dd);

  df = new sDIFolder;
  df->Init("Programms",this,0);
  List->Add(df);

  df = new sDIFolder;
  df->Init("System",this,0);
  List->Add(df);
    dd = new sDISystemInfo; dd->Init("Info",0,0); df->Add(dd);

}

sDIRoot::~sDIRoot()
{
}

void sDIRoot::Tag()
{
  sDiskItem::Tag();
}

sBool sDIRoot::Init(sChar *name,sDiskItem *parent,void *data)
{
  return sTRUE;
}

sBool sDIRoot::GetAttr(sInt attr,void *d,sInt &s)
{
  sInt result;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)d,"root",s);
    break;
  case sDIA_ISDIR:
    *(sInt*)d = 1;
    break;
  default:
    result = sFALSE;
    break;
  }

  return result;
}

sBool sDIRoot::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sDInt sDIRoot::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  return sFALSE;
}

sBool sDIRoot::Support(sInt id)
{
  sInt result;

  result = sFALSE;
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
    result = sTRUE;
    break;
  }
  return result;
}


/****************************************************************************/

sDIFile::sDIFile()
{
  Name[0] = 0;
  WriteProtect = 0;
  Size = 0;
}

sDIFile::~sDIFile()
{
}

void sDIFile::Tag()
{
  sDiskItem::Tag();
}

void sDIFile::MakePath(sChar *dest,sChar *name)
{
  if(name==0)
    name = Name;
  sCopyString(dest,((sDIDir *)Parent)->Path,sDI_PATHSIZE);
  sAppendString(dest,"/",sDI_PATHSIZE);
  sAppendString(dest,name,sDI_PATHSIZE);
}

/****************************************************************************/

sBool sDIFile::Init(sChar *name,sDiskItem *parent,void *data)
{
  Size = ((sU32 *)(data))[0];
  WriteProtect = ((sU32 *)(data))[1];
  sCopyString(Name,name,sizeof(Name));
  Parent = parent;
  return sTRUE;
}

sBool sDIFile::GetAttr(sInt attr,void *data,sInt &s)
{
  sChar path[sDI_PATHSIZE];
  sInt result;
  sU8 *d2;
  sInt rs;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)data,Name,s);
    break;
  case sDIA_DATA:
    MakePath(path);
    if(data)
    {
      d2 = sSystem->LoadFile(path,rs);
      if(d2 && rs<=s)
      {
        sCopyMem(data,d2,rs);
        result = sTRUE;
      }
      else
      {
        result = sFALSE;
      }
      if(d2)
        delete[] d2;
    }
    else
      s = Size;
    break;
  case sDIA_ISDIR:
    *(sInt *)data = 0;
    break;
  case sDIA_WRITEPROT:
    *(sInt *)data = WriteProtect;
    break;

  default:
    result = sFALSE;
  }
  return result;
}

sBool sDIFile::SetAttr(sInt attr,void *data,sInt s)
{
  sChar path[sDI_PATHSIZE];
  sChar path2[sDI_PATHSIZE];
  sInt result;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    MakePath(path);
    MakePath(path2,(sChar *)data);
    result = sSystem->RenameFile(path,path2);
    if(result)
      sCopyString(Name,(sChar *)data,sizeof(Name));
    break;
  case sDIA_DATA:
    MakePath(path);
    result = sSystem->SaveFile(path,(sU8 *)data,s);
    if(result)
      Size = s;
    else
      ;//Size = 0;   // need better error handling here!
    break;
  default:
    result = sFALSE;
    break;
  }
  return result;
}

sDInt sDIFile::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  sChar path[sDI_PATHSIZE];
  sInt result;

  result = sTRUE;
  switch(cmd)
  {
  case sDIC_DELETE:
    MakePath(path);
    result = sSystem->DeleteFile(path);
    break;
  default:
    result = sFALSE;
    break;
  }
  return result;
}

sBool sDIFile::Support(sInt id)
{
  sInt result;

  result = sFALSE;
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_DATA:
  case sDIA_ISDIR:
  case sDIA_WRITEPROT:
  case sDIC_DELETE:
    result = sTRUE;
    break;
  }
  return result;
}

/****************************************************************************/

sDIDir::sDIDir()
{
  Path[0]=0;
  Complete = 0;
}

sDIDir::~sDIDir()
{
}

void sDIDir::Tag()
{
  sDiskItem::Tag();
}

sBool sDIDir::Init(sChar *name,sDiskItem *parent,void *data)
{
  sCopyMem(Path,name,sDI_PATHSIZE);
  Parent = parent;
  return sTRUE;
}

sBool sDIDir::GetAttr(sInt attr,void *data,sInt &size)
{
  sInt result;
  sChar *t,*r;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    r = t =  Path;
    while(*r!=0)
    {
      if(*r=='/')
        t = r+1;
      r++;
    }
    sCopyString((sChar *)data,t,size);
    break;
  case sDIA_ISDIR:
    *(sInt*)data = 1;
    break;
  case sDIA_INCOMPLETE:
    *(sInt*)data = Complete ? 0 : 1;
    break;
  default:
    result = sFALSE;
    break;
  }

  return result;
}

sBool sDIDir::SetAttr(sInt attr,void *data,sInt size)
{
  return sFALSE;
}

sDInt sDIDir::Cmd(sInt cmd,sChar *name,sDiskItem *di)
{
  sInt result;
  sChar buffer[sDI_PATHSIZE];

  switch(cmd)
  {
  case sDIC_FINDALL:
  case sDIC_FIND:
    if(!Complete)
      LoadDir();
    return sTRUE;
  case sDIC_CREATEDIR:
    sCopyString(buffer,Path,sDI_PATHSIZE);
    sAppendString(buffer,"/",sDI_PATHSIZE);
    sAppendString(buffer,name,sDI_PATHSIZE);
    result = sSystem->MakeDir(buffer);
    LoadDir();
    return result;
  case sDIC_CREATEFILE:
    sCopyString(buffer,Path,sDI_PATHSIZE);
    sAppendString(buffer,"/",sDI_PATHSIZE);
    sAppendString(buffer,name,sDI_PATHSIZE);
    result = sSystem->SaveFile(buffer,(sU8*)" ",0);
    LoadDir();
    return result;
  case sDIC_RELOAD:
    Complete = 0;
    LoadDir();
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool sDIDir::Support(sInt id)
{
  sInt result;

  result = sFALSE;
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
  case sDIA_INCOMPLETE:
  case sDIC_FIND:
  case sDIC_FINDALL:
  case sDIC_CREATEFILE:
  case sDIC_CREATEDIR:
  case sDIC_RELOAD:
    result = sTRUE;
    break;
  }
  return result;
}

void sDIDir::LoadDir()
{
  sDIDir *nd;
  sDIFile *nf;
  sDirEntry *dir,*ds;
  sChar name[sDI_PATHSIZE];
  sU32 prv[2];

  List->Clear();

  ds = dir = sSystem->LoadDir(Path);
  while(dir && dir->Name[0])
  {
    if(dir->IsDir)
    {
      sCopyString(name,Path,sDI_PATHSIZE);
      sAppendString(name,"/",sDI_PATHSIZE);
      sAppendString(name,dir->Name,sDI_PATHSIZE);
      nd = new sDIDir;
      if(nd->Init(name,this,0))
        List->Add(nd);
    }
    else
    {
      prv[0] = dir->Size;
      prv[1] = 0;
      sCopyString(name,dir->Name,sDI_NAMESIZE);

      nf = new sDIFile;
      if(nf->Init(name,this,prv))
        List->Add(nf);
    }

    dir++;
  }
  delete[] ds;
  Complete = 1;
}

/****************************************************************************/

sDIFolder::sDIFolder()
{
  Name[0] = 0;
}

sDIFolder::~sDIFolder()
{
}

void sDIFolder::Tag()
{
  sDiskItem::Tag();
}

sBool sDIFolder::Init(sChar *name,sDiskItem *parent,void *data)
{
  sCopyMem(Name,name,sizeof(Name));
  Parent = parent;
  return sTRUE;
}

sBool sDIFolder::GetAttr(sInt attr,void *data,sInt &size)
{
  sInt result;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)data,Name,size);
    break;
  case sDIA_ISDIR:
    *(sInt*)data = 1;
    break;
  default:
    result = sFALSE;
    break;
  }

  return result;
}

sBool sDIFolder::SetAttr(sInt attr,void *d,sInt s)
{
  return 0;
}

sDInt sDIFolder::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  return 0;
}

sBool sDIFolder::Support(sInt id)
{
  sInt result;

  result = sFALSE;
  switch(id)
  {
  case sDIA_NAME:
  case sDIA_ISDIR:
    result = sTRUE;
    break;
  }
  return result;
}


/****************************************************************************/

sDIApp::sDIApp()
{
}

sDIApp::~sDIApp()
{
}

void sDIApp::Tag()
{
  sDiskItem::Tag();
}

sBool sDIApp::Init(sChar *name,sDiskItem *parent,void *data)
{
  sCopyMem(Name,name,sizeof(Name));
  Parent = parent;
  Start = (void (*)()) data;
  return sTRUE;
}

sBool sDIApp::GetAttr(sInt attr,void *data,sInt &size)
{
  sInt result;

  result = sTRUE;
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)data,Name,size);
    break;
  default:
    result = sFALSE;
    break;
  }

  return result;
}

sBool sDIApp::SetAttr(sInt attr,void *data,sInt size)
{
  return sFALSE;
} 

sDInt sDIApp::Cmd(sInt cmd,sChar *name,sDiskItem *di)
{
  if(cmd==sDIC_EXECUTE)
  {
    if(Start)
      (*Start)();
    return sTRUE;
  }
  return sFALSE;
}

sBool sDIApp::Support(sInt id)
{
  sInt result;

  result = sFALSE;
  switch(id)
  {
  case sDIA_NAME:
  case sDIC_EXECUTE:
    result = sTRUE;
    break;
  }
  return result;
}

/****************************************************************************/
/***                                                                      ***/
/***   System Info (like proc-filesystem)                                 ***/
/***                                                                      ***/
/****************************************************************************/

#if sLINK_GUI

class sSystemInfoWindow : public sGuiWindow
{
public:
  sSystemInfoWindow();
  void OnCalcSize();
  void OnPaint();
};

sSystemInfoWindow::sSystemInfoWindow()
{
  AddBorder(new sFocusBorder);
}

void sSystemInfoWindow::OnCalcSize()
{
  SizeX = 200;
  SizeY = 100;
}
  
void sSystemInfoWindow::OnPaint()
{
  sInt x,y,h;
  sChar buffer[128];
  sU32 col,cpu;
  sInt font;
  sInt oc,rc;

  sPainter->Paint(sGui->FlatMat,Client,sGui->Palette[sGC_BACK]);
  x = Client.x0+2;
  y = Client.y0+2;
  font = sGui->FixedFont;
  col = sGui->Palette[sGC_TEXT];
  h = sPainter->GetHeight(font);

  sBroker->Stats(oc,rc);

  sSPrintF(buffer,sizeof(buffer),"Objects Allocated:    %d",oc);
  sPainter->Print(font,x,y,buffer,col); y+=h;
  sSPrintF(buffer,sizeof(buffer),"Objects Rooted:       %d",rc);
  sPainter->Print(font,x,y,buffer,col); y+=h;
  sSPrintF(buffer,sizeof(buffer),"Memory Used:          %d",sSystem->MemoryUsed());
  sPainter->Print(font,x,y,buffer,col); y+=h;

  sSPrintF(buffer,sizeof(buffer),"CPU Features:         ");
  cpu = sSystem->GetCpuMask();
  if(cpu&sCPU_CMOVE)  sAppendString(buffer,"CMOVE " ,sizeof(buffer));
  if(cpu&sCPU_RDTSC)  sAppendString(buffer,"RDTSC " ,sizeof(buffer));
  if(cpu&sCPU_MMX)    sAppendString(buffer,"MMX "   ,sizeof(buffer));
  if(cpu&sCPU_MMX2)   sAppendString(buffer,"MMX2 "  ,sizeof(buffer));
  if(cpu&sCPU_SSE)    sAppendString(buffer,"SSE "   ,sizeof(buffer));
  if(cpu&sCPU_SSE2)   sAppendString(buffer,"SSE2 "  ,sizeof(buffer));
  if(cpu&sCPU_3DNOW)  sAppendString(buffer,"3DNOW " ,sizeof(buffer));
  if(cpu&sCPU_3DNOW2) sAppendString(buffer,"3DNOW2 ",sizeof(buffer));
  sPainter->Print(font,x,y,buffer,col); y+=h;

  sSPrintF(buffer,sizeof(buffer),"CPU Clock:           %-8.2f",0.0f);
  col = sSystem->GetCpuMask();
  sPainter->Print(font,x,y,buffer,col); y+=h;
}

#endif

/****************************************************************************/
/****************************************************************************/

sBool sDISystemInfo::Init(sChar *name,sDiskItem *parent,void *data)
{
  sCopyString(Name,name,sizeof(Name));
  return sTRUE;
}

sBool sDISystemInfo::GetAttr(sInt attr,void *d,sInt &s)
{
  switch(attr)
  {
  case sDIA_NAME:
    sCopyString((sChar *)d,Name,s);
    return sTRUE;
  default:
    return sFALSE;
  }
}

sBool sDISystemInfo::SetAttr(sInt attr,void *d,sInt s)
{
  return sFALSE;
}

sDInt sDISystemInfo::Cmd(sInt cmd,sChar *s,sDiskItem *d)
{
  switch(cmd)
  {
  case sDIC_CUSTOMWINDOW:
#if sLINK_GUI
    return (sDInt) new sSystemInfoWindow;
#else
    return sFALSE;
#endif
  default:
    return sFALSE;
  }
}

sBool sDISystemInfo::Support(sInt id)
{
  switch(id)
  {
  case sDIA_NAME:
  case sDIC_CUSTOMWINDOW:
    return sTRUE;
  default:
    return sFALSE;
  }
}
