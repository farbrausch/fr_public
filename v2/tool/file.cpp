#include "file.h"

sS32 file::copy(file &src, sS32 cnt)
{
	sS32 copied=0;
	sS32 maxlen=src.size()-src.tell();
	if (cnt>0 && cnt<maxlen)
		maxlen=cnt;

	sU8 *buf=new sU8[65536];

	while (maxlen)
	{
		sS32 bsize=(maxlen<65536)?maxlen:65536;
		sS32 bread=src.read(buf, bsize);
		sS32 bwritten=write(buf, bread);
		maxlen-=bread;
		copied+=bwritten;

		if (!bread || bwritten!=bread)
			break;
	}
  
  delete[] buf;

	return copied;
}

// --------------------------------------------------------------------------- sfile: standard file

sBool fileS::open(const sChar *name, sInt mode)
{
  DWORD      acc, crm;

  if (hnd!=INVALID_HANDLE_VALUE)
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

  hnd=CreateFile(name, acc, FILE_SHARE_READ, 0, crm, 0, 0);

  return (hnd!=INVALID_HANDLE_VALUE)?sTRUE:sFALSE;
}

sS32 fileS::read(void *buf, sS32 cnt)
{
  DWORD bread;
  
  if (hnd==INVALID_HANDLE_VALUE || !ReadFile(hnd, buf, cnt, &bread, 0))
	{
		sU32 err=GetLastError();
    bread=0;
	}

  return bread;
}

sS32 fileS::write(const void *buf, sS32 cnt)
{
  DWORD bwr;

  if (hnd==INVALID_HANDLE_VALUE || !WriteFile(hnd, buf, cnt, &bwr, 0))
    bwr=0;

  return bwr;
}
