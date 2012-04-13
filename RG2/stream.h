// This code is in the public domain. See LICENSE for details.

#ifndef __fr_stream_h_
#define __fr_stream_h_

#include "types.h"

#define GETPUTMETHODS(T) virtual T get##T##() { T t; read(&t, sizeof(T)); return t; } \
						 virtual sS32 put##T##(T t) { return write(&t, sizeof(T)); }

#define GETPUTFUNCS(T)  static inline T get##T##(stream &f) { return f.get##T##(); }; \
            static inline sS32 put##T##(stream &f, T t) { return f.put##T##(t); };

namespace fr
{
	class FR_NOVTABLE stream
	{
	public:
	  stream()          { }
	  virtual ~stream() { }

	  virtual void      close()=0;

	  virtual sS32      read(void *buf, sS32 cnt)=0;
	  virtual sS32      write(const void *buf, sS32 cnt)=0;

	  virtual sS32      seek(sS32 pos=0)=0;
	  virtual sS32      seekcur(sS32 pos)   { return seek(tell()+pos); };
	  virtual sS32      seekend(sS32 pos=0) { return seek(size()+pos); };

	  virtual sBool     flush()             { return sTRUE; }

	  virtual sS32      tell()=0;
	  virtual sS32      size()=0;

	  virtual stream&   operator [](sS32 pos) { seek(pos); return *this; }
	  virtual sBool     eof()                 { return tell()==size(); }

	  virtual sBool     eread(void *buf, sS32 cnt)        { return read(buf, cnt)==cnt; }
	  virtual sBool     ewrite(const void *buf, sS32 cnt) { return write(buf, cnt)==cnt; }

	  virtual sS32      copy(stream &src, sS32 cnt=-1);

		template<class T> sS32 read(T& buf)   { return read(&buf,sizeof(T)); }
		template<class T> sS32 write(T& buf)  { return write(&buf,sizeof(T)); }

	  GETPUTMETHODS(sInt)
    GETPUTMETHODS(sUInt)
	  GETPUTMETHODS(sBool)
	  GETPUTMETHODS(sChar)
	  GETPUTMETHODS(sS8)
	  GETPUTMETHODS(sS16)
	  GETPUTMETHODS(sS32)
	  GETPUTMETHODS(sS64)
	  GETPUTMETHODS(sU8)
	  GETPUTMETHODS(sU16)
	  GETPUTMETHODS(sU32)
	  GETPUTMETHODS(sU64)
	  GETPUTMETHODS(sF32)
	  GETPUTMETHODS(sF64)

	  virtual void gets(sChar *buf, sInt max, sChar end)
	  {
	    sInt  i=0, ch;

	    while (1)
	    {
	      ch=getsChar();
	      *buf=0;
	      if (ch==end)
	        break;

	      if (max==1)
	      {
	        sU8 bf;
	        while (eread(&bf, 1));
	        break;
	      }

	      max--;
	      *buf++=ch;
	    }
	  }

	  virtual sS32 puts(const sChar *buf)
	  {
			sInt slen=0;
			if (buf)
			{
				while (buf[slen])
					slen++;
			}

	    return buf?write(buf, slen):0;
	  }
	};

	class streamF: public stream
	{
	private:
		struct privData;

		privData	*data;

	public:
	  streamF();
	  streamF(const sChar *name, sInt mode=rd|ex);
		~streamF();

		enum {
		  rd=0, wr=1, rw=2,
		  ex=0, cr=4, up=8
		};

		virtual sBool     open(const sChar *name, sInt mode=rd|ex);
		virtual sS32      read(void *buf, sS32 cnt);
	  virtual sS32      write(const void *buf, sS32 cnt);

		virtual void      close();

		virtual sS32      seek(sS32 pos);
		virtual sS32      seekcur(sS32 pos);
		virtual sS32      seekend(sS32 pos);

	  virtual sBool     flush();

		virtual sS32      tell();
		virtual sS32      size();			      

		template<class T> sS32 read(T& buf)   { return read(&buf, sizeof(T)); }
		template<class T> sS32 write(T& buf)  { return write(&buf, sizeof(T)); }
	};

	class streamM: public stream
	{
	protected:
		sU8   *fbuf;
		sS32  fpos, flen;
		sBool dwc;
		sBool readonly;

	public:
	  streamM()
		{
			fbuf=0;
			flen=0;
			fpos=0;
			dwc=sFALSE;
			readonly=sFALSE;
		}

	  ~streamM()
	  {
	    close();
	  }

	  virtual sBool open(void *buf, sS32 len, sBool deleteMemOnClose=sFALSE);
	  virtual sBool open(sInt resID);
	  virtual sBool open(stream &f, sS32 len=-1);
	  virtual sBool openNew(sS32 len);

	  virtual void *detach();
	  virtual void close();

	  virtual sS32 read(void *buf, sS32 cnt);
	  virtual sS32 write(const void *buf, sS32 cnt);

	  virtual sS32 seek(sS32 pos);

		virtual sS32 tell() { return fpos; }
		virtual sS32 size() { return flen; }

		template<class T> sS32 read(T& buf)   { return read(&buf,sizeof(T)); }
		template<class T> sS32 write(T& buf)  { return write(&buf,sizeof(T)); }
	};


	class streamA: public stream
	{
	protected:
	  stream  *fhost;
	  sS32    foffset, flen, fpos;
	  sBool   dhc;

	public:
	  streamA()
		{
		  fhost=0;
		  foffset=0;
		  flen=0;
		  fpos=0;
		  dhc=0;
		}

	  ~streamA()
	  {
	    close();
	  }

	  virtual sBool open(stream &f, sS32 pos, sS32 len, sBool deleteHostOnClose=sFALSE);
	  virtual void close();

	  virtual sS32 read(void *buf, sS32 cnt);
	  virtual sS32 write(const void *buf, sS32 cnt);

	  virtual sS32 seek(sS32 pos);
	  virtual sS32 seekcur(sS32 pos);

		virtual sS32 tell() { return fpos; }
		virtual sS32 size() { return flen; }

		template<class T> sS32 read(T& buf)   { return read(&buf,sizeof(T)); }
		template<class T> sS32 write(T& buf)  { return write(&buf,sizeof(T)); }
	};

	class streamMTmp: public stream
	{
	public:
	  enum { BLOCKSIZE=65536 };

	protected:
		struct tmblock 
	  {
			tmblock()
	    {
	      next=0; 
	    }

			~tmblock() 
			{ 
				if (next)
					delete next;
			}

			tmblock *next;
	    sU8     content[BLOCKSIZE];
		};

		tmblock *firstblk, *curblk;
		sS32    bpos, fpos, flen;

	public:
	  streamMTmp()
		{
			firstblk=0;
		  curblk=0;
			flen=0;
			fpos=0;
			bpos=0;
		}

	  ~streamMTmp()
	  {
	    close();
	  }

	  virtual sBool open();
	  virtual void close();

	  virtual sS32 read(void *buf, sS32 cnt);
	  virtual sS32 write(const void *buf, sS32 cnt);

	  virtual sS32 seek(sS32 pos);

		virtual sS32 tell() { return fpos; }
		virtual sS32 size() { return flen; }

		template<class T> sS32 read(T& buf)   { return read(&buf, sizeof(T)); }
		template<class T> sS32 write(T& buf)  { return write(&buf, sizeof(T)); }
	};

	GETPUTFUNCS(sBool)
	GETPUTFUNCS(sInt)
  GETPUTFUNCS(sUInt)
	GETPUTFUNCS(sChar)
	GETPUTFUNCS(sS8)
	GETPUTFUNCS(sS16)
	GETPUTFUNCS(sS32)
	GETPUTFUNCS(sS64)
	GETPUTFUNCS(sU8)
	GETPUTFUNCS(sU16)
	GETPUTFUNCS(sU32)
	GETPUTFUNCS(sU64)
	GETPUTFUNCS(sF32)
	GETPUTFUNCS(sF64)
}

#endif
