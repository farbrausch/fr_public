// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_lekktor.hpp"
#if sUSE_LEKKTOR
#include <windows.h>

/****************************************************************************/

void sLekktor::Init(sChar *name)
{
  Name=name;
  sSetMem(Modify,0,sizeof(Modify));
}

void sLekktor::Exit()
{
  sInt result;
  HANDLE handle; 
  DWORD test;

  result = sFALSE;
  handle = CreateFile(Name,GENERIC_WRITE,FILE_SHARE_WRITE,0,CREATE_NEW,0,0);  
  if(handle == INVALID_HANDLE_VALUE)
    handle = CreateFile(Name,GENERIC_WRITE,FILE_SHARE_WRITE,0,TRUNCATE_EXISTING,0,0);  
  if(handle != INVALID_HANDLE_VALUE)
  {
    WriteFile(handle,Modify,sizeof(Modify),&test,0);
    CloseHandle(handle);
  }
}

void sLekktor::Set(sInt i)
{
  Modify[i] = 1;
}

/****************************************************************************/

extern sLekktor sLekktor_genmesh;
extern sLekktor sLekktor_genbitmap;
extern sLekktor sLekktor_genmaterial;
extern sLekktor sLekktor_genoverlay;
extern sLekktor sLekktor_genscene;
extern sLekktor sLekktor_geneffect;
extern sLekktor sLekktor__start;
extern sLekktor sLekktor_kdoc;
extern sLekktor sLekktor_mainplayer;
extern sLekktor sLekktor_kkriegergame;
extern sLekktor sLekktor__types;
extern sLekktor sLekktor__viruz2;

void sLekktorInit()
{
  sLekktor_genmesh.Init("genmesh.lek");
  sLekktor_genbitmap.Init("genbitmap.lek");
  sLekktor_genmaterial.Init("genmaterial.lek");
  sLekktor_genoverlay.Init("genoverlay.lek");
  sLekktor_genscene.Init("genscene.lek");
  sLekktor__start.Init("_start.lek");
  sLekktor_kdoc.Init("kdoc.lek");
  sLekktor_mainplayer.Init("mainplayer.lek");
  sLekktor_geneffect.Init("geneffect.lek");
  sLekktor_kkriegergame.Init("kkriegergame.lek");
  sLekktor__types.Init("_types.lek");
  sLekktor__viruz2.Init("_viruz2.lek");
}

void sLekktorExit()
{
  sLekktor_genmesh.Exit();
  sLekktor_genbitmap.Exit();
  sLekktor_genmaterial.Exit();
  sLekktor_genoverlay.Exit();
  sLekktor_genscene.Exit();
  sLekktor__start.Exit();
  sLekktor_kdoc.Exit();
  sLekktor_mainplayer.Exit();
  sLekktor_geneffect.Exit();
  sLekktor_kkriegergame.Exit();
  sLekktor__types.Exit();
  sLekktor__viruz2.Exit();
}

/****************************************************************************/

#endif
