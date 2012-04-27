/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "netdebug.hpp"

#if sNETDEBUG_ENABLE

#include "base/system.hpp"
#include "base/windows.hpp"
#include "base/graphics.hpp"
#include "util/image.hpp"
#include "util/perfmon.hpp"
#include "network/sockets.hpp"
#include "network/http.hpp"

/****************************************************************************/

bool operator< (const sMemoryLeakInfo &a, const sMemoryLeakInfo &b)
{
  const sChar8 *pa=a.File;
  const sChar8 *pb=b.File;
  if (!pb) return sFALSE;
  if (!pa) return sTRUE;
  while (*pa || *pb)
  {
    if (*pa<*pb) return sTRUE;
    if (*pa>*pb) return sFALSE;
    pa++;
    pb++;
  }
  return a.Line<b.Line;
}

namespace sNetDebug
{

  /****************************************************************************/

  static const sChar8 Stylesheet[] = 
    "body {"
      "margin-left:20px;"
      "margin-right:10px;"
      "margin-top:10px;"
      "margin-bottom:10px;"
      "font:normal 10pt Arial, Helvetica;"
      "vertical-align:middle;"
      "color:#FFCC99;"
      "border-width:0;"
      "background-color:#000000;"
    "}"

    "div#menubar {"
      "background-color:#303030;"
      "background: -moz-linear-gradient(left,#303030,#606060);"
      "padding: 5px;"
      "border-width: 5px;"
      "-moz-border-radius: 5px;"
      "max-width: 100%;"
      "vertical-align: middle;"
      "overflow: auto;"
      "position: relative;"
    "}"

    "a:link 		{ color:#FF9966; text-decoration:none; }"
    "a:active 	{ color:#FFCC99; text-decoration:none; }"
    "a:visited 	{ color:#FF9966; text-decoration:none; }"
    "a:hover 	{ color:#FFDDAA; text-decoration:underline; }"
    "img 		{ border-width:0; }"
    ".cfgvalue 	{ border-left:1px solid; border-right:1px solid; border-top:0px none; border-bottom:0px none; padding:2px 6px;}"
    ".cfgheader { border-bottom:1px solid;background-color:#333333; }"
    ".cfgblock  { border:1px solid; margin:16px 0px 16px 32px; }"
    "input.text { background-color:#202020; color:#ffcc99; border:0px; }"
    "input.btn  { background-color:#203050; color:#ffcc99; border:1px solid; }"
    ;


  static const sChar8 Frameset[] = 
    "<html>"
      "<head>"
		    "<title>Altona Debug Server</title>"
		    "<meta content=\"text/html\" http-equiv=Content-Type>"
	    "</head>"
	    "<frameset rows=\"40,*\" frameborder=0 framespacing=0 border=0>"
		    "<frame src=\"menubar\" name=\"menubar\" scrolling=no noresize marginwidth=0 marginheight=1>"
		    "<frame src=\"debug\" name=\"main\" noresize marginwidth=0 marginheight=0>"
	    "</frameset>"
	    "<body>Sorry, you need a frame capable browser for viewing this page</body>"
    "</html>"
    ;

  /****************************************************************************/
  /****************************************************************************/

  static const sChar8 DbgViewHeader[] = 
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
    "<html><body><pre>\n";

  static const sChar DbgViewScroller[] = 
    L"<script language=\"javascript\">window.scrollBy(0,%d);</script>\n";

  class DebugView : public sHTTPServer::Handler
  {
  public:

    static const sInt BUFFERSIZE=65536; // power of 2
    static sU8 Buffer[BUFFERSIZE];
    static volatile sInt WritePtr;

    sInt HdrDone;
    sInt ScrlDone;
    sInt LastTime;
    sInt ReadPtr;
    sInt NewlineCount;

    static const sInt ScrollerSize=sCOUNTOF(DbgViewScroller)+16;
    sChar8 Scroller[ScrollerSize];
    sInt   ScrlLen;

    sHTTPServer::HandlerResult Init(sHTTPServer::Connection *conn)
    {
      LastTime=sGetTime();
      HdrDone=0;
      ScrlDone=ScrlLen=0;
      NewlineCount=0;
      ReadPtr=WritePtr;
      return sHTTPServer::HR_OK;
    }

    sBool DataAvailable()
    {
      sInt time=sGetTime();
      if ((sU32) HdrDone<sizeof(DbgViewHeader)) return sTRUE;
      if (NewlineCount && LastTime+100<time)
      {
        sString<ScrollerSize> scrl;
        sSPrintF(scrl,DbgViewScroller,NewlineCount*100);
        sCopyString(Scroller,scrl,ScrollerSize);
        ScrlLen=sGetStringLen(scrl);
        ScrlDone=0;
        return sTRUE;
      }
      if (LastTime+5000<time) return sTRUE;
      return ReadPtr!=WritePtr;
    }

    sInt GetData(sU8 *buffer, sInt len)
    {
      LastTime=sGetTime();

      // header?
      if ((sU32) HdrDone<sizeof(DbgViewHeader))
      {
        sInt reallen=sMin<sInt>(sizeof(DbgViewHeader)-HdrDone,len);
        sCopyMem(buffer,DbgViewHeader+HdrDone,reallen);
        HdrDone+=reallen;
        return reallen;
      }

      if (ScrlDone<ScrlLen)
      {
        sInt reallen=sMin<sInt>(ScrlLen-ScrlDone,len);
        sCopyMem(buffer,Scroller+ScrlDone,reallen);
        ScrlDone+=reallen;
        if (ScrlDone==ScrlLen) NewlineCount=0;
        return reallen;
      }

      // idle? send something invisible
      if (ReadPtr==WritePtr)
      {
        sVERIFY(len>=11);
        sCopyMem(buffer,"<div></div>",11);
        return 11;
      }

      // send debug output
      const sInt wp=WritePtr;
      sInt reallen=sMin((BUFFERSIZE+wp-ReadPtr)%BUFFERSIZE,len);
      sInt pass1=sMin(reallen,BUFFERSIZE-ReadPtr);
      sCopyMem(buffer,Buffer+ReadPtr,pass1);
      if (pass1<reallen)
        sCopyMem(buffer+pass1,Buffer,reallen-pass1);
      ReadPtr=wp;

      // count newlines
      for (sInt i=0; i<reallen; i++)
        if (buffer[i]=='\n')
          NewlineCount++;

      return reallen;
    }

    void GetAdditionalHeaders(const sStringDesc &str)
    {
      // the browser needs this so it can render the page instantly
      // without having to detect the type
      sSPrintF(str,L"Content-Type: text/html; charset=ISO-8859-1\r\nExpires: 0\r\n");
    }

    static void DPrint(const sChar* txt,void *user)
    {
      sChar c;
      while ((c=*txt++)!=0)
      {
        sChar8 buf[8];
        sInt len=sGetHTMLChar(c,buf);
        for (sInt i=0; i<len; i++)
        {
          Buffer[WritePtr]=buf[i]; 
          WritePtr=(WritePtr+1)&(BUFFERSIZE-1);
        }
      }
    }

    static void StaticInit()
    {
      sDebugOutHook->Add(DPrint);
    }

    static void StaticExit()
    {
      sDebugOutHook->Rem(DPrint);
    }

    static Handler *Factory() { return new DebugView; }
  };

  sU8 DebugView::Buffer[65536];
  volatile sInt DebugView::WritePtr=0;

  /****************************************************************************/
  /****************************************************************************/

#if sRENDERER!=sRENDER_BLANK

  class Screenshot : public sHTTPServer::Handler
  {
  public:

    static volatile sInt Done;   
    static sThreadLock *Lock;

    static sU8 * volatile Image;
    static volatile sInt ImageSize;

    sInt ImagePos;

    static void FlipHandler(void *user)
    {
      sPreFlipHook->Rem(FlipHandler);

      Lock->Lock();
      sPushMemType(sAMF_DEBUG);

      sDeleteArray(Image);

      sInt  xs,ys;
      sSetTarget(sTargetPara(sST_CLEARNONE,0,0));
      sGetRendertargetSize(xs,ys);

      const sU8 *data;
      sS32 pitch;
      sTextureFlags flags;
      sBeginSaveRT(data,pitch,flags);
      if(flags==sTEX_ARGB8888)
      {
        ImageSize = xs*ys*3+16384; // should be ok
        Image = new sU8[ImageSize];

        sImage img;
        img.Init(xs,ys);
        sU8 *ptr=(sU8*)img.Data;
        for (sInt y=0; y<ys; y++, ptr+=4*xs)
          sCopyMem(ptr,data+pitch*y,4*xs);

        ImageSize = img.SaveBMP(Image,ImageSize);
      }
      sEndSaveRT();

      Done=1;
      sPopMemType(sAMF_DEBUG);
      Lock->Unlock();
    }

    static void StaticInit()
    {
      Lock=new sThreadLock;
    }

    static void StaticExit()
    {
      sDeleteArray(Image);
      sDelete(Lock);
    }

    sHTTPServer::HandlerResult Init(sHTTPServer::Connection *conn)
    {
      Done=0;
      ImagePos=0;
      sPreFlipHook->Add(FlipHandler);

      return sHTTPServer::HR_OK;
    }

    sBool DataAvailable()
    {
      return Done;
    }

    sInt GetData(sU8 *buffer, sInt len)
    {
      Lock->Lock();
      sInt reallen=sMin(len,ImageSize-ImagePos);
      sCopyMem(buffer,Image+ImagePos,reallen);
      ImagePos+=reallen;
      Lock->Unlock();
      return reallen;
    }

    void GetAdditionalHeaders(const sStringDesc &str)
    {
      sSPrintF(str,L"Content-Type: image/bmp\r\n");
    }
    static Handler *Factory() { return new Screenshot; }
  };

  volatile sInt Screenshot::Done=0;
  sU8 * volatile Screenshot::Image=0;
  volatile sInt Screenshot::ImageSize=0;
  sThreadLock *Screenshot::Lock=0;

#endif

  /****************************************************************************/
  /****************************************************************************/

  static const sChar *ModeNames[]=
  {
    L"Location",
    L"Description",
  };

  static const sChar *MemTypeNames[16]=
  {
    L"All",
    L"Heap",
    L"Debug",
    L"Graphics",
    L"Network",
    L"Xtra1",
    L"Xtra2",
    L"User07",L"User08",L"User09",L"User10",L"User11",L"User12",L"User13",L"User14",L"User15",
  };

  static const sChar *MemStatSort[2][3]=
  {
    { L"File", L"Size", L"Count", },
    { L"Description", L"Size", L"Count", },
  };

  static const sChar *MemStatToggle[]=
  {
    L"Off",
    L"On",
  };

  class MemStat : public sHTTPServer::SimpleHandler
  {
  public:

    enum
    {
      MODE_LOC,
      MODE_DESC,
    };

    struct Entry
    {
      sString<128> File;
      sInt Line;
      sInt Count;
      sInt Size;
      sInt HeapId;

      bool operator < (const Entry &b) const
      {
        sInt cmp=sCmpString(File,b.File);
        if (cmp<0) return true;
        if (cmp>0) return false;
        if (Line<b.Line) return true;
        if (Line>b.Line) return true;
        return (HeapId<b.HeapId);
      }

      bool operator == (const Entry &b) const
      {
        if (HeapId!=b.HeapId) return false;
        if (Line!=b.Line) return false;
        return !sCmpString(File,b.File);
      }
    };

    const sChar *MyUrl;

    void Link(const sChar *text, sInt mode, sInt memtype, sInt sortby, sInt lines, sInt path, const sChar *filter=0)
    {      
      PrintF(L"<a href=\"%s?mode=%d&type=%d&sort=%d&lines=%d&path=%d",MyUrl,mode, memtype,sortby,lines,path);
      if (filter && filter[0]) 
      {
        sString<512> filterstr;
        sEncodeBase64(filter,2*sGetStringLen(filter)+2,filterstr,sTRUE);
        PrintF(L"&filter=%s",filterstr);
      }
      PrintF(L"\">%s</a>",text);
    }

    sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
    {
      WriteHTMLHeader(L"memory",L"/style.css");

      sMemoryLeakTracker *t=sGetMemoryLeakTracker();

      if (!t)
      {
        PrintF(L"Memory tracking is not available<br>\n\n");

        PrintF(L"MEMORY MAP:<br>\n<code><pre>\n");
        static const sChar *name[sAMF_MASK+1] = 
        {
          L"0:         ",
          L"1: main    ",
          L"2: debug   ",
          L"3: gfx     ",
          L"4: network ",
          L"5: XTRA1   ",
          L"6: XTRA2   ",
          L"7: Audio   ",
          L"8:         ",
          L"9:         ",
          L"10:        ",
          L"11:        ",
          L"12:        ",
          L"13:        ",
          L"14:        ",
          L"15:        ",
        };
        for(sInt i=0;i<sAMF_MASK+1;i++)
        {
          sMemoryHandler *h = sGetMemHandler(i);
          if(h)
            PrintF(L"%s: %08x..%08x   Total %6Kb Free %6Kb Largest %6Kb\n",name[i],h->Start,h->End,(sU64)h->GetSize(),(sU64)h->GetFree(),(sU64)h->GetLargestFree());
        }
        PrintF(L"</pre></code>\n");

        return sHTTPServer::HR_OK;
      }

      sInt mode=GetParamI(L"mode",MODE_LOC);
      sInt memtype=GetParamI(L"type",sAMF_HEAP);
      sInt sortby=GetParamI(L"sort",0);
      sInt lines=GetParamI(L"lines",1);
      sInt path=GetParamI(L"path",0);

      sString<128> filterstr;
      for (sInt i=0; i<128; i++) filterstr[i]=0;
      sDecodeBase64(GetParamS(L"filter",L""),(sChar*)filterstr,2*128,sTRUE);

      MyUrl=URL;

      sBool avail[sAMF_MASK+1];
      avail[0]=sTRUE;
      for (sInt t=1; t<=sAMF_MASK; t++) avail[t]=sIsMemTypeAvailable(t);

      // header/settings
      PrintF(L"<p><table>");

      PrintF(L"<tr><td>Mode:</td><td>");
      for (sInt i=0; i<sCOUNTOF(ModeNames); i++)
      {
        if (i) PrintF(L" | ");
        if (i==mode)
          PrintF(L"<b>%s</b>",ModeNames[i]);
        else
          Link(ModeNames[i],i,memtype,sortby,lines,path);
      }
      PrintF(L"</td></tr>");

      PrintF(L"<tr><td>Mem Type:</td><td>");
      for (sInt i=0; i<16; i++)
        if (avail[i])
        {
          if (i) PrintF(L" | ");
          if (i==memtype)
            PrintF(L"<b>%s</b>",MemTypeNames[i]);
          else
            Link(MemTypeNames[i],mode,i,sortby,lines,path);
        }
      PrintF(L"</td></tr>");

      PrintF(L"<tr><td>Sort by:</td><td>");
      for (sInt i=0; i<sCOUNTOF(MemStatSort[0]); i++)
      {
        if (i) PrintF(L" | ");
        if (i==sortby)
          PrintF(L"<b>%s</b>",MemStatSort[mode][i]);
        else
          Link(MemStatSort[mode][i],mode,memtype,i,lines,path);
      }
      PrintF(L"</td></tr>");

      if (mode==MODE_LOC)
      {

        PrintF(L"<tr><td>Source lines:</td><td>");
        for (sInt i=0; i<2; i++)
        {
          if (i) PrintF(L" | ");
          if (i==lines)
            PrintF(L"<b>%s</b>",MemStatToggle[i]);
          else
            Link(MemStatToggle[i],mode,memtype,sortby,i,path);
        }
        PrintF(L"</td></tr>");

        PrintF(L"<tr><td>Path names:</td><td>");
        for (sInt i=0; i<2; i++)
        {
          if (i) PrintF(L" | ");
          if (i==path)
            PrintF(L"<b>%s</b>",MemStatToggle[i]);
          else
            Link(MemStatToggle[i],mode,memtype,sortby,lines,i);
        }
        PrintF(L"</td></tr>");
      }

      PrintF(L"</table></p>");


      // totals
      PrintF(L"<p><table>");
      for (sInt i=1; i<=sAMF_MASK; i++)
        if ((!memtype || memtype==i) && avail[i])
        {
          sMemoryHandler *h=sGetMemHandler(i);
          h->Lock();
          sCONFIG_SIZET hsize=h->GetSize();
          sCONFIG_SIZET free=h->GetFree();
          sCONFIG_SIZET largest=h->GetLargestFree();
          h->Unlock();
          PrintF(L"<tr><td><b>%s:</b></td><td>free: %K of %K,</td><td>largest block: %K</td></tr>",MemTypeNames[i],(sU64)free,(sU64)hsize,(sU64)largest);
        }
      PrintF(L"</table></p>");

      if (mode==MODE_LOC)
      {

        // gather leaks from tracker and sort by file/line
        sArray<Entry> Entries2;

        {
          sStaticArray<sMemoryLeakInfo> leaks;

          t->GetLeaks(leaks,0);
          sHeapSortUp(leaks); // uses static operator< above

          sMemoryLeakInfo *l;
          sFORALL(leaks,l)
          {
            if (!memtype || (l->HeapId & sAMF_MASK)==memtype)
            {
              Entry e;
              if (l->File)
              {
                sInt slash=-1;
                if (!path)
                  for (sInt i=0; l->File[i]; i++)
                    if (l->File[i]=='/' || l->File[i]=='\\')
                      slash=i;
                sCopyString(e.File,l->File+slash+1);
              }
              else
                e.File=L"(unknown)";
              e.Line=lines?l->Line:0;
              e.Size=l->TotalBytes;
              e.Count=l->Count;
              e.HeapId=l->HeapId;

              sInt c2=Entries2.GetCount()-1;
              if (c2>=0 && e==Entries2[c2])
              {
                Entries2[c2].Size+=e.Size;
                Entries2[c2].Count+=e.Count;
              }
              else
                Entries2.AddTail(e);
            }
          }
        }

        // now sort if necessary
        switch (sortby)
        {
        case 0:
          sHeapSortUp(Entries2);
          break;
        case 1:
          sHeapSortUp(Entries2,&Entry::Size);
          break;
        case 2:
          sHeapSortUp(Entries2,&Entry::Count);
          break;
        }

       
        PrintF(L"<p><table border=1><tr><td>File");
        if (lines) PrintF(L"(Line)");
        PrintF(L"</td><td align=right>Count</td><td align=right>Size (accum)</td>");
        if (!memtype) PrintF(L"<td align=right>Type</td>");
        PrintF(L"</tr>");
        sInt bytes=0;
        sInt count=0;
        Entry *se;
        sFORALL(Entries2,se)
        {
          PrintF(L"<tr><td>%s",se->File);
          if (lines) PrintF(L"(%d)",se->Line);
          PrintF(L"</td><td align=right>%d</td><td align=right>%d</td>",se->Count,se->Size);
          if (!memtype)
          {
            if(se->HeapId&sAMF_ALT)
              PrintF(L"<td align=right>%s[ALT]</td>",MemTypeNames[se->HeapId&sAMF_MASK]);
            else
              PrintF(L"<td align=right>%s</td>",MemTypeNames[se->HeapId&sAMF_MASK]);
          }
          PrintF(L"</tr>");
          bytes+=se->Size;
          count+=se->Count;
        }

        PrintF(L"<tr><td><b>TOTAL</b></td><td align=right><b>%d</b></td><td align=right><b>%d</b></td></tr>",count,bytes);

        PrintF(L"</table></p>");
      }
      else if (mode==MODE_DESC)
      {
        sStaticArray<sMemoryLeakDesc> descs;
        t->GetLeakDescriptions(descs,0);

        sInt totalsize[sAMF_MASK+1];
        sInt totalcount[sAMF_MASK+1];
        sClear(totalsize);
        sClear(totalcount);

        for (sInt i=0; i<descs.GetCount();)
        {
          sMemoryLeakDesc &d=descs[i];
          sBool remove=sFALSE;

          // accumulate total mem/count in unused memtype 0
          d.Count[0]=d.Size[0]=0;
          for (sInt t=1; t<=sAMF_MASK; t++)
          {
            d.Count[0]+=d.Count[t];
            d.Size[0]+=d.Size[t];
          }

          // more struct abuse for sorting (as we're only dealing with copies that we own, it's ok)
          d.Hash=d.Size[memtype];
          d.RefCount=d.Count[memtype];

          if (!d.Hash)
            remove=sTRUE;
          else if (filterstr && sFindString(d.String,filterstr)<0)
            remove=sTRUE;

          if (remove)
            descs.RemAt(i);
          else
          {
            for (sInt t=0; t<sAMF_MASK; t++)
            {
              totalcount[t]+=d.Count[t];
              totalsize[t]+=d.Size[t];
            }
            i++;             
          }
        }

        switch (sortby)
        {
        case 0: sHeapSortUp(descs,&sMemoryLeakDesc::String); break;
        case 1: sHeapSortUp(descs,&sMemoryLeakDesc::Hash); break;
        case 2: sHeapSortUp(descs,&sMemoryLeakDesc::RefCount); break;         
        }

        if (!filterstr.IsEmpty())
        {
          PrintF(L"<p><b>Filtered by '%s'</b> | ",filterstr);
          Link(L"Disable filter",mode,memtype,sortby,lines,path);
          PrintF(L"</p>");
        }

        PrintF(L"<p><table border=1><tr><td>Description</td>");
        if (memtype>0)
          PrintF(L"<td align=right>Count</td><td align=right>Size</td>");
        else
        {
          PrintF(L"<td align=right>Total</td>");
          for (sInt t=1; t<=sAMF_MASK; t++) 
            if (avail[t])
              PrintF(L"<td align=right>%s</td>",MemTypeNames[t]);
        }
        PrintF(L"</tr>");

        sMemoryLeakDesc *d;
        sFORALL(descs,d)
        {
          // cut string and make filter links
          const sChar *str=d->String;
          sChar word[128];
          sInt wl=0;
          PrintF(L"<td>");
          for (;;)
          {
            if (!*str || *str==' ')
            {
              word[wl]=0;
              Link(word,mode,memtype,sortby,lines,path,word);
              PrintF(L" ");
              wl=0;
              while (*str==' ') str++;
              if (!*str) break;
            }
            else
              word[wl++]=*str++;
          };
          PrintF(L"</td>");

          if (memtype>0)
          {
            PrintF(L"<td align=right>%d</td>",d->Count[memtype]);
            PrintF(L"<td align=right>%d</td>",d->Size[memtype]);
          }
          else
            for (sInt t=0; t<=sAMF_MASK; t++) 
              if (avail[t])
              {
                if (d->Size[t])
                  PrintF(L"<td align=right>%d</td>",d->Size[t]);
                else
                  PrintF(L"<td align=right></td>");
              }
          PrintF(L"</tr>");
        }

        PrintF(L"<tr><td><b>TOTAL</b></td>");
        if (memtype>0)
          PrintF(L"<td align=right><b>%d</b></td><td align=right><b>%d</b></td>",totalcount[memtype],totalsize[memtype]);
        else
        {
          for (sInt t=0; t<=sAMF_MASK; t++) 
            if (avail[t])
              PrintF(L"<td align=right><b>%d</b></td>",totalsize[t]);
        }
        PrintF(L"</tr>");

        PrintF(L"</table></p>");

      }

      WriteHTMLFooter();
      return sHTTPServer::HR_OK;
    }

    static Handler *Factory() { return new MemStat; }
  };

  /****************************************************************************/
  /****************************************************************************/

  struct PerfStats : public sHTTPServer::SimpleHandler
  {
    const sChar *MyUrl;

    void Link(const sChar *text, sInt set=-1, sInt value=0)
    {
      if (set>=0)
        PrintF(L"<a href=\"%s?set=%d&value=%d\">%s</a>",MyUrl,set,value,text);
      else
        PrintF(L"<a href=\"%s\">%s</a>",MyUrl,text);
    }

    struct SortEntry
    {
      sInt Index;
      const sChar *Name;

      bool operator> (const SortEntry &b) const { return sCmpStringI(Name,b.Name)>0; }
    };

    sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
    {
      MyUrl=URL;

      WriteHTMLHeader(L"values/switches",L"/style.css");

      const sChar *name;
      const sChar *choice;
      sBool group;
      sInt  value;

      // switches
      {
        sInt index=GetParamI(L"set",-1);
        sPerfIntGetSwitch(index,name,choice,value);
        if (name)
        {
          value=GetParamI(L"value",0);
          sPerfIntSetSwitch(name,value);
        }
      }

      sInt max=-1;
      do 
      {
        max++;
        sPerfIntGetSwitch(max,name,choice,value);
      } while (name);
    
      if (max>0)
      {
        sStaticArray<SortEntry> entries;
        entries.Resize(max);
        for (sInt i=0; i<max; i++)
        {
          entries[i].Index=i;
          sPerfIntGetSwitch(i,entries[i].Name,choice,value);
        }
        sSortUp(entries);

        PrintF(L"<h2>Switches</h2>");
        PrintF(L"<table>");

        for (sInt i=0; i<max; i++)
        {
          sInt index = entries[i].Index;
          sPerfIntGetSwitch(index,name,choice,value);

          PrintF(L"<tr><td><b>%s</b></td><td>",name);

          sInt nc=sCountChoice(choice);
          for (sInt i=0; i<nc; i++)
          {
            sString<128> ch;
            sMakeChoice(ch,choice,i);
            if (i) PrintF(L" | ");
            if (i==value)
              PrintF(L"%s",ch);
            else
              Link(ch,index,i);
          }

          PrintF(L"</td></tr>");
        }

        PrintF(L"</table>");
      }


      // values
      // --------------------
      sInt index=0;
      sPerfIntGetValue(index,name,value,group);

      if (name)
      {
        PrintF(L"<h2>Tracked Values</h2>");
        PrintF(L"<div>"); Link(L"Refresh"); PrintF(L"</div>");
        PrintF(L"<table border=1 rules=cols><tr><th>Name</th><th>Value</th></tr>");
        PrintF(L"<tr><td colspan=2><hr></td></tr>",name,value);

        while (name)
        {
          if (index && group)
            PrintF(L"<tr><td colspan=2><hr></td></tr>",name,value);
          PrintF(L"<tr><td>%s</td><td>%d</td></tr>",name,value);

          ++index;
          sPerfIntGetValue(index,name,value,group);
        }

        PrintF(L"</table>");
      }

      WriteHTMLFooter();
      return sHTTPServer::HR_OK;
    }

    static Handler *Factory() { return new PerfStats; }
  };

  /****************************************************************************/
  /****************************************************************************/

  // read me!
  struct JustAnExample : public sHTTPServer::SimpleHandler
  {

    sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
    {
      WriteHTMLHeader(L"example",L"/style.css");
      PrintF(L"<marquee>Test!</marquee>");
      WriteHTMLFooter();
      return sHTTPServer::HR_OK;
    }

    static Handler *Factory() { return new JustAnExample; }
  };

  /****************************************************************************/
  /****************************************************************************/

  struct Plugin
  {
    sString<64> Name;
    sString<64> URL;
    sHTTPServer::HandlerCreateFunc Factory;

    Plugin() : Factory(0) {}
    Plugin(const Plugin &p) : Name(p.Name), URL(p.URL), Factory(p.Factory) {}
    Plugin(const sChar *name, const sChar *url, sHTTPServer::HandlerCreateFunc fact) : Name(name), URL(url), Factory(fact) {}
  };

  static sArray<Plugin> *Plugins;

  /****************************************************************************/

  class MenuBar : public sHTTPServer::SimpleHandler
  {
  public:

    sHTTPServer::HandlerResult WriteDocument(const sChar *URL)
    {
      WriteHTMLHeader(L"menubar",L"/style.css");

      Print(L"<div id='menubar'><div style='position:relative; overflow:auto;'>\n");

      Print(L"<span style='position:absolute; right:0px;'>");
      sString<128> localhost;
      sGetLocalHostName(localhost);
      PrintF(L"'%s' running on %s",sGetWindowName(),localhost);
      Print(L"</span>\n");

      Print(L"<span>");
      for (sInt i=0; i<Plugins->GetCount(); i++) if (!(*Plugins)[i].Name.IsEmpty())
      {
        if (i) PrintF(L" | ");
        PrintF(L"<a href=\"%s\" target=\"main\">%s</a>",(*Plugins)[i].URL,(*Plugins)[i].Name);
      }
      Print(L"</span>\n");


      Print(L"</div></div>\n");

      WriteHTMLFooter();
      return sHTTPServer::HR_OK;
    }

    static Handler *Factory() { return new MenuBar; }
  };

  /****************************************************************************/
  /****************************************************************************/

  static sInt Port;
  static sThread *Thread=0;
  static sHTTPServer *HTTPD;

  static sTCPClientSocket *LogSocket=0;
  static sString<256> LogServer=L"";

  static void InitLog(const sChar *host); // fwd decl

  void ThreadFunc(sThread *t, void*)
  {
    sPushMemType(sAMF_DEBUG);
    sPushMemLeakDesc(L"HTTPDebug");

    // if the network is currently busy starting up, just wait
    while (sGetNetworkStatus()==sNET_CONNECTING) 
    {
      sSleep(100);
      if (!t->CheckTerminate()) return;
    };

    // if netlog is down and prepared for deferred init, do it now.
    if ( (!LogSocket) && (LogServer.Count()>0) ) 
    { 
      InitLog(LogServer); 
    } 

    if (HTTPD->Init(Port,0))
    {
      sIPAddress localaddr;
      sGetLocalAddress(localaddr);
      sLogF(L"debug", L"starting debug server at %d:%d\n",localaddr,Port);

      if (!HTTPD->Run(t))
        sLogF(L"debug",L"error starting server\n");
    }
    else
      sLogF(L"debug",L"error starting debug server (is port %d occupied?)\n",Port);

    sDelete(HTTPD);

#if sRENDERER!=sRENDER_BLANK
    Screenshot::StaticExit();
#endif
    DebugView::StaticExit();

    sPopMemLeakDesc();
    sPopMemType(sAMF_DEBUG);
  }

  static void Dummy() {}

  static void Init()
  {
    sPushMemType(sAMF_DEBUG);

    Plugins = new sArray<Plugin>;

    Plugins->AddTail(Plugin(L"Debug Out", L"/debug", DebugView::Factory));
    Plugins->AddTail(Plugin(L"Mem Statistics",L"/memstat", MemStat::Factory));
#if sRENDERER!=sRENDER_BLANK
    Plugins->AddTail(Plugin(L"Screenshot", L"/screenshot", Screenshot::Factory));
#endif
    if (sPerfMonInited())
      Plugins->AddTail(Plugin(L"Switches/Values", L"/perf", PerfStats::Factory));

    HTTPD = new sHTTPServer;
    HTTPD->AddStaticPage(L"/",Frameset,sizeof(Frameset));
    HTTPD->AddStaticPage(L"/style.css",Stylesheet,sizeof(Stylesheet)-1); // omit the trailing zero byte
    HTTPD->AddHandler(L"/menubar",MenuBar::Factory);

    DebugView::StaticInit();
#if sRENDERER!=sRENDER_BLANK
    Screenshot::StaticInit();
#endif

    for (sInt i=0; i<Plugins->GetCount(); i++)
      HTTPD->AddHandler((*Plugins)[i].URL,(*Plugins)[i].Factory);


    Thread=new sThread(ThreadFunc,1,65536*4);
    sPopMemType(sAMF_DEBUG);
  }

  static void Exit()
  {
    sDelete(Thread);
    
    sDelete(Plugins);
  }

  static void NetLog(const sChar *txt,void *user=0)
  {
    sChar8 t8[1024];
    sInt len;
    while (*txt)
    {
      len=0;
      sInt lc=0;
      while (*txt && len<1023)
      {
        sInt c=*txt++;
        if (c=='\n' && lc!='\r') t8[len++]='\r';
        t8[len++]=c;
        lc=c;
      }
      if (len) 
        if (!LogSocket->WriteAll(t8,len))
        {
          sDebugOutHook->Rem(NetLog);
          sDelete(LogSocket);
          sLogF(L"debug",L"connection to log server lost!\n");
          return;
        }
    }
  }

  static void InitLogDeferred(const sChar *host)
  {
    sCopyString(LogServer,host);
  }

  static void InitLog(const sChar *host)
  {
    sIPAddress addr;
    sIPPort port=49149;
    if (!sResolveNameAndPort(host,addr,port))
    {
      sLogF(L"debug",L"could not resolve log server '%s'\n",host);
      return;
    }

    LogSocket = new sTCPClientSocket;
    if (!LogSocket->Connect(addr,port))
    {
      sDelete(LogSocket);
      sLogF(L"debug",L"could not connect to log server %d:%d\n",addr,port);
      return;
    }

    sDebugOutHook->Add(NetLog);

    sString<256> msg;
    sString<256> app;
    sString<256> lhost;
    sGetLocalHostName(lhost);
    app=sGetWindowName();
    sInt fc;
    while ((fc=sFindFirstChar(app,' '))>=0) app[fc]='_';
    msg.PrintF(L"#49/APP[%s]APP/49#\n#49/HST[%s]HST/49#\nThis is '%s' running on '%d'\n----------\n",app,lhost,sGetWindowName(),lhost);
    NetLog(msg);
  }

  static void ExitLog()
  {
    if (LogSocket)
    {
      NetLog(L"\n----------\nGoodbye.\n");
      sDebugOutHook->Rem(NetLog);
      LogSocket->Disconnect();
      sDelete(LogSocket);
    }
  }

};


/****************************************************************************/
/****************************************************************************/

void sAddDebugServer(sInt port)
{
  if (!sIsMemTypeAvailable(sAMF_DEBUG))
  {
    sLogF(L"debug",L"server disabled (no memory)\n");
    return;
  }

  sAddSubsystem(L"DebugServer",0x7e,sNetDebug::Dummy,sNetDebug::Exit);
  sNetDebug::Port=port;
  sNetDebug::Init();
}

/****************************************************************************/


void sAddNetDebugLog(const sChar *host)
{


  sAddSubsystem(L"DebugLog",0x16,sNetDebug::Dummy,sNetDebug::ExitLog);
  sNetDebug::InitLogDeferred(host);

}


/****************************************************************************/

sBool sAddNetDebugPlugin(sHTTPServer::HandlerCreateFunc factory,const sChar *name, const sChar *path)
{
  if (!sNetDebug::HTTPD)
  {
    sLogF(L"debug",L"Tried to add debug server plug-in without a debug server, ignoring\n");
    return 0;
  }
  if ((!name || !name[0]) && (!path || !path[0]))
  {
    sLogF(L"debug",L"Error: debug server plug-ins need a name or an explicit path\n");
    return 0;
  }

  sPushMemType(sAMF_DEBUG);

  sString<64> fakepath;
  if (!path || !path[0])
  {
    sSPrintF(fakepath,L"/user%d",sNetDebug::Plugins->GetCount());
    path=fakepath;
  }

  sNetDebug::Plugin p;
  p.Factory=factory;
  if (path[0]=='/')
    p.URL=path;
  else
    p.URL.PrintF(L"/%s",path);

  p.Name=name?name:L"";

  sNetDebug::Plugins->AddTail(p);
  sNetDebug::HTTPD->AddHandler(p.URL, factory);

  sPopMemType(sAMF_DEBUG);

  return 1;
}

sBool sAddNetDebugFile(const sChar *path, const sU8* buffer, sDInt size)
{
  if (!sNetDebug::HTTPD)
  {
    sLogF(L"debug",L"Tried to add debug server file without a debug server, ignoring\n");
    return 0;
  }
  if (!path || !path[0])
  {
    sLogF(L"debug",L"Error: debug server files need an explicit path\n");
    return 0;
  }

  sPushMemType(sAMF_DEBUG);

  if (path[0]=='/')
    sNetDebug::HTTPD->AddStaticPage(path,buffer,size);
  else
  {
    sString<64> fakepath;
    fakepath.PrintF(L"/%s",path);
    sNetDebug::HTTPD->AddStaticPage(fakepath,buffer,size);
  }
    
  sPopMemType(sAMF_DEBUG);

  return 1;
}

#endif // sNETDEBUG_ENABLE
