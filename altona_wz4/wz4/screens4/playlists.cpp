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

extern void LogTime();

/****************************************************************************/

template<class streamer> void PlaylistItem::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6401,4);
  if (version>0)
  {
    s | ID | Type | Path;
    s | Duration | TransitionId | TransitionDuration | ManualAdvance | Mute;
    if (version>=2) s | SlideType | MidiNote;
    if (version >= 3) s | Cached;
	if (version >= 4) s | CallbackUrl | CallbackDelay;

    s | BarColor | BarBlinkColor1 | BarBlinkColor2 | BarAlpha;
    s.ArrayAll(BarPositions);
  }
  s.Footer();
}

template<class streamer> void Playlist::Serialize_(streamer &s)
{
  sInt version = s.Header(sSerId::Werkkzeug4+0x6402,2);
  if (version>0)
  {
    s | ID | CallbackUrl | Timestamp | Loop;
    s | LastPlayedItem;
		if (version >= 2)
			s | CallbacksOn;
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
  CurrentDuration = CurrentSwitchTime = CurrentSlideTime = 0;
  CurrentPl = 0;
  SwitchHard = sFALSE;
  CallbackTriggered = sTRUE;
  RefreshTriggered = sTRUE;
  CallbackToCall = 0;
  PreparedSlide = 0;
  Time = 1.0; // one second phase shift into the future to hide the TARDIS
  CurRefreshing = 0;
  Locked = sFALSE;
	CallbacksOn = sFALSE;

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
        it->MyAsset = GetAsset(it->Path, it->Cached);
        it->MyAsset->AddRef();
        if (it->MyAsset->CacheStatus == Asset::NOTCACHED)
        {
          sScopeLock lock(&Lock);
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
    if (pl && pl->Items.GetCount()>0)
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
  CallbackThread = new sThread(CallbackThreadProxy, -1, 0, this);
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
  CallbackThread->Terminate();
  CallbackEvent.Signal();
  delete CallbackThread;
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
  LogTime(); sDPrintF(L"AddPlaylist %s (%d)\n",newpl->ID,newpl->Timestamp);
  
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
          LogTime(); sDPrintF(L"-> already in cache\n");
          pl->CallbacksOn = newpl->CallbacksOn;
          sRelease(newpl);
          RefreshAssets(pl);
          return;
        }
        LogTime(); sDPrintF(L"-> replacing (%d)\n",pl->Timestamp);
        replaced = pl;
        Playlists.Rem(pl);
        break;
      }
    }
    if (!replaced) LogTime(); sDPrintF(L"-> new\n");
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

NewSlideData* PlaylistMgr::OnFrame(sF32 delta, const sChar *doneId, sBool doneHard)
{
  {
    sScopeLock lock(&Lock);

    bool valid = CurrentPl && CurrentPl->Items.GetCount()>0 && CurrentPos.SlideNo >= 0;
    sString<64> curId =  valid ? CurrentPl->Items[CurrentPos.SlideNo]->ID : L"";

    // refresh next slide
    if (!RefreshTriggered && valid && (Time-CurrentSlideTime)>=CurrentPl->Items[CurrentPos.SlideNo]->TransitionDuration+0.2f)
    {
      PlaylistItem *current =  CurrentPl->Items[CurrentPos.SlideNo%CurrentPl->Items.GetCount()];
      PlaylistItem *next =  CurrentPl->Items[(CurrentPos.SlideNo+1)%CurrentPl->Items.GetCount()];
      if (sCmpStringI(next->Type,L"video"))
      {
        if (!next->MyAsset->RefreshNode.IsValid())
        {
          next->MyAsset->AddRef();
          RefreshList.AddHead(next->MyAsset);
        }
      }
	  RefreshTriggered = sTRUE;
    }

	// slide callbacks
	if (CallbacksOn && !CallbackTriggered && valid && CurrentPl->CallbacksOn &&
		!CurrentPl->Items[CurrentPos.SlideNo]->CallbackUrl.IsEmpty() &&
		(Time - CurrentSlideTime) > CurrentPl->Items[CurrentPos.SlideNo]->CallbackDelay)
	{
		LogTime(); sDPrintF(L"Calling slide callback\n");
		CallbackToCall = CurrentPl->Items[CurrentPos.SlideNo]->CallbackUrl;
		CallbackEvent.Signal();
		CallbackTriggered = sTRUE;
	}

    // auto advance
    if (CurrentSwitchTime>0 && (Time-CurrentSwitchTime)>=CurrentDuration)
    {
      CurrentSwitchTime = 0;
      Next(sFALSE, sTRUE);
    }
    else if (doneId != 0 && !sCmpString(doneId,curId))
    {
      Next(doneHard, sTRUE);
    }

    Time += delta;
  }

  NewSlideData *nsd = PreparedSlide;

  if (nsd && !nsd->Error)
  {
      switch (nsd->Type)
      {
      case VIDEO:
      {
          // wait for first frame to be fully decoded. must be in rendering thread, thus here.
          if (nsd->Movie->GetInfo().XSize<0)
          {
              nsd->Error = sTRUE;
              sRelease(nsd->Movie);
          }
          else
          {
              sFRect uvr;
              nsd->Movie->GetFrame(uvr);
              if (nsd->Movie->GetInfo().XSize == 0 || uvr.SizeX() == 0)
                  nsd = 0;
          }
      } break;
      case WEB:
      {
          // wait for web page to be rendered
          if (nsd->Web->Error)
          {
              nsd->Error = sTRUE;
              sRelease(nsd->Web);
          }
          else
          {
              sFRect uvr;
              nsd->Web->GetFrame(uvr);
              if (uvr.SizeX() == 0)
                  nsd = 0;
          }
      } break;
      }
  }

  if (nsd)
  {
    PreparedSlide = 0;
    CurrentSlideTime = Time;
	CallbackTriggered = sFALSE;
	RefreshTriggered = sFALSE;

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
  if (CurrentPl && CurrentPos.SlideNo>=0)
    sCopyString(slide, CurrentPl->Items[CurrentPos.SlideNo]->ID);
}

void PlaylistMgr::Seek(const sChar *plId, const sChar *slideId, sBool hard)
{
  LogTime(); sDPrintF(L"Seek %s:%s %s\n",plId, slideId, hard?L"(hard)":L"");
  Playlist *pl = GetPlaylist(plId);
  if (!pl) return;
  sInt slide = GetItem(pl,slideId);
  if (slide<0) return;
  RawSeek(pl, slide, hard);
}

void PlaylistMgr::Next(sBool hard, sBool force)
{
  LogTime(); sDPrintF(L"Next %s\n",hard?L"(hard)":L"");
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
      if (slide < 0)
        return;
    }
  } while (!force && !sCmpStringI(pl->Items[slide]->Type,L"siegmeister_winners"));
  RawSeek(pl, slide, hard);
}

void PlaylistMgr::Previous(sBool hard)
{
  LogTime(); sDPrintF(L"Prev %s\n",hard?L"(hard)":L"");
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

      if (slide<0)
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

Asset *PlaylistMgr::GetAsset(const sChar *path, sBool cache)
{
  sScopeLock lock(&Lock);
  Asset *found = sFind(Assets, &Asset::Path, path);
  if (!found)
  {
    found = new Asset;
    found->Path = path;

    if (cache)
    {
        found->CacheStatus = Asset::NOTCACHED;

        // check cache files for existence
        sString<sMAXPATH> filename, metafilename;
        MakeFilename(filename, found->Path, AssetDir);
        metafilename = filename;
        sAppendString(metafilename, L".meta");
        if (sCheckFile(filename))
        {
            if (sLoadObject(metafilename, &found->Meta))
            {
                found->CacheStatus = Asset::CACHED;
            }
        }
    }
    else
        found->CacheStatus = Asset::ONLINE;

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
      it->MyAsset = GetAsset(it->Path, it->Cached);
      it->MyAsset->AddRef();
    }

    if (it->MyAsset->CacheStatus != Asset::ONLINE)
    {
        it->MyAsset->CacheStatus = Asset::NOTCACHED;
        {
            sScopeLock lock(&Lock);
            if (!it->MyAsset->RefreshNode.IsValid())
            {
                it->MyAsset->AddRef();
                RefreshList.AddTail(it->MyAsset);
            }
        }
    }
  }
  pl->Release();
  AssetEvent.Signal();
}

void PlaylistMgr::RawSeek(Playlist *pl, sInt slide, sBool hard)
{
  if (Locked || !pl || slide < 0)
    return;

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

  if (CurrentPl && CurrentPl->Items.GetCount()>0)
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

  PlaylistItem *item = pl->Items.GetCount() > 0 ? pl->Items[CurrentPos.SlideNo] : 0;
  CurrentDuration = !item || item->ManualAdvance ? 0 : item->Duration+item->TransitionDuration;
  CurrentSwitchTime = 0;

  PrepareEvent.Signal();
}


/****************************************************************************/

void PlaylistMgr::PlCacheThreadFunc(sThread *t)
{
	int waitStart = 0;
  while (t->CheckTerminate())
  {
    if (PlCacheEvent.Wait(100))
      waitStart = sGetTime();

		if (waitStart==0 || (sGetTime()-waitStart < 3000 && t->CheckTerminate()))
			continue;
	
		waitStart = 0;
		LogTime(); sDPrintF(L"caching playlists\n");

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
     
		LogTime(); sDPrintF(L"caching playlists done\n");
  }
}

void PlaylistMgr::AssetThreadFunc(sThread *t)
{
  static const int DLSIZE = 524288;
  sU8 *dlBuffer = new sU8[DLSIZE];

  while (t->CheckTerminate())
  {
    Asset *toRefresh = 0;

    // find asset to refresh
    {
      sScopeLock lock(&Lock);
      if (!RefreshList.IsEmpty())
        toRefresh = RefreshList.RemHead();
    }

    if (!toRefresh || toRefresh->CacheStatus == Asset::ONLINE)
    {
      AssetEvent.Wait(100);
      continue;
    }

    CurRefreshing = toRefresh;
		LogTime(); sDPrintF(L"refreshing %s\n",toRefresh->Path);

    sString<sMAXPATH> filename, metafilename;
    MakeFilename(filename,toRefresh->Path,AssetDir);
    metafilename = filename;
    sAppendString(metafilename,L".meta");

    // send HTTP request...
    sHTTPClient client;
    client.SetETag(toRefresh->Meta.ETag);
    sBool result = client.Connect(toRefresh->Path);

    // .. and interpret response.
    int code; 
    client.GetStatus(code);
    if (!result)
    {
      LogTime(); sDPrintF(L"ERROR: connection failed refreshing %s\n",toRefresh->Path);
      if (toRefresh->CacheStatus == Asset::NOTCACHED)
        toRefresh->CacheStatus = Asset::INVALID;
    }
    else if (code>=400) // error :(
    {
      LogTime(); sDPrintF(L"ERROR: refreshing %s resulted in %d\n",toRefresh->Path, code);
      if (toRefresh->CacheStatus == Asset::NOTCACHED)
        toRefresh->CacheStatus = Asset::INVALID;
    }
    else if (code == 304) // already cached \o/
    {
      //LogTime(); sDPrintF(L"%s already cached\n",toRefresh->Path);
      toRefresh->CacheStatus = Asset::CACHED;
    }
    else if (code == 200) // not cached or updated, download
    {
      sString<sMAXPATH> downloadfilename;
      downloadfilename = filename;
      sAppendString(downloadfilename,L".tmp");

      sFile *file = sCreateFile(downloadfilename,sFA_WRITE);
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
        LogTime(); sDPrintF(L"ERROR: download of %s failed\n",toRefresh->Path);
        if (toRefresh->CacheStatus == Asset::NOTCACHED)
          toRefresh->CacheStatus = Asset::INVALID;
        sDeleteFile(downloadfilename);
      }
      else
      {
        LogTime(); sDPrintF(L"downloaded %s\n",toRefresh->Path);
        {
          sScopeLock lock(&Lock);
          sRenameFile(downloadfilename, filename, sTRUE);
          toRefresh->CacheStatus = Asset::CACHED;
          toRefresh->Meta.ETag = client.GetETag();
        }
        if (toRefresh->Meta.ETag==L"")
				{ LogTime(); sDPrintF(L"WARNING: no Etag!\n"); }
        sSaveObject(metafilename,&toRefresh->Meta);
      }
    }

    sRelease(toRefresh);
    CurRefreshing = 0;
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
      if (!CurrentPl || CurrentPl->Items.GetCount()==0) continue;
      item = CurrentPl->Items[CurrentPos.SlideNo];

      LogTime(); sDPrintF(L"preparing %s\n",item->Path);

      /*
      // TEST for video player as long as Partymeister doesn't give us mp4 files
      if (!sCmpString(item->ID,L"70_7041"))
      {
        item->Path=L"c:\\test\\yay.mp4";
        item->Type=L"Video";
        item->MyAsset->CacheStatus = Asset::CACHED;
        item->MyAsset->Path = item->Path;
        item->TransitionDuration = 0.5f;
        item->ManualAdvance = sFALSE;
      }
      */

      /*
      if (!sCmpString(item->ID, L"89_3376"))
      {
          item->Path = L"https://developer.cdn.mozilla.net/media/uploads/demos/s/u/sumantro/3dd07ab8f2a0ddd932babd86e442771c/geek-monkey-studios_1427572961_demo_package/index.html";
          //item->Path = L"http://2015.revision-party.net/";
          //item->Path = L"http://www.creativebloq.com/css3/animation-with-css3-712437";
          //item->Path = L"http://twitter.com/kebby";
          item->Type = L"Web";
          item->MyAsset->CacheStatus = Asset::ONLINE;
          item->MyAsset->Path = item->Path;
          item->Duration = 60;
          item->TransitionId = 255;
          item->TransitionDuration = 2.0f;
          item->ManualAdvance = sFALSE;
      }
      */

      nsd = new NewSlideData;
      nsd->Error = sFALSE;
      nsd->Id = item->ID;
      nsd->TransitionId = item->TransitionId;
      nsd->TransitionTime = SwitchHard ? 0 : item->TransitionDuration;
      nsd->RenderType = item->SlideType;
      nsd->MidiNote = item->MidiNote;

      if (!sCmpStringI(item->Type,L"Image"))
        nsd->Type = IMAGE;
      else if (!sCmpStringI(item->Type,L"Video"))
        nsd->Type = VIDEO;
      else if (!sCmpStringI(item->Type, L"Web"))
          nsd->Type = WEB;
      else if (!sCmpStringI(item->Type, L"siegmeister_bars"))
      {
        nsd->Type = SIEGMEISTER_BARS;
      }
      else if (!sCmpStringI(item->Type,L"siegmeister_winners"))
      {
        nsd->Type = SIEGMEISTER_WINNERS;
        nsd->TransitionTime = 0;
      }
      else
	    {
        nsd->Type = UNKNOWN;
		    nsd->Error = sTRUE;
		    LogTime(); sDPrintF(L"WARNING: got unknown slide type '%s'\n",item->Type);
	    }

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
    while (myAsset->CacheStatus == Asset::NOTCACHED && (myAsset==CurRefreshing || nccount++<400))
      sSleep(5);
   
    if (myAsset->CacheStatus == Asset::CACHED)
    {
      sString<sMAXPATH> filename;
      MakeFilename(filename,myAsset->Path,AssetDir);
      if (!sCheckFile(filename)) filename=myAsset->Path;

      switch (nsd->Type)
      {
      case IMAGE: case SIEGMEISTER_BARS: case SIEGMEISTER_WINNERS:
        {
          sFile *f = sCreateFile(filename);
          if (!f)
          {
            nsd->Error=sTRUE;
            return;
          }
          sU8 *ptr = f->MapAll();

          sImage *img = new sImage();

          sInt size = (sInt)f->GetSize();
          if (img->LoadPNG(ptr,size))
          {
            img->PMAlpha();
            nsd->ImgOpaque = !img->HasAlpha();
            nsd->ImgData = new sImageData(img,sTEX_2D|sTEX_ARGB8888);
            nsd->OrgImage = img;
          }
          else
          {
            // if the internal image loader fails, try the Windows one
            delete img;
            img = sLoadImageWin32(f);
            if (img)
            {
              img->PMAlpha();
              nsd->ImgOpaque = !img->HasAlpha();
              nsd->ImgData = new sImageData(img,sTEX_2D|sTEX_ARGB8888);
              nsd->OrgImage = img;
            }
            else
            {
              sDPrintF(L"Error loading %s\n",myAsset->Path);
              nsd->Error = sTRUE;
            }
          }
          delete f;
        } break;
      case VIDEO:
        {
          nsd->Movie = sCreateMoviePlayer(filename,sMOF_DONTSTART|(item->Mute?sMOF_NOSOUND:0));
          if (nsd->Movie)
          {
            
          }
          else
          {
            nsd->Error = sTRUE;
          }
        } break;
      }
    }
    else if (myAsset->CacheStatus == Asset::ONLINE)
    {
        switch (nsd->Type)
        {
        case WEB:
        {
            sInt w, h;
            sGetScreenSize(w, h);
            nsd->Web = new WebView(myAsset->Path, w, h);
            /*
            if (nsd->Movie)
            {

            }
            else
            {
                nsd->Error = sTRUE;
            }
            */
        } break;
        }
    }
    else
    {
      LogTime();
      sDPrintF(L"prepare: cache failed");
      nsd->Error=sTRUE;
    }
   

    {
      sScopeLock lock(&Lock);
      myAsset->Release();
      sDelete(PreparedSlide);
      PreparedSlide = nsd;
    }

		LogTime(); sDPrintF(L"prepare done\n");
  }
}

void PlaylistMgr::CallbackThreadFunc(sThread *t)
{
	while (t->CheckTerminate())
	{
		const sChar *toCall = CallbackToCall;
		CallbackToCall = 0;
		if (!toCall)
		{
			CallbackEvent.Wait(100);
			continue;
		}

		LogTime(); sDPrintF(L"Calling %s\n", toCall);

		// send HTTP request...
		sHTTPClient client;
		sBool result = client.Connect(toCall);

		// .. and interpret response.
		int code;
		client.GetStatus(code);
		if (!result)
		{
			LogTime(); sDPrintF(L"ERROR: connection failed calling %s\n", toCall);
		}
		else if (code >= 400) // error :(
		{
			LogTime(); sDPrintF(L"ERROR: calling %s resulted in %d\n", toCall, code);
		}

		client.Disconnect();
	}
}