/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#define sPEDANTIC_WARN 1
#include "playlists.hpp"
#include "network/http.hpp"
#include "util/image.hpp"
#include "image_win.hpp"

/****************************************************************************/

template<class streamer> void PlaylistItem::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6401,1);
  if (version>0)
  {
    s | ID | Type | Path;
    s | Duration | TransitionId | TransitionDuration | ManualAdvance | Mute;
    
    s | BarColor | BarBlinkColor1 | BarBlinkColor2 | BarAlpha;
    s.ArrayAll(BarPositions);
  }
  s.Footer();
}

template<class streamer> void Playlist::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6402,1);
  if (version>0)
  {
    s | ID | CallbackUrl | Timestamp | Loop;
    s | LastPlayedItem;
    s.ArrayAllPtr(Items);
  }
  s.Footer();

  Dirty = sFALSE;
}

template<class streamer> void AssetMetaData::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6403,1);
  if (version>0)
  {
    s | ETag;
  }
  s.Footer();
}

template<class streamer> void PlayPosition::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6404,1);
  if (version>0)
  {
    s | PlaylistId | SlideNo;
  }
  s.Footer();

  Dirty = sFALSE;
}

/****************************************************************************/

static sU32 ParseColor(const sChar *str)
{
  sU32 color = 0xffff007f;
  int len = str ? sGetStringLen(str) : 0;
  if (len>0 && str[0]=='#') 
  {
    sInt val;
    if (sScanHex(++str,val,6))
    {
      if (len>=7)
        color = 0xff000000 | val;
      else
        color = 0xff000000 | ((val&0xf00)*0x1100) | ((val&0xf0)*0x110) | ((val&0xf)*0x11);
    }
  }
  return color;
}

/****************************************************************************/

PlaylistMgr::PlaylistMgr()
{
  // set up cache directories
  CacheDir = L"cache/";
  PlaylistDir = L"cache/playlists/";
  AssetDir = L"cache/assets/";
  sMakeDirAll(PlaylistDir);
  sMakeDirAll(AssetDir);

  // initialize the miscallanea
  Assets.HintSize(1024);
  CurrentDuration = CurrentSwitchTime = 0;
  CurrentPl = 0;
  SwitchHard = sFALSE;
  PreparedSlide = 0;
  Time = 1.0; // one second phase shift into the future to hide the TARDIS

  // load all cached playlists
  sArray<sDirEntry> dir;
  sLoadDir(dir, PlaylistDir, L"*.pl");
  sDirEntry *de;
  sFORALL(dir,de)
  {
    sString<sMAXPATH> path = PlaylistDir;
    sAppendPath(path, de->Name);
    Playlist *pl = new Playlist;  
    if (sLoadObject<Playlist>(path, pl))
    {
      Playlists.AddTail(pl);
      PlaylistItem *it;
      sFORALL(pl->Items, it)
      {
        it->MyAsset = GetAsset(it->Path);
        it->MyAsset->AddRef();
        if (it->MyAsset->CacheStatus == Asset::NOTCACHED)
        {
          RefreshList.AddTail(it->MyAsset);
          it->MyAsset->AddRef();
        }
      }
    }
    else
      pl->Release();

  }


  // load last play/loop position
  {
    sString<sMAXPATH> fn = CacheDir; sAppendString(fn, L"LastLoopPos");
    sLoadObject<PlayPosition>(fn,&LastLoopPos);
    if (!GetPlaylist(LastLoopPos.PlaylistId))
      LastLoopPos.PlaylistId=L"";
  }
  {
    sString<sMAXPATH> fn = CacheDir; sAppendString(fn, L"CurrentPos");
    sLoadObject<PlayPosition>(fn,&CurrentPos);
    Playlist *pl = GetPlaylist(CurrentPos.PlaylistId);
    if (pl)
    {
      CurrentPos.SlideNo = sClamp(CurrentPos.SlideNo,0,pl->Items.GetCount()-1);
      Seek(CurrentPos.PlaylistId, pl->Items[CurrentPos.SlideNo]->ID, sTRUE);
    }
    else
    {
      CurrentPos.PlaylistId=L"";
    }
  }

  // we want to download uncached assets please
  AssetEvent.Signal();

  // .. and start all worker threads
  PlCacheThread = new sThread(PlCacheThreadProxy, -1, 0, this);
  AssetThread = new sThread(AssetThreadProxy, -1, 0, this);
  PrepareThread = new sThread(PrepareThreadProxy, -1, 0, this);
}

PlaylistMgr::~PlaylistMgr()
{
  PlCacheThread->Terminate();
  PlCacheEvent.Signal();
  delete PlCacheThread;
  AssetThread->Terminate();
  AssetEvent.Signal();
  delete AssetThread;
  PrepareThread->Terminate();
  PrepareEvent.Signal();
  delete PrepareThread;
  while (!Playlists.IsEmpty())
    Playlists.RemHead()->Release();

  while (!RefreshList.IsEmpty())
    RefreshList.RemHead()->Release();
  sReleaseAll(Assets);

  sRelease(CurrentPl);
  delete PreparedSlide;

}

// Adds new play list to pool (takes ownership of pl)
void PlaylistMgr::AddPlaylist(Playlist *newpl)
{
  if (!newpl) return;
  sLogF(L"pl",L"AddPlaylist %s (%d)\n",newpl->ID,newpl->Timestamp);
  
  Playlist *replaced = 0;
  // add new playlist (if not already existing)
  {
    sScopeLock lock(&Lock);
    Playlist *pl;

    // check if playlist is already loaded
    sFORALL_LIST(Playlists, pl)
    {
      if (!sCmpString(newpl->ID,pl->ID))
      {
        if (newpl->Timestamp <= pl->Timestamp && newpl->Items.GetCount() == pl->Items.GetCount())
        {
          sLogF(L"pl",L"-> already in cache\n");
          RefreshAssets(pl);
          return;
        }
        sLogF(L"pl",L"-> replacing (%d)\n",pl->Timestamp);
        replaced = pl;
        Playlists.Rem(pl);
        break;
      }
    }
    if (!replaced) sLogF(L"pl",L"-> new\n");
    Playlists.AddTail(newpl);
  }

  RefreshAssets(newpl);

  // playlist replaced? clean up and modify all pointers
  if (replaced)
  {
    sScopeLock lock(&Lock);
    if (CurrentPl == replaced)
    {
      sRelease(CurrentPl);
      CurrentPl = newpl;
      sAddRef(CurrentPl);
      CurrentPos.SlideNo = sClamp(CurrentPos.SlideNo,0,newpl->Items.GetCount()-1);
    }
  }

  sRelease(replaced);
  PlCacheEvent.Signal();
}

NewSlideData* PlaylistMgr::OnFrame(sF32 delta)
{
  sScopeLock lock(&Lock);

  // auto advance
  if (CurrentSwitchTime>0 && (Time-CurrentSwitchTime)>=CurrentDuration)
  {
    CurrentSwitchTime = 0;
    Next(sFALSE, sTRUE);
  }

  Time += delta;

  NewSlideData *nsd = PreparedSlide;
  PreparedSlide = 0;
  if (nsd)
  {
    // prepare auto advance
    if (CurrentDuration>0)
      CurrentSwitchTime = Time;
  }
  return nsd;
}

sBool PlaylistMgr::OnInput(const sInput2Event &ev)
{
  sU32 key = ev.Key;
  if (key&sKEYQ_SHIFT) key|=sKEYQ_SHIFT;
  if (key&sKEYQ_CTRL) key|=sKEYQ_CTRL;
  if (key&sKEYQ_ALT) key|=sKEYQ_ALT;
  switch (key)
  {
    // previous
  case sKEY_LEFT:
  case sKEY_LEFT|sKEYQ_SHIFT:
    Previous(!!(key&sKEYQ_SHIFT));
    return sTRUE;

    // next
  case sKEY_RIGHT:
  case sKEY_RIGHT|sKEYQ_SHIFT:
    Next(!!(key&sKEYQ_SHIFT), sFALSE);
    return sTRUE;

    // skip to beginning
  case sKEY_LEFT|sKEYQ_CTRL:
  case sKEY_LEFT|sKEYQ_CTRL|sKEYQ_SHIFT:
    {
      sScopeLock lock(&Lock);
      RawSeek(CurrentPl, 0, !!(key&sKEYQ_SHIFT));
    }
    return sTRUE;

    // skip to end
  case sKEY_RIGHT|sKEYQ_CTRL:
  case sKEY_RIGHT|sKEYQ_CTRL|sKEYQ_SHIFT:
    {
      sScopeLock lock(&Lock);
      RawSeek(CurrentPl, CurrentPl->Items.GetCount()-1, !!(key&sKEYQ_SHIFT));
    }
    return sTRUE;
  }
  return sFALSE;
}

Playlist *PlaylistMgr::GetBegin()
{
  Lock.Lock();
  if (Playlists.IsEmpty())
  {
    Lock.Unlock();
    return 0;
  }
  return Playlists.GetHead();
}

Playlist *PlaylistMgr::GetNext(Playlist *pl)
{
  if (!pl) return 0;
  pl = Playlists.GetNext(pl);
  if (Playlists.IsEnd(pl))
  {
    Lock.Unlock();
    return 0;
  }
  return pl;
}

void PlaylistMgr::GetCurrentPos(const sStringDesc &pl, const sStringDesc &slide)
{
  sScopeLock lock(&Lock);
  sCopyString(pl, CurrentPos.PlaylistId);
  if (CurrentPl)
    sCopyString(slide, CurrentPl->Items[CurrentPos.SlideNo]->ID);
}

void PlaylistMgr::Seek(const sChar *plId, const sChar *slideId, sBool hard)
{
  sLogF(L"pl",L"Seek %s:%s %s\n",plId, slideId, hard?L"(hard)":L"");
  Playlist *pl = GetPlaylist(plId);
  if (!pl) return;
  sInt slide = GetItem(pl,slideId);
  if (slide<0) return;
  RawSeek(pl, slide, hard);
}

void PlaylistMgr::Next(sBool hard, sBool force)
{
  sLogF(L"pl",L"Next %s\n",hard?L"(hard)":L"");
  if (!CurrentPl) return;
  Playlist *pl = CurrentPl;
  sInt slide = CurrentPos.SlideNo;
  do
  {
    slide++;
    if (slide >= pl->Items.GetCount())
    {
      if (pl->Loop)
      {
        slide = 0;
      }
      else
      {
        pl = GetPlaylist(LastLoopPos.PlaylistId);
        if (!pl) return;
        slide = LastLoopPos.SlideNo;
      }
    }
  } while (!force && !sCmpStringI(pl->Items[slide]->Type,L"siegmeister_winners"));
  RawSeek(pl, slide, hard);
}

void PlaylistMgr::Previous(sBool hard)
{
  sLogF(L"pl",L"Prev %s\n",hard?L"(hard)":L"");
  if (!CurrentPl) return;
  Playlist *pl = CurrentPl;
  sInt slide = CurrentPos.SlideNo;
  do
  {
    slide--;
    if (slide<0) 
    {
      if (pl->Loop)
        slide = pl->Items.GetCount()-1;
      else
        return;
    }
  } while (!sCmpStringI(pl->Items[slide]->Type,L"siegmeister_winners"));
  RawSeek(pl, slide, hard);
}


/****************************************************************************/

void PlaylistMgr::MakeFilename(const sStringDesc& buffer, const sChar *id, const sChar *path, const sChar *ext)
{
  sChecksumMD5 md5;
  md5.Calc((const sU8*)id,sizeof(sChar)*sGetStringLen(id));
  sSPrintF(buffer,L"%s%08x-%08x-%08x-%08x%s",path,md5.Hash[0],md5.Hash[1],md5.Hash[2],md5.Hash[3],ext);
}

Playlist *PlaylistMgr::GetPlaylist(const sChar *id)
{
  sScopeLock lock(&Lock);
  Playlist *pl;
  sFORALL_LIST(Playlists,pl)
    if (pl->ID == id)
      return pl;
  return 0;
}

sInt PlaylistMgr::GetItem(Playlist *pl, const sChar *id)
{
  if (!pl) return -1;
  for (sInt i=0; i<pl->Items.GetCount(); i++)
    if (pl->Items[i]->ID == id)
      return i;
  return -1;
}

Asset *PlaylistMgr::GetAsset(const sChar *path)
{
  sScopeLock lock(&Lock);
  Asset *found = sFind(Assets, &Asset::Path, path);
  if (!found)
  {
    found = new Asset;
    found->Path = path;
    found->CacheStatus = Asset::NOTCACHED;

    // check cache files for existence
    sString<sMAXPATH> filename, metafilename;
    MakeFilename(filename,found->Path,AssetDir);
    metafilename = filename;
    sAppendString(metafilename,L".meta");
    if (sCheckFile(filename))
    {
      if (sLoadObject(metafilename,&found->Meta))
      {
        found->CacheStatus = Asset::CACHED;
      }
    }

    Assets.AddTail(found);
  }
  return found;
}

void PlaylistMgr::RefreshAssets(Playlist *pl)
{
  PlaylistItem *it;
  pl->AddRef();
  sFORALL(pl->Items, it)
  {
    if (!it->MyAsset)
    {
      it->MyAsset = GetAsset(it->Path);
      it->MyAsset->AddRef();
    }
    it->MyAsset->CacheStatus = Asset::NOTCACHED;
    {
      sScopeLock lock(&Lock);
      it->MyAsset->AddRef();
      RefreshList.AddTail(it->MyAsset);
    }
  }
  pl->Release();
  AssetEvent.Signal();
}

void PlaylistMgr::RawSeek(Playlist *pl, sInt slide, sBool hard)
{
  sScopeLock lock(&Lock);

  SwitchHard = hard;
  if (pl==CurrentPl && pl->Timestamp == CurrentPlTime && slide==CurrentPos.SlideNo)  
     return;

  if (CurrentPl && CurrentPl->Loop)
  {
    LastLoopPos = CurrentPos;
    LastLoopPos.Dirty = sTRUE;
  }

  sRelease(CurrentPl);
  CurrentPl = pl;
  CurrentPlTime = pl->Timestamp;
  sAddRef(CurrentPl);

  if (CurrentPl)
  {
    CurrentPos.PlaylistId = CurrentPl->ID;
    CurrentPos.SlideNo = sClamp(slide,0,pl->Items.GetCount()-1);
    CurrentPl->LastPlayedItem = CurrentPos.SlideNo;
    CurrentPl->Dirty = sTRUE;
  }
  else
  {
    CurrentPos.PlaylistId = L"";
    CurrentPos.SlideNo = 0;
  }
  CurrentPos.Dirty = sTRUE;
  PlCacheEvent.Signal();

  PlaylistItem *item = pl->Items[CurrentPos.SlideNo];
  CurrentDuration = item->ManualAdvance ? 0 : item->Duration+item->TransitionDuration;
  CurrentSwitchTime = 0;

  PrepareEvent.Signal();
}


/****************************************************************************/

void PlaylistMgr::PlCacheThreadFunc(sThread *t)
{
  while (t->CheckTerminate())
  {
    if (!PlCacheEvent.Wait(100))
      continue;

    // get all dirty playlists
    sDList<Playlist, &Playlist::CacheNode> dirtyLists;
    Playlist *pl;
    {
      sScopeLock lock(&Lock); // CAUTION: priority inversion?
      sFORALL_LIST(Playlists, pl)
        if (pl->Dirty)
        {
          pl->AddRef();
          dirtyLists.AddTail(pl);
        }
    }

    // write all dirty playlists
    while (!dirtyLists.IsEmpty())
    {
      pl = dirtyLists.RemHead();
      sString<sMAXPATH> fn;
      MakeFilename(fn, pl->ID, PlaylistDir, L".pl");
      sSaveObject(fn, pl);
      pl->Dirty = sFALSE;
      pl->Release();
    }

    // positions?
    if (CurrentPos.Dirty)
    {
      sString<sMAXPATH> fn = CacheDir; sAppendString(fn, L"CurrentPos");
      sSaveObject(fn, &CurrentPos);
    }

    if (LastLoopPos.Dirty)
    {
      sString<sMAXPATH> fn = CacheDir; sAppendString(fn, L"LastLoopPos");
      sSaveObject(fn, &LastLoopPos);
    }
      
  }
}

void PlaylistMgr::AssetThreadFunc(sThread *t)
{
  Asset *toRefresh = 0;

  static const int DLSIZE = 524288;
  sU8 *dlBuffer = new sU8[DLSIZE];

  while (t->CheckTerminate())
  {
    if (RefreshList.IsEmpty() && !AssetEvent.Wait(100))
      continue;

    // find asset to refresh
    {
      sScopeLock lock(&Lock);
      if (!toRefresh) toRefresh = RefreshList.RemHead();
    }

    if (toRefresh)
    {
      sString<sMAXPATH> filename, metafilename;
      MakeFilename(filename,toRefresh->Path,AssetDir);
      metafilename = filename;
      sAppendString(metafilename,L".meta");

      // send HTTP request...
      sHTTPClient client;
      client.SetETag(toRefresh->Meta.ETag);
      client.Connect(toRefresh->Path);

      // .. and interpret response.
      int code; 
      client.GetStatus(code);
      if (code>=400) // error :(
      {
        sLogF(L"pl",L"ERROR: refreshing %s resulted in %d\n",toRefresh->Path, code);
        sDeleteFile(filename);
        sDeleteFile(metafilename);
        toRefresh->CacheStatus = Asset::INVALID;
      }
      else if (code == 304) // already cached \o/
      {
        //sLogF(L"pl",L"%s already cached\n",toRefresh->Path);
        toRefresh->CacheStatus = Asset::CACHED;
      }
      else if (code == 200) // not cached or updated, download
      {
        sFile *file = sCreateFile(filename,sFA_WRITE);
        sBool error = sFALSE;

        for (;;)
        {
          sDInt read;
          error = !client.Read(dlBuffer,DLSIZE,read);
          if (error || read==0)
            break;
          file->Write(dlBuffer,read);
        }

        delete file;
        if (error)
        {
          sLogF(L"pl",L"ERROR: download of %s failed\n",toRefresh->Path);
          toRefresh->CacheStatus = Asset::INVALID;
          sDeleteFile(filename);
          sDeleteFile(metafilename);
        }
        else
        {
          sLogF(L"pl",L"downloaded %s\n",toRefresh->Path);
          toRefresh->CacheStatus = Asset::CACHED;
          toRefresh->Meta.ETag = client.GetETag();
          if (toRefresh->Meta.ETag==L"")
            sLogF(L"pl",L"WARNING: no Etag!\n");
          sSaveObject(metafilename,&toRefresh->Meta);
        }
      }

      sRelease(toRefresh);
    }
  }

  delete[] dlBuffer;
}

void PlaylistMgr::PrepareThreadFunc(sThread *t)
{
  while (t->CheckTerminate())
  {
    if (!PrepareEvent.Wait(100))
      continue;
    if (!t->CheckTerminate())
      break;

    NewSlideData *nsd;
    PlaylistItem *item;
    Asset *myAsset;
    {
      sScopeLock lock(&Lock);
      if (!CurrentPl) continue;
      item = CurrentPl->Items[CurrentPos.SlideNo];

      nsd = new NewSlideData;
      nsd->ImgData = 0;
      nsd->Error = sFALSE;
      nsd->TransitionId = item->TransitionId;
      nsd->TransitionTime = SwitchHard ? 0 : item->TransitionDuration;

      if (!sCmpStringI(item->Type,L"Image"))
        nsd->Type = IMAGE;
      else if (!sCmpStringI(item->Type,L"siegmeister_bars"))
      {
        nsd->Type = SIEGMEISTER_BARS;
      }
      else if (!sCmpStringI(item->Type,L"siegmeister_winners"))
      {
        nsd->Type = SIEGMEISTER_WINNERS;
        nsd->TransitionTime = 0;
      }
      else
        nsd->Type = UNKNOWN;

      myAsset = item->MyAsset;
      myAsset->AddRef();

      if (nsd->Type == SIEGMEISTER_BARS || nsd->Type == SIEGMEISTER_WINNERS)
      {
        SiegmeisterData *sd = new SiegmeisterData;

        sd->BarColor = ParseColor(item->BarColor);
        sd->BarBlinkColor1 = ParseColor(item->BarBlinkColor1);
        sd->BarBlinkColor2 = ParseColor(item->BarBlinkColor2);
        sd->BarAlpha = item->BarAlpha;
        sd->BarPositions.Copy(item->BarPositions);
        sSortDown(sd->BarPositions, &sFRect::x1);
        sd->Winners = nsd->Type == SIEGMEISTER_WINNERS;

        nsd->SiegData = sd;
      }
    }

    // ok, let's just hope any NOTCACHED asset will eventually...
    sInt nccount = 0;
    while (myAsset->CacheStatus == Asset::NOTCACHED && nccount++<200)
      sSleep(5);
   
    if (myAsset->CacheStatus == Asset::CACHED)
    {
      sString<sMAXPATH> filename;
      MakeFilename(filename,myAsset->Path,AssetDir);
      sFile *f = sCreateFile(filename);
      sU8 *ptr = f->MapAll();

      // TODO: support several asset types
      sImage img;
      sInt size = (sInt)f->GetSize();
      if (img.LoadPNG(ptr,size))
      {
        img.PMAlpha();
        nsd->ImgData = new sImageData(&img,sTEX_2D|sTEX_ARGB8888);
      }
      else
      {
        // if the internal image loader fails, try the Windows one
        sImage *img2 = sLoadImageWin32(f);
        if (img2)
        {
          img2->PMAlpha();
          nsd->ImgData = new sImageData(img2,sTEX_2D|sTEX_ARGB8888);
          delete img2;
        }
        else
        {
          sDPrintF(L"Error loading %s\n",myAsset->Path);
          nsd->Error = sTRUE;
        }
      }
      delete f;
    }
   

    {
      sScopeLock lock(&Lock);
      myAsset->Release();
      sDelete(PreparedSlide);
      PreparedSlide = nsd;
    }
  }
}

