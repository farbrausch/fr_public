// This code is in the public domain. See LICENSE for details.

#include "types.h"
#include "debug.h"

#ifndef __fr_tool_h_
#define __fr_tool_h_

// ---- safe deletes

#define FRSAFEDELETE(a)  { if (a) { delete (a);     (a)=0; } }
#define FRSAFEDELETEA(a) { if (a) { delete[] (a);   (a)=0; } }

// ---- compile time assertions

namespace
{
	template<bool expr> struct CTAssertionHelper
	{
	};

	template<> struct CTAssertionHelper<true>
	{
		inline static void aCompileTimeAssertionHasHappened() { }
	};
};

#define FRCTASSERT(x)	{ CTAssertionHelper<(bool) (x)>::aCompileTimeAssertionHasHappened(); }

namespace fr
{
  // ---- smart pointers

#pragma warning (push)
#pragma warning (disable: 4284)

  template<class T> class scopedPtr
  {
    T       *value;

  public:
    explicit scopedPtr(T *p=0)
      : value(p)
    {
    }

    explicit scopedPtr(scopedPtr &x)
      : value(x.release())
    {
    }

    ~scopedPtr()
    {
      FRSAFEDELETE(value);
    }

    inline void reset(T *p=0)
    {
      if (p!=value)
      {
        FRSAFEDELETE(value);
        value=p;
      }
    }

    inline T *release()
    {
      T *ret=value;
      value=0;
      return ret;
    }

  	inline scopedPtr &operator =(const T *p)
  	{
  		reset(p);
  		return *this;
  	}

		inline scopedPtr &operator =(scopedPtr &x)
		{
			reset(x.release());
			return *this;
		}

  	inline T& operator *() const				{ return *value; }
  	inline T* operator ->() const				{ return value; }
  	inline T* get() const								{ return value; }
  	inline operator T *() const					{	return value;	}
  };

  template<class T, class U> inline bool operator == (const scopedPtr<T> &a, const scopedPtr<U> &b)
  {
  	return a.get()==b.get();
  }

  template<class T, class U> inline bool operator != (const scopedPtr<T> &a, const scopedPtr<U> &b)
  {
  	return a.get()!=b.get();
  }

  template<class T> class scopedArrayPtr
  {
    T       *value;

  public:
    explicit scopedArrayPtr(T *p=0)
      : value(p)
    {
    }

    explicit scopedArrayPtr(scopedArrayPtr &x)
      : value(x.release())
    {
    }

    ~scopedArrayPtr()
    {
      FRSAFEDELETEA(value);
    }

    inline void reset(T *p=0)
    {
      FRSAFEDELETEA(value);
      value=p;
    }

    inline T *release()
    {
      T *tmp=value;
      value=0;
      return tmp;
    }

    inline scopedArrayPtr &operator =(T *p)
    {
      reset(p);
      return *this;
    }

    inline scopedArrayPtr &operator =(scopedArrayPtr &x)
    {
      reset(x.release());
      return *this;
    }

  	inline T* get() const								{ return value; }
  	inline operator T *() const					{	return value;	}
  };

  template<class T, class U> inline bool operator == (const scopedArrayPtr<T> &a, const scopedArrayPtr<U> &b)
  {
  	return a.get()==b.get();
  }

  template<class T, class U> inline bool operator != (const scopedArrayPtr<T> &a, const scopedArrayPtr<U> &b)
  {
  	return a.get()!=b.get();
  }

  template<class T> class sharedPtr
  {
  	T				*value;
  	sUInt		*refCount;

  	inline void release()
  	{
  		if (--*refCount==0)
  		{
  			FRSAFEDELETE(value);
  			delete refCount;
  		}
  	}

  	inline void share(T *rval, sUInt *rrc)
  	{
  		if (rrc!=refCount)
  		{
  			++*rrc;
  			release();
  			value=rval;
  			refCount=rrc;
  		}
  	}

  public:
  	explicit sharedPtr(T *p=0)
  	{
  		value=p;
  		refCount=new sUInt(1);
  	}

    sharedPtr(const sharedPtr &p)
      : value(p.value)
    {
      ++*(refCount=p.refCount);
    }

  	~sharedPtr()
  	{
  		release();
  	}

  	inline sharedPtr &operator =(T *p)
  	{
  		reset(p);
  		return *this;
  	}

  	inline sharedPtr &operator =(const sharedPtr &x)
  	{
  		share(x.value, x.refCount);
  		return *this;
  	}

  	inline void reset(T *p=0)
  	{
  		if (value==p)
  			return;

  		if (--*refCount==0)
  			FRSAFEDELETE(value)
  		else
  			refCount=new sUInt;

  		*refCount=1;
  		value=p;
  	}

  	inline T& operator *() const				{ return *value; }
  	inline T* operator ->() const				{ return value; }
  	inline T* get() const								{ return value; }
  	inline operator T *() const					{	return value;	}

  	inline sUInt useCount() const				{ return *refCount; }
  	inline sBool	isUnique() const			{ return *refCount==1; }
  };

  template<class T, class U> inline bool operator == (const sharedPtr<T> &a, const sharedPtr<U> &b)
  {
  	return a.get()==b.get();
  }

  template<class T, class U> inline bool operator != (const sharedPtr<T> &a, const sharedPtr<U> &b)
  {
  	return a.get()!=b.get();
  }

  template<class T> class sharedArrayPtr
  {
  	T				*value;
  	sUInt		*refCount;

  	inline void release()
  	{
  		if (--*refCount==0)
  		{
  			FRSAFEDELETEA(value);
  			delete refCount;
  		}
  	}

  	inline void share(T *rval, sUInt *rrc)
  	{
  		if (rrc!=refCount)
  		{
  			++*rrc;
  			release();
  			value=rval;
  			refCount=rrc;
  		}
  	}

  public:
  	explicit sharedArrayPtr(T *p=0)
  	{
  		value=p;
  		refCount=new sUInt(1);
  	}

    sharedArrayPtr(const sharedArrayPtr &p)
      : value(p.value)
    {
      ++*(refCount=p.refCount);
    }

  	~sharedArrayPtr()
  	{
  		release();
  	}

  	inline sharedArrayPtr &operator =(T *p)
  	{
  		reset(p);
  		return *this;
  	}

  	inline sharedArrayPtr &operator =(const sharedArrayPtr &x)
  	{
  		share(x.value, x.refCount);
  		return *this;
  	}

  	inline void reset(T *p=0)
  	{
  		if (value==p)
  			return;

  		if (--*refCount==0)
  			FRSAFEDELETEA(value)
  		else
  			refCount=new sUInt;

  		*refCount=1;
  		value=p;
  	}

  	inline T* get() const								{ return value; }
  	inline operator T *() const					{	return value;	}

  	inline sUInt  useCount() const			{ return *refCount; }
  	inline sBool	isUnique() const			{ return *refCount==1; }
  };

  template<class T, class U> inline bool operator == (const sharedArrayPtr<T> &a, const sharedArrayPtr<U> &b)
  {
  	return a.get()==b.get();
  }

  template<class T, class U> inline bool operator != (const sharedArrayPtr<T> &a, const sharedArrayPtr<U> &b)
  {
  	return a.get()!=b.get();
  }

#pragma warning (pop)

	// ---- string class

  class string
  {
	public:
    struct stringData
    {
      sInt    nRefs;
      sInt    nDataLength, nAllocLength;
      
      sChar *data()
      {
        return (sChar *) (this+1);
      }
    };

  protected:
    sChar *strData;
    
    stringData *getData() const;
    void init();
    void allocCopy(string &dest, sInt copyLen, sInt copyIndex, sInt extraLen) const;
    sBool allocBuffer(sInt len);
    void assignCopy(sInt srcLen, const sChar *srcData);
    sBool concatCopy(sInt srcLen1, const sChar *srcData1, sInt srcLen2, const sChar *srcData2);
    void concatInPlace(sInt srcLen, const sChar *srcData);
    void copyBeforeWrite();
    sBool allocBeforeWrite(sInt len);
    void release();
    static void __stdcall release(stringData *data);
    static sInt __stdcall safeStrLen(const sChar *str);

  public:
		string()															{ init(); }
    string(const string &src);
    string(sChar chr, sInt repeat=1);
    string(const sChar *str);
    string(const sUChar *str);
    ~string();

		inline sInt getLength() const					{ return getData()->nDataLength; }
		inline sInt getAllocLength() const		{ return getData()->nAllocLength; }
		sBool isEmpty() const									{ return getData()->nDataLength==0; }
    void empty();

    inline sChar getAt(sInt index) const
		{
			FRASSERT(index>=0);
			FRASSERT(index<=getLength());
			return strData[index];
		}

    inline sChar operator[](sInt index) const
		{
			return getAt(index);
		}

    void setAt(sInt index, sChar chr);
    
		inline operator const sChar *() const	{ return strData; }

    const string &operator =(const string &src);
    const string &operator =(const sChar ch);
    const string &operator =(const sChar *str);
    const string &operator =(const sUChar *str);

    const string &operator +=(const string &src);
    const string &operator +=(const sChar ch);
    const string &operator +=(const sChar *str);
    const string &operator +=(const sUChar *str);

    friend string __stdcall operator +(const string &str1, const string &str2);
    friend string __stdcall operator +(const string &str, const sChar ch);
    friend string __stdcall operator +(const sChar ch, const string &str);
    friend string __stdcall operator +(const string &str1, const sChar *str2);
    friend string __stdcall operator +(const sChar *str1, const string &str2);
    friend string __stdcall operator +(const string &str1, const sUChar *str2);
    friend string __stdcall operator +(const sUChar *str1, const string &str2);

    sInt compare(const sChar *str, sBool ignoreCase=sFALSE) const;

    string mid(sInt first, sInt count) const;
    string mid(sInt first) const;
    string left(sInt count) const;
    string right(sInt count) const;

    void upperCase();
    void lowerCase();

    void trimLeft();
    void trimRight();
    
    sInt insert(sInt index, sChar ch);
    sInt insert(sInt index, const sChar *str);
    sInt remove(sInt index, sInt count=1);

    sInt find(sChar ch) const;
    sInt find(const sChar *str) const;
    sInt reverseFind(sChar ch) const;
    sInt findOneOf(const sChar *set) const;

    sChar *getBuffer(sInt minLength);
    void releaseBuffer(sInt newLength=-1);

    sChar *lockBuffer();
    void unlockBuffer();
	};

  string __stdcall operator +(const string &str1, const string &str2);
  string __stdcall operator +(const string &str, const sChar ch);
  string __stdcall operator +(const sChar ch, const string &str);
  string __stdcall operator +(const sChar *str1, const string &str2);
  string __stdcall operator +(const sUChar *str1, const string &str2);

	inline bool __stdcall operator ==(const string &s1, const string &s2)	{ return s1.compare(s2)==0; }
	inline bool __stdcall operator ==(const sChar *s1, const string &s2)	{ return s2.compare(s1)==0; }
  inline bool __stdcall operator ==(const string &s1, const sChar *s2)  { return s1.compare(s2)==0; }
	inline bool __stdcall operator !=(const string &s1, const string &s2)	{ return s1.compare(s2)!=0; }
	inline bool __stdcall operator !=(const sChar *s1, const string &s2)	{ return s2.compare(s1)!=0; }
  inline bool __stdcall operator !=(const string &s1, const sChar *s2)  { return s1.compare(s2)!=0; }
	inline bool __stdcall operator < (const string &s1, const string &s2)	{ return s1.compare(s2)<0; }
	inline bool __stdcall operator < (const sChar *s1, const string &s2)	{ return s2.compare(s1)>0; }
  inline bool __stdcall operator < (const string &s1, const sChar *s2)  { return s1.compare(s2)<0; }
	inline bool __stdcall operator <=(const string &s1, const string &s2)	{ return s1.compare(s2)<=0; }
	inline bool __stdcall operator <=(const sChar *s1, const string &s2)	{ return s2.compare(s1)>=0; }
  inline bool __stdcall operator <=(const string &s1, const sChar *s2)  { return s1.compare(s2)<=0; }
	inline bool __stdcall operator > (const string &s1, const string &s2)	{ return s1.compare(s2)>0; }
	inline bool __stdcall operator > (const sChar *s1, const string &s2)	{ return s2.compare(s1)<0; }
  inline bool __stdcall operator > (const string &s1, const sChar *s2)  { return s1.compare(s2)>0; }
	inline bool __stdcall operator >=(const string &s1, const string &s2)	{ return s1.compare(s2)>=0; }
	inline bool __stdcall operator >=(const sChar *s1, const string &s2)	{ return s2.compare(s1)<=0; }
  inline bool __stdcall operator >=(const string &s1, const sChar *s2)  { return s1.compare(s2)>=0; }

	// ---- helpers

	class noncopyable
	{
	protected:
		noncopyable()		{}
		~noncopyable()	{}

	private:
		noncopyable(const noncopyable &);
		const noncopyable &operator =(const noncopyable &);
	};
  
  template<class T> T minimum(T a, T b)
  {
    return (a<b)?a:b;
  }

  template<class T> T maximum(T a, T b)
  {
    return (a<b)?b:a;
  }
  
  template<class T> T clamp(T x, T min, T max)
  {
    return (x<min)?min:(x>max)?max:x;
  }
};

#endif
