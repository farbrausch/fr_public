#ifndef _STRING_H_
#define _STRING_H_

// string class

// CT : type of character (eg. sChar)
// t_alloc_headroom : headroom for allocations (eg. 128 chars)
// t_compat : flag if string ends with 0 character (compatibility to C-Style strings)

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#pragma warning (push)
#pragma warning (disable: 4127)


#ifdef memcpy
#pragma intrinsic (memcpy)
	inline void sMemCpy(CP d, CCP s, sU32 l)	{ memcpy(d,s,l); }
#else
	inline void sMemCpy(void *d, const void *s, sU32 l)	{ for (sU32 i=0; i<l; i++) ((sU8*)d)[i]=((sU8*)s)[i]; }
#endif


template <class CT, sU32 t_alloc_headroom, sBool t_compat> class sStringT
{

protected:
	typedef const CT CCT;
	typedef CT * CP;
	typedef const CT * CCP;
	typedef sStringT<CT, t_alloc_headroom, t_compat> ST;
	typedef const sStringT<CT, t_alloc_headroom, t_compat> CST;

	CP   m_data;
	sU32 m_size;
	sU32 m_allocsize;

	// own strlen for C-style strings
	static inline sU32 int_strlen(CCP s)						{  if (!s) return 0; sU32 l; for (l=0; *s; s++, l++) {} return l; }
	static inline void int_cpy(CP d, CCP s, sU32 l)	{  sMemCpy(d,s,l*sizeof(CT)); }
	inline void int_null()										{ if (t_compat) m_data[m_size]=0; }
	inline void int_alloc(sU32 l)							
  {
    m_allocsize=l+t_alloc_headroom; 
    m_data=new CT[m_allocsize+(t_compat?1:0)];
  }
	inline void int_make(sU32 l)						  { delete[] m_data; m_data=0; int_alloc(m_size=l); int_null(); }
	inline void int_expand(sU32 l)           
	{
		if (m_size+l > m_allocsize)
		{
			CP olddata=m_data;
			int_alloc(m_size+l);
			int_cpy(m_data,olddata,m_size+1);
			delete[] olddata;
		}
	}

	// internal constructor for prealloc
	explicit inline sStringT(sU32 prealloc) : m_size(0) { int_alloc(prealloc); int_null(); }

// -----------------------------------------------------------------------------

public:

	// default constructor
	inline sStringT() : m_size(0), m_data(0) { int_make(0); }


	// constructor from C-style string
	inline sStringT(CCP s) : m_data(0)
	{ 
		int_make(int_strlen(s));
		int_cpy(m_data,s,m_size);
	}

  inline sStringT (CCP first, CCP last) : m_data(0)
  {
    int_make(last-first+1);
		int_cpy(m_data,first,last-first+1);
  }

	// constructor from other string
	inline sStringT(CST &s) : m_data(0)
	{
		int_make(s.m_size);
		int_cpy(m_data,s.m_data,m_size);
	}

	// constructor from raw memory block
	inline sStringT(const void *s, sU32 len, sU32 allocsize=0) : m_data(0)
	{
		if (allocsize)
		{
			m_data=reinterpret_cast<CCP>(s);
			m_size=len;
			m_allocsize=allocsize-1;
		}
		else
			int_make(len);
		int_cpy(m_data,reinterpret_cast<CCP>(s),len);
	}

	// destructor
	inline ~sStringT() {
    delete[] m_data; 
  }

// -----------------------------------------------------------------------------

	// assignments
	inline ST & operator = (CCP s)
	{
		const sU32 l=int_strlen(s);
		if (l<=m_allocsize)
		{
			m_size=l;
			int_null();
		}
		else
		{
			int_make(l);
		}
		int_cpy(m_data,s,m_size);
		return *this;
	}

	// constructor from other string
	inline ST & operator = (CST &s)
	{
		if (s.m_size<=m_allocsize)
		{
			m_size=s.m_size;
			int_null();
		}
		else
		{
			int_make(s.m_size);
		}
		int_cpy(m_data,s.m_data,m_size);
		return *this;
	}


	// to c-style string
	inline CP strdup() const
	{
		CP p=new CT[m_size+1];
		int_cpy(p,m_data,m_size+1);
		return p;
	}

	// to c-style string, clear self
	inline CP detach()
	{
		CP p=strdup();
		int_make(0);
		return p;
	}

	// USE WITH CAUTION!!!
	inline CCP data() const { 
		CTASSERT(t_compat);
		return m_data; 
	}

// -----------------------------------------------------------------------------

	// character access
	inline CCT & operator [] (sU32 index) const
	{
		return m_data[index];
	}

	inline CT & operator [] (sU32 index)
	{
		return m_data[index];
	}

	// string access
	ST substr(sU32 start, sU32 len) const
	{
		ST tmp(len);
		int_cpy(tmp.m_data,m_data+start,len);
		tmp.m_size=len;
		tmp.int_null();
		return tmp;
	}

	inline ST leftstr(sU32 len) const  { return substr(0,len); }
	inline ST rightstr(sU32 len) const { return substr(m_size-len,len); }

// -----------------------------------------------------------------------------

	// properties
	inline sU32 size() const { return m_size; }

// -----------------------------------------------------------------------------
 
	// concatenation
	ST & concat(CST &s)
	{
		int_expand(s.m_size);
		int_cpy(m_data+m_size,s.m_data,s.m_size);
		m_size+=s.m_size;
		int_null();
		return *this;
	}

	inline ST & operator += (CST &s) { return concat(s); }


	ST & concat(CCP s)
	{
		const sU32 l=int_strlen(s);
		int_expand(l);
		int_cpy(m_data+m_size,s,l);
		m_size+=l;
		int_null();
		return *this;
	}

	ST & operator += (CCP &s) { return concat(s); }

	ST & concat(CCT c)
	{
		int_expand(1);
		m_data[m_size++]=c;
		int_null();
		return *this;
	}

	inline ST & operator += (CCT c) { return concat(c); }
	inline ST operator + (CST &s) const { ST tmp(m_size+s.m_size); tmp=*this; tmp+=s; return tmp; }
	inline ST operator + (CCP &s) const { ST tmp(m_size+int_strlen(s)); tmp=*this; tmp+=s; return tmp; }
	inline ST operator + (CCT c)  const { ST tmp(m_size+1); tmp=*this; tmp+=c; return tmp; }

// -----------------------------------------------------------------------------

	// search from left: chars
  sU32 seek(CCT c, sU32 startindex=0) const
	{
		while (startindex<m_size)
			if (m_data[startindex]==c)
				return startindex;
			else startindex++;

		return m_size;
	}

	// search from left: sStrings
	sU32 seek(CST &s, sU32 startindex=0) const
	{
		while ((startindex+s.m_size)<=m_size)
		{
			sU32 match=0;
			while (match<s.m_size && m_data[startindex+match]==s.m_data[match]) match++;
			if (match==s.m_size) return startindex; else startindex++;
		}
		return m_size;
	}

	// search from left: C-Style strings
	sU32 seek(CCP s, sU32 startindex=0) const
	{
		const sU32 l=int_strlen(s);
		while ((startindex+l)<=m_size)
		{
			sU32 match=0;
			while (match<l && m_data[startindex+match]==s[match]) match++;
			if (match==l) return startindex; else startindex++;
		}
		return m_size;
	}


	// search from right: chars
	sU32 seekr(CCT c, sU32 startindex=0) const
	{
		if (startindex>=m_size) return m_size;
		startindex=m_size-startindex-1;
		while (startindex>0)
			if (m_data[startindex]==c)
				return startindex;
			else startindex--;
		return *m_data==c?0:m_size;
	}


	// search from right: C-Style strings
	sU32 seekr(CCP s, sU32 startindex=0) const
	{
		if (startindex>=m_size) return m_size;
		sS32 si=m_size-startindex-1;
		const sS32 l=int_strlen(s);
		while ((si-l)>=0)
		{
			sS32 match=0;
			while (match<l && match<=si && m_data[si-match]==s[l-match-1]) match++;
			if (match==l) return si-l+1; else si--;
		}
		return m_size;
	}


	// replace first occurrence: char
	inline sU32 replace(CCT oldc, CCT newc, sU32 startindex=0) 
	{ 
		sU32 indx=seek(oldc,startindex);
		if (indx<m_size) m_data[indx]=newc;
		return indx;
	}

	// replace first occurrence: sString
	sU32 replace(CST &olds, CST &news, sU32 startindex=0)
	{
		sU32 indx=seek(olds,startindex);
		const sU32 oldsize=olds.m_size;
		const sU32 newsize=news.m_size;
    if (indx<m_size)
		{
			const sU32 oldpos=indx+oldsize;
			const sU32 newpos=indx+newsize;
			const sU32 restlen=m_size-indx-oldsize;

			// expand to fit
			int_expand(news.m_size-olds.m_size);

			// relocate part after replacement, no int_copy because of ordering
			if (restlen)
				if (newpos<oldpos)
					for (sS32 i=0; (sU32)i<restlen; i++) m_data[newpos+i]=m_data[oldpos+i];
				else if (newpos>oldpos)
					for (sS32 i=restlen-1; i>=0; i--) m_data[newpos+i]=m_data[oldpos+i];

			int_cpy(m_data+indx,news.m_data,newsize);
			m_size=m_size+newsize-oldsize;
			int_null();
		}
		return indx;
	}


	// replace first occurrence: C-style string
	sU32 replace(CCP &olds, CCP &news, sU32 startindex=0)
	{
		sU32 indx=seek(olds,startindex);
		const sU32 oldsize=int_strlen(olds);
		const sU32 newsize=int_strlen(news);
    if (indx<m_size)
		{
			const sU32 oldpos=indx+oldsize;
			const sU32 newpos=indx+newsize;
			const sU32 restlen=m_size-indx-oldsize;

			// expand to fit
			int_expand(newsize-oldsize);

			// relocate part after replacement, no int_copy because of ordering
			if (restlen)
				if (newpos<oldpos)
					for (sU32 i=0; i<restlen; i++) m_data[newpos+i]=m_data[oldpos+i];
				else if (newpos>oldpos)
					for (sU32 i=restlen-1; i>=0; i--) m_data[newpos+i]=m_data[oldpos+i];

			int_cpy(m_data+indx,news,newsize);
			m_size=m_size+newsize-oldsize;
			int_null();
		}
		return indx;
	}

	// replace all occurrences: char
	template<class T> inline void replaceall(const T oldc, const T newc) { sU32 indx=0; while ((indx=replace(oldc,newc,indx))<m_size) {} }

	// delete all: chars
	void deleteall(CCT c)
	{
		sU32 newpos=0;
		for (sU32 oldpos=0; oldpos<m_size; oldpos++, newpos++) 
		{
			while (oldpos<m_size && m_data[oldpos]==c) oldpos++;
			if (oldpos<m_size) m_data[newpos]=m_data[oldpos];
		}
		m_size=newpos;
		int_null();
	}

	// delete all: sStrings
	inline void deleteall(CST &s) { ST nullstr; replaceall<CST>(s,nullstr); }

	// delete all: C-Style strings
	inline void deleteall(CCP &s) { replaceall<CCP>(s,0); }

	template <class T> inline ST & operator -= (const T x) { deleteall(x); return *this; }
	template <class T> inline ST operator - (const T x) { ST tmp(*this); tmp.deleteall(x); return tmp; }

	// truncate string to substring
	inline void trunc(sU32 start, sU32 len)
	{
		if (start>m_size) start=m_size;
		if (start+len > m_size) len=m_size-start;
		if (start>0 && len) int_cpy(m_data,m_data+start,len);
		m_size=len;
		int_null();
	}

	ST Tokenize(CCT c, sU32 &currentpos)
	{
		if (currentpos>=m_size) return sString();
		sU32 tpos=seek(c,currentpos);
		sU32 size=tpos-currentpos;
		ST out;
		out.int_expand(size);
		out.m_size=size;
		memcpy(out.m_data,m_data+currentpos,size);
		out.m_data[size]=0;
		return out;
	}

// -----------------------------------------------------------------------------

	// compare operations

	inline bool operator == (CST & s)
	{
		if (m_size != s.m_size) return FALSE;
		for (sU32 i=0; i<m_size; i++) if (m_data[i]!=s.m_data[i]) return FALSE;
		return TRUE;
	}

	inline bool operator == (CCP s)
	{
		sU32 i;
		for (i=0; i<m_size; i++) if (s[i] != m_data[i]) return FALSE;
		return s[i]==0;
	}


	inline bool operator < (CST &s)
	{
		for (sU32 i=0; i<m_size && i<s.m_size; i++)
		{
			if (m_data[i]<s.m_data[i]) return true;
			if (m_data[i]>s.m_data[i]) return false;
		}
		if (m_size < s.m_size) return true;
		return false;
	}

	inline bool operator < (CCP s)
	{
		sU32 l=int_strlen(s);
		for (sU32 i=0; i<m_size && i<l; i++)
		{
			if (m_data[i]<s[i]) return true;
			if (m_data[i]>s[i]) return false;
		}
		if (m_size < l) return true;
		return false;
	}

	inline bool operator > (CST &s)
	{
		for (sU32 i=0; i<m_size && i<s.m_size; i++)
		{
			if (m_data[i]>s.m_data[i]) return true;
			if (m_data[i]<s.m_data[i]) return false;
		}
		if (m_size > s.m_size) return true;
		return false;
	}

	inline bool operator > (CCP s)
	{
		sU32 l=int_strlen(s);
		for (sU32 i=0; i<m_size && i<l; i++)
		{
			if (m_data[i]>s[i]) return true;
			if (m_data[i]<s[i]) return false;
		}
		if (m_size > l) return true;
		return false;
	}


	template<class T> inline bool operator != (const T x) { return !(operator ==(x));}
	template<class T> inline bool operator <= (const T x) { return !(operator >(x)); }
	template<class T> inline bool operator >= (const T x) { return !(operator <(x)); }

};


class sString : public sStringT<sChar, 128, sTRUE>
{
protected:

	static inline sBool is_ws (sChar c) { return (c>0 && c<33); }

  friend int __format8 (sString &arg, const sChar * format, va_list arglist);

public:
	sString() : ST() {}
	sString(CST &s) : ST(s) {}
	sString(CCP s) : ST(s) {}
  sString(CCP a_first, CCP a_last) : ST(a_first, a_last) {}


	sString &ltrim()
	{
		sU32 pos=0;
		while (is_ws(m_data[pos])) pos++;
		if (pos)
		{
			int_cpy(m_data,m_data+pos,m_size-pos+1);
			m_size-=pos;
		}
		return *this;
	}

	sString &rtrim()
	{
		while (m_size && is_ws(m_data[m_size-1])) 
		{
			m_size--;
			int_null();
		}
		return *this;
	}

	sString &upper() { _strupr(m_data); return *this; }
	sString &lower() { _strlwr(m_data); return *this; }
	sString &trim() { ltrim(); rtrim(); return *this; }

	bool operator == (const sString &b) const { return strcmp(m_data,b.m_data)==0; }
	bool operator != (const sString &b) const { return strcmp(m_data,b.m_data)!=0; }
	bool operator <  (const sString &b) const { return strcmp(m_data,b.m_data)<0;  }
	bool operator <= (const sString &b) const { return strcmp(m_data,b.m_data)<=0; }
	bool operator >  (const sString &b) const { return strcmp(m_data,b.m_data)>0;  }
	bool operator >= (const sString &b) const { return strcmp(m_data,b.m_data)>=0; }

  bool operator == (const sChar * b) const { return strcmp(m_data,b)==0; }
	bool operator != (const sChar * b) const { return strcmp(m_data,b)!=0; }
	bool operator <  (const sChar * b) const { return strcmp(m_data,b)<0;  }
	bool operator <= (const sChar * b) const { return strcmp(m_data,b)<=0; }
	bool operator >  (const sChar * b) const { return strcmp(m_data,b)>0;  }
	bool operator >= (const sChar * b) const { return strcmp(m_data,b)>=0; }

  int format (CCP format, ...)
  {
    va_list arg;
    va_start(arg, format);
    int ret = __format8 (*this, format, arg);
    va_end(arg);
    return ret;
  }
};


inline sString operator + (const char *a, const sString &b) { return sString(a)+b; }


#pragma warning (pop)

#endif _STRING_H_

