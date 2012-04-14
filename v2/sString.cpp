#include "PlatformAPI.h"
#include "types.h"
#include "debug.h"
#include "tool.h"
#include "Stream.h"
#include "sString.h"



sWString & LIBAPI __ltrim16 (sWString &arg)
////////////////////////////////////
{
	sU32 pos=0;
	while (arg.is_ws(arg.m_data[pos])) pos++;
	if (pos)
	{
		arg.int_cpy(arg.m_data,arg.m_data+pos,arg.m_size-pos+1);
		arg.m_size-=pos;
	}
	return arg;
}


sWString & LIBAPI __rtrim16 (sWString &arg)
////////////////////////////////////////////
{
	while (arg.m_size && arg.is_ws(arg.m_data[arg.m_size-1])) 
	{
		arg.m_size--;
		arg.int_null();
	}
	return arg;
}



int LIBAPI __format16 (sWString &arg, const sWChar* format, va_list arglist)
////////////////////////////////////////////////////////////////////////////
{
  // try to write directly into the buffer (without realloc) 
  if (arg.m_allocsize)
  {
	  int n = _vsnwprintf(arg.m_data, arg.m_allocsize, format, arglist);
    if (n>0)
    {
      arg.m_data[arg.m_allocsize-1]=0;
      arg.m_size = n;
      return n;
    }
  }

  sU32 MaxSize = 8192;
  sInt n;
  do 
  {
    sWChar * temp = new sWChar[MaxSize];
	  n = _vsnwprintf(temp, MaxSize-1, format, arglist);
    temp[MaxSize-1]=0;

    if (n>=0)
    {
      arg = temp;
      delete [] temp;
      return n;
    }
    else MaxSize += 8192;
    DASSERT (MaxSize < 65536) // well, 64k of text is very unlikely
    delete [] temp;
  } while (1);
}


int LIBAPI __format8 (sString &arg, const sChar * format, va_list arglist)
////////////////////////////////////////////////////////////////////////////
{
  // try to write directly into the buffer (without realloc) 
  if (arg.m_allocsize)
  {
	  int n = _vsnprintf(arg.m_data, arg.m_allocsize, format, arglist);
    if (n>0)
    {
      arg.m_data[arg.m_allocsize-1]=0;
      arg.m_size = n;
      return n;
    }
  }

  sU32 MaxSize = 8192;
  sInt n;
  do 
  {
    char * temp = new char[MaxSize];
	  n = _vsnprintf(temp, MaxSize-1, format, arglist);
    temp[MaxSize-1]=0;

    if (n>=0)
    {
      arg = temp;
      delete [] temp;
      return n;
    }
    else MaxSize += 8192;
    DASSERT (MaxSize < 65536) // well, 64k of text is very unlikely
    delete [] temp;
  } while (1);
}
