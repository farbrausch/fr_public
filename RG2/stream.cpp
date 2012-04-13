// This code is in the public domain. See LICENSE for details.

#include "stream.h"
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fr
{
	sS32 stream::copy(stream &src, sS32 cnt)
	{
	  sS32  copied=0;
	  sBool readOk=sTRUE;
	  sS32  maxlen=-1;

		if (cnt>0 && cnt<maxlen)
			maxlen=cnt;

		sU8 *buf=new sU8[65536];

	  while (readOk)
		{
	    sS32 bsize;
	    if (maxlen!=-1)
	      bsize=(maxlen<65536)?maxlen:65536;
	    else
	      bsize=65536;

			sS32 bread=src.read(buf, bsize);
			sS32 bwritten=write(buf, bread);

	    if (maxlen!=-1)
	    {
	      maxlen-=bread;
	      if (maxlen<=0)
	      {
	        src.seekcur(-maxlen);
	        readOk=sTRUE;
	      }
	    }

			copied+=bwritten;

			if (!bread || bwritten!=bread)
				break;
		}
	  
	  delete[] buf;

		return copied;
	}

	// --------------------------------------------------------------------------

	struct streamF::privData
	{
		HANDLE hnd;
	};

	streamF::streamF()
	{
		data=new privData;
		data->hnd=INVALID_HANDLE_VALUE;
	}

	streamF::streamF(const sChar *name, sInt mode)
	{
		data=new privData;
		data->hnd=INVALID_HANDLE_VALUE;
		open(name, mode);
	}

	streamF::~streamF()
	{
		close();
		delete data;
	}

	sBool streamF::open(const sChar *name, sInt mode)
	{
	  DWORD      acc, crm;

	  close();

	  switch (mode & 3)
	  {
	  case rd: acc=GENERIC_READ; break;
	  case wr: acc=GENERIC_WRITE; break;
	  case rw: acc=GENERIC_READ | GENERIC_WRITE; break;
	  }

	  switch (mode & 12)
	  {
	  case ex: crm=OPEN_EXISTING; break;
	  case cr: crm=CREATE_ALWAYS; break;
	  case up: crm=OPEN_ALWAYS; break;
	  }

	  data->hnd=CreateFile(name, acc, FILE_SHARE_READ, 0, crm, 0, 0);

	  return (data->hnd!=INVALID_HANDLE_VALUE)?sTRUE:sFALSE;
	}

	void streamF::close()
	{
    if (data->hnd!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(data->hnd);
			data->hnd=INVALID_HANDLE_VALUE;
		}
	}

	sS32 streamF::read(void *buf, sS32 cnt)
	{
	  DWORD bread;
	  
	  if (!data->hnd || !ReadFile(data->hnd, buf, cnt, &bread, 0))
	    bread=0;

	  return bread;
	}

	sS32 streamF::seek(sS32 pos)
	{
		return SetFilePointer(data->hnd, pos, 0, FILE_BEGIN);
	}

	sS32 streamF::seekcur(sS32 pos)
	{
		return SetFilePointer(data->hnd, pos, 0, FILE_CURRENT);
	}

	sS32 streamF::seekend(sS32 pos)
	{
		return SetFilePointer(data->hnd, pos, 0, FILE_END);
	}

	sBool streamF::flush()
	{
		return !!FlushFileBuffers(data->hnd);
	}

	sS32 streamF::tell()
	{
		return SetFilePointer(data->hnd, 0, 0, FILE_CURRENT);
	}

	sS32 streamF::size()
	{
		return GetFileSize(data->hnd, 0);
	}

	sS32 streamF::write(const void *buf, sS32 cnt)
	{
	  DWORD bwr;

	  if (!data->hnd || !WriteFile(data->hnd, buf, cnt, &bwr, 0))
	    bwr=0;

	  return bwr;
	}

	// ---------------------------------------------------------------------------

	sBool streamM::open(void *buf, sS32 len, sBool deleteMemOnClose)
	{
	  close();

	  fbuf=(sU8 *)buf;
	  flen=len;
	  dwc=deleteMemOnClose;

	  return (fbuf && flen) ? sTRUE : sFALSE;
	}

	sBool streamM::open(sInt resID)
	{
	  HINSTANCE inst;
	  HRSRC     res;
	  HGLOBAL   rptr;

	  inst=GetModuleHandle(0);
	  if (!(res=FindResource(inst, MAKEINTRESOURCE(resID), RT_RCDATA)))
	    return sFALSE;

	  if (!(rptr=LoadResource(inst, res)))
	    return sFALSE;

	  readonly=sTRUE;
	 
	  return open(LockResource(rptr), SizeofResource(inst, res));
	}

	sBool streamM::open(stream &f, sS32 len)
	{
	  sS32 maxlen=f.size()-f.tell();
	  if (len>=0 && len<maxlen)
	    maxlen=len;

	  if (!openNew(maxlen))
	    return sFALSE;

	  flen=f.read(fbuf,flen);
	  return sTRUE;
	}

	sBool streamM::openNew(sS32 len)
	{
	  close();

	  if (len)
	  {
	    fbuf=new sU8[len];

	    if (fbuf)
	    {
	      flen=len;
	      dwc=sTRUE;
	    }
	  }

	  return (fbuf && flen) ? sTRUE : sFALSE;
	}

	void *streamM::detach()
	{
	  if (!dwc || !fbuf)
	    return 0;

	  void *r=fbuf;
	  fbuf=0;
	  flen=0;
	  fpos=0;

	  return r;
	}

	void streamM::close()
	{
	  if (fbuf && dwc)
	    delete[] fbuf;
	  
	  fbuf=0;
	  flen=0;
	  fpos=0;
	}

	sS32 streamM::read(void *buf, sS32 cnt)
	{
	  if (!buf || !fbuf)
	    return 0;

	  if (cnt<0) 
	    cnt=0;

	  if (cnt>(flen-fpos))
	    cnt=flen-fpos;

	  memcpy(buf, fbuf+fpos, cnt);
	  fpos+=cnt;

	  return cnt;
	}

	sS32 streamM::write(const void *buf, sS32 cnt)
	{
	  if (!buf || !fbuf || readonly)
	    return 0;

	  if (cnt<0)
	    cnt=0;

	  if (cnt>(flen-fpos))
	    cnt=flen-fpos;

	  memcpy(fbuf+fpos, buf, cnt);
	  fpos+=cnt;
	  return cnt;
	}

	sS32 streamM::seek(sS32 pos)
	{
	  if (pos<0)
	    pos=0;

	  if (pos>flen)
	    pos=flen;

	  return fpos=pos;
	}

	// ---------------------------------------------------------------------------

	sBool streamA::open(stream &f, sS32 pos, sS32 len, sBool deleteHostOnClose)
	{
	  sS32 fsize=f.size();
	  fhost=&f;

	  pos=(pos<0)?0:(pos>fsize)?fsize:pos;
	  if (len<0)
	    len=0;

	  if ((pos+len)>fsize)
	    len=fsize-pos;

	  f.seek(pos);
	  foffset=pos;

	  flen=len;
	  dhc=deleteHostOnClose;
	  fpos=0;

	  return (flen) ? sTRUE : sFALSE;
	}

	void streamA::close()
	{
	  if (fhost && dhc)
	  {
	    fhost->close();
	    delete fhost;
	  }

	  fhost=0;
	  foffset=0;
	  flen=0;
	  fpos=0;
	}

	sS32 streamA::read(void *buf, sS32 cnt)
	{
	  if (!buf || !fhost)
	    return 0;

	  if (cnt<0)
	    cnt=0;
	  if (cnt>(flen-fpos))
	    cnt=flen-fpos;

	  cnt=fhost->read(buf,cnt);
	  fpos+=cnt;

	  return cnt;
	}

	sS32 streamA::write(const void *buf, sS32 cnt)
	{
	  if (!buf || !fhost)
	    return 0;

	  if (cnt<0)
	    cnt=0;

	  if (cnt>(flen-fpos))
	    cnt=flen-fpos;

	  cnt=fhost->write(buf,cnt);
	  fpos+=cnt;

	  return cnt;
	}

	sS32 streamA::seek(sS32 pos)
	{
	  if (pos<0)
	    pos=0;

	  if (pos>flen)
	    pos=flen;

	  fhost->seek(foffset+pos);

	  return fpos=fhost->tell()-foffset;
	}

	sS32 streamA::seekcur(sS32 pos)
	{
	  fhost->seekcur(pos);
	  return fpos=fhost->tell()-foffset;
	}

	// ---------------------------------------------------------------------------

	sBool streamMTmp::open()
	{
	  close();
	  firstblk=new tmblock;
	  seek(0);

	  return firstblk ? sTRUE : sFALSE;
	}

	void streamMTmp::close()
	{
	  if (firstblk)
	    delete firstblk;

	  firstblk=0;
	  curblk=0;
	  flen=0;
	  fpos=0;
	  bpos=0;
	}

	sS32 streamMTmp::read(void *buf, sS32 cnt)
	{
	  if (!buf || !curblk)
	    return 0;

	  if (cnt>(flen-fpos))
	    cnt=flen-fpos;

	  sU8   *out=(sU8 *) buf;
	  sS32  rb=0;

	  while (cnt)
	  {
	    // noch im selben block?
	    if (bpos+cnt<BLOCKSIZE)
	    {
	      memcpy(out, curblk->content+bpos, cnt);
	      out+=cnt;
	      bpos+=cnt;
	      rb+=cnt;
	      cnt=0;
	    }
	    else
	    {
	      // rest vom block übertragen
	      sS32 tocopy=BLOCKSIZE-bpos;
	      memcpy(out, curblk->content+bpos, tocopy);

	      out+=tocopy;
	      rb+=tocopy;
	      cnt-=tocopy;
	      bpos=0;
	      curblk=curblk->next;

	      if (!curblk)
	        cnt=0;
	    }
	  }

	  fpos+=rb;
	  return rb;
	}

	sS32 streamMTmp::write(const void *buf, sS32 cnt)
	{
	  if (!buf || !curblk)
	    return 0;

	  if (cnt<0) 
	    cnt=0;

	  sU8   *in=(sU8 *) buf;
	  sS32  wb=0;

	  while (cnt)
	  {
	    // noch im selben block?
	    if (bpos+cnt<BLOCKSIZE)
	    {
	      memcpy(curblk->content+bpos, in, cnt);
	      in+=cnt;
	      bpos+=cnt;
	      wb+=cnt;
	      cnt=0;
	    }
	    else
	    {
	      // rest vom block übertragen
	      sS32 tocopy=BLOCKSIZE-bpos;
	      memcpy(curblk->content+bpos, in, tocopy);
	      in+=tocopy;
	      wb+=tocopy;
	      cnt-=tocopy;
	      bpos=0;
	      // nexter block (oder nen neuer)
	      if (!curblk->next)
	        curblk->next=new tmblock;

	      curblk=curblk->next;
	      if (!curblk)
	        cnt=0;
	    }
	  }

	  fpos+=wb;
	  if (fpos>flen)
	    flen=fpos;

	  return wb;
	}

	sS32 streamMTmp::seek(sS32 pos)
	{
	  pos=(pos<0)?0:(pos>flen)?flen:pos;

	  sS32 pos2=pos;
	  curblk=firstblk;

	  while (pos2>=BLOCKSIZE && curblk)
	  {
	    curblk=curblk->next;
	    pos2-=BLOCKSIZE;
	  }

	  bpos=pos2;

	  return fpos=pos;
	}
}
