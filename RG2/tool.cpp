// This code is in the public domain. See LICENSE for details.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "debug.h"
#include "types.h"
#include "tool.h"

// required clib functions:
// - new
// - delete
// - memcpy
// - memmove
// required win32 functions:
// - InterlockedIncrement
// - InterlockedDecrement

namespace fr
{
	// ---- support stuff

	static sInt rgInitData[]={-1, 0, 0, 0};
	static string::stringData *nullStringData=(string::stringData *) &rgInitData;
  static const sChar *nullString=(const sChar *) ((sU8 *) &rgInitData+sizeof(string::stringData));
	static const string *emptyString=(string *) &nullString;

	static inline sInt myStrCmp(const sChar *s1, const sChar *s2)
	{
		sInt i=0;

		while (s1[i] && s1[i]==s2[i])
			i++;

		return s1[i]-s2[i];
	}

	static inline sChar toUpper(sChar ch)
	{
		if (ch>='a' && ch<='z')
			return ch&(~0x20);
		else
			return ch;
	}

	static inline sInt myStrCmpI(const sChar *s1, const sChar *s2)
	{
		sInt i=0;

		while (s1[i] && toUpper(s1[i])==toUpper(s2[i]))
			i++;

		return toUpper(s1[i])-toUpper(s2[i]);
	}

	static inline sInt myStrLen(const sChar *str)
	{
    sInt i;
    for (i=0; str[i]; i++);
    
		return i;
	}

	// ---- class impl

	string::stringData *string::getData() const
	{
		FRDASSERT(strData!=0);
		return ((stringData *) strData)-1;
	}

	void string::init()
	{
		strData=emptyString->strData;
	}

	const string &string::operator =(const sUChar *str)
	{
		*this=(const sChar *) str;
		return *this;
	}

	sInt __stdcall string::safeStrLen(const sChar *str)
	{
		return str?myStrLen(str):0;
	}

	sInt string::compare(const sChar *str, sBool ignoreCase) const
	{
		return ignoreCase?myStrCmpI(strData, str):myStrCmp(strData, str);
	}

	inline sBool string::allocBuffer(sInt nLen)
	{
		FRDASSERT(nLen>=0);

		if (nLen==0)
			init();
		else
		{
			stringData *pData=0;
			pData=(stringData *) (new sU8[sizeof(stringData)+(nLen+1)*sizeof(sChar)]);
			if (!pData)
				return sFALSE;

			pData->nRefs=1;
			pData->data()[nLen]=0;
			pData->nDataLength=nLen;
			pData->nAllocLength=nLen;
			strData=pData->data();
		}

		return sTRUE;
	}

	inline void string::release()
	{
		if (strData!=nullString)
		{
			FRDASSERT(getData()->nRefs!=0);
			if (InterlockedDecrement((LONG *) &getData()->nRefs)<=0)
				delete[] (sU8 *) getData();
			init();
		}
	}

	inline void __stdcall string::release(stringData *data)
	{
		if (data!=nullStringData)
		{
			FRDASSERT(data->nRefs!=0);
			if (InterlockedDecrement((LONG *) &data->nRefs)<=0)
				delete[] (sU8 *) data;
		}
	}

	void string::empty()
	{
		if (getData()->nDataLength==0)
			return;

		if (getData()->nRefs>=0)
			release();
		else
			*this="";

		FRDASSERT(getData()->nDataLength==0);
		FRDASSERT(getData()->nRefs<0 || getData()->nAllocLength==0);
	}

	inline void string::copyBeforeWrite()
	{
		if (getData()->nRefs>1)
		{
			stringData *pData=getData();
	    release();
			if (allocBuffer(pData->nDataLength))
				memcpy(strData, pData->data(), (pData->nDataLength+1)*sizeof(sChar));
		}

		FRDASSERT(getData()->nRefs<=1);
	}

	inline sBool string::allocBeforeWrite(sInt len)
	{
		sBool bRet=sTRUE;

		if (getData()->nRefs>1 || len>getData()->nAllocLength)
		{
			release();
			bRet=allocBuffer(len);
		}

		FRDASSERT(getData()->nRefs<=1);
		return bRet;
	}

	inline void string::allocCopy(string &dest, sInt copyLen, sInt copyIndex, sInt extraLen) const
	{
		sInt newLen=copyLen+extraLen;

		if (newLen==0)
			dest.init();
		else
		{
			if (dest.allocBuffer(newLen))
				memcpy(dest.strData, strData+copyIndex, copyLen*sizeof(sChar));
		}
	}

	inline void string::assignCopy(sInt srcLen, const sChar *srcData)
	{
		if (allocBeforeWrite(srcLen))
		{
			memcpy(strData, srcData, srcLen*sizeof(sChar));
			getData()->nDataLength=srcLen;
			strData[srcLen]=0;
		}
	}

	inline sBool string::concatCopy(sInt srcLen1, const sChar *src1, sInt srcLen2, const sChar *src2)
	{
		sBool bRet=sFALSE;
		sInt newLen=srcLen1+srcLen2;

		if (newLen!=0)
		{
			bRet=allocBuffer(newLen);
			if (bRet)
			{
				memcpy(strData, src1, srcLen1*sizeof(sChar));
				memcpy(strData+srcLen1, src2, srcLen2*sizeof(sChar));
			}
		}

		return bRet;
	}

	inline void string::concatInPlace(sInt srcLen, const sChar *src)
	{
		if (srcLen==0)
			return;

		if (getData()->nRefs>1 || getData()->nDataLength+srcLen>getData()->nAllocLength)
		{
			stringData *pOldData=getData();
			if (concatCopy(getData()->nDataLength, strData, srcLen, src))
			{
				FRDASSERT(pOldData != 0);
				release(pOldData);
			}
		}
		else
		{
			memcpy(strData+getData()->nDataLength, src, srcLen*sizeof(sChar));
			getData()->nDataLength+=srcLen;
			FRDASSERT(getData()->nDataLength<getData()->nAllocLength);
			strData[getData()->nDataLength]=0;
		}
	}

	string::string(const string &str)
	{
		FRDASSERT(str.getData()->nRefs!=0);
		if (str.getData()->nRefs>=0)
		{
			FRDASSERT(str.getData() != nullStringData);
			strData=str.strData;
			InterlockedIncrement((LONG *) &str.getData()->nRefs);
		}
		else
		{
			init();
			*this=str.strData;
		}
	}

	string::string(const sChar *str)
	{
		init();

		if (str!=0)
		{
			sInt nLen=safeStrLen(str);
			if (nLen!=0)
			{
				if (allocBuffer(nLen))
					memcpy(strData, str, nLen*sizeof(sChar));
			}
		}
	}

	string::~string()
	{
		if (getData()!=nullStringData)
		{
      stringData *dta=getData();

			if (InterlockedDecrement((LONG *) &getData()->nRefs)<=0)
				delete[] (sU8 *) getData();
		}
	}

	const string &string::operator =(const string &src)
	{
		if (strData!=src.strData)
		{
			if ((getData()->nRefs<0 && getData()!=nullStringData) || src.getData()->nRefs<0)
				assignCopy(src.getData()->nDataLength, src.strData);
			else
			{
				release();
				FRDASSERT(src.getData() != nullStringData);
				strData = src.strData;
				InterlockedIncrement((LONG *) &getData()->nRefs);
			}
		}

		return *this;
	}

	const string &string::operator =(const sChar *str)
	{
		FRDASSERT(str==0 || !IsBadStringPtr(str, (sUInt) -1));

		assignCopy(safeStrLen(str), str);
		return *this;
	}

	string __stdcall operator +(const string &str1, const string &str2)
	{
		string s;
		s.concatCopy(str1.getLength(), str1.strData, str2.getLength(), str2.strData);
		return s;
	}

	string __stdcall operator +(const sChar *str1, const string &str2)
	{
		string s;
		s.concatCopy(string::safeStrLen(str1), str1, str2.getLength(), str2.strData);
		return s;
	}

	string __stdcall operator +(const sUChar *str1, const string &str2)
	{
		string s;
		s.concatCopy(string::safeStrLen((const sChar *) str1), (const sChar *) str1, str2.getLength(), str2.strData);
		return s;
	}

	const string &string::operator +=(const sChar *str)
	{
		FRDASSERT(str==0 || !IsBadStringPtr(str, (sUInt) -1));
		concatInPlace(safeStrLen(str), str);
		return *this;
	}

	const string &string::operator +=(const sChar ch)
	{
		concatInPlace(1, &ch);
		return *this;
	}

	const string &string::operator +=(const string &str)
	{
		concatInPlace(str.getLength(), str.strData);
		return *this;
	}

	sChar *string::getBuffer(sInt minLength)
	{
		FRDASSERT(minLength>=0);

		if (getData()->nRefs>1 || minLength>getData()->nAllocLength)
		{
			stringData *pOldData=getData();
			sInt nOldLen=getData()->nDataLength;
			if (minLength<nOldLen)
				minLength=nOldLen;

			if (allocBuffer(minLength))
			{
				memcpy(strData, pOldData->data(), (nOldLen+1)*sizeof(sChar));
				getData()->nDataLength=nOldLen;
				release(pOldData);
			}
		}
		
		FRDASSERT(getData()->nRefs<=1);
	  FRDASSERT(strData != 0);

		return strData;
	}

	void string::releaseBuffer(sInt newLength)
	{
		copyBeforeWrite();
		
		if (newLength==-1)
			newLength=myStrLen(strData);

		FRDASSERT(newLength <= getData()->nAllocLength);
		getData()->nDataLength=newLength;
		strData[newLength]=0;
	}

	sChar *string::lockBuffer()
	{
		sChar *str=getBuffer(0);
		getData()->nRefs=-1;
		return str;
	}

	void string::unlockBuffer()
	{
		FRDASSERT(getData()->nRefs==-1);
		if (getData()!=nullStringData)
			getData()->nRefs=1;
	}

	sInt string::find(sChar ch) const
	{
		sInt ndx=-1;

		for (sInt i=0; i<getLength(); i++)
		{
			if (strData[i]==ch)
			{
				ndx=i;
				break;
			}
		}

		return ndx;
	}

	sInt string::find(const sChar *str) const
	{
		sInt ndx=-1;
		sInt i, j;

		for (i=0; i<getLength(); i++)
		{
			for (j=0; str[j] && strData[i+j]==str[j]; j++);

			if (!str[j])
			{
				ndx=i;
				break;
			}
		}

		return ndx;
	}

	sInt string::reverseFind(sChar ch) const
	{
		sInt ndx=-1;

		for (sInt i=getLength()-1; i>=0; --i)
		{
			if (strData[i]==ch)
			{
				ndx=i;
				break;
			}
		}

		return ndx;
	}

	sInt string::findOneOf(const sChar *set) const
	{
		sInt ndx=-1;

		for (sInt i=0; i<getLength(); i++)
		{
			for (sInt j=0; set[j]; j++)
			{
				if (strData[i]==set[j])
				{
					ndx=i;
					goto out;
				}
			}
		}

	out:
		return ndx;
	}

	string string::mid(sInt first, sInt count) const
	{
		if (first<0)
			first=0;

		if (count<0)
			count=0;

		if (first+count>getLength())
			count=getLength()-first;

		if (first>getLength())
			count=0;

		string dest;
		allocCopy(dest, count, first, 0);
		return dest;
	}

	string string::mid(sInt first) const
	{
		return mid(first, getLength()-first);
	}

	string string::left(sInt count) const
	{
		if (count<0)
			count=0;
		else if (count>getLength())
			count=getLength();

		string dest;
		allocCopy(dest, count, 0, 0);
		return dest;
	}

	string string::right(sInt count) const
	{
		if (count<0)
			count=0;
		else if (count>getLength())
			count=getLength();

		string dest;
		allocCopy(dest, count, getLength()-count, 0);
		return dest;
	}

	void string::upperCase()
	{
		copyBeforeWrite();

		for (sInt i=0; i<getLength(); i++)
		{
			if (strData[i]>='a' && strData[i]<='z')
				strData[i]&=~0x20;
		}
	}

	void string::lowerCase()
	{
		copyBeforeWrite();

		for (sInt i=0; i<getLength(); i++)
		{
			if (strData[i]>='A' && strData[i]<='Z')
				strData[i]|=0x20;
		}
	}

	void string::trimRight()
	{
		copyBeforeWrite();

    sInt i;
		for (i=getLength()-1; i>=0; i--)
		{
			if (strData[i]!='\r' && strData[i]!='\n' && strData[i]!='\t' && strData[i]!=' ')
				break;
		}

		strData[i+1]=0;
		getData()->nDataLength=i+1;
	}

	void string::trimLeft()
	{
		copyBeforeWrite();

    sInt i;
		for (i=0; strData[i]; i++)
		{
			if (strData[i]!='\r' && strData[i]!='\n' && strData[i]!='\t' && strData[i]!=' ')
				break;
		}

		sInt newLength=getLength()-i;
		memmove(strData, strData+i, (newLength+1)*sizeof(sChar));
		getData()->nDataLength=newLength;
	}

	sInt string::insert(sInt index, sChar ch)
	{
		copyBeforeWrite();

		if (index<0)
			index=0;

		sInt newLength=getLength();
		if (index>newLength)
			index=newLength;

		newLength++;

		if (getData()->nAllocLength<newLength)
		{
			stringData *pOldData=getData();
			const sChar *str=strData;
			if (!allocBuffer(newLength))
				return -1;

			memcpy(strData, str, (pOldData->nDataLength+1)*sizeof(sChar));
			release(pOldData);
		}

		memmove(strData+index+1, strData+index, (newLength-index)*sizeof(sChar));
		strData[index]=ch;
		getData()->nDataLength=newLength;

		return newLength;
	}

	sInt string::insert(sInt index, const sChar *str)
	{
		if (index<0)
			index=0;

		sInt insertLength=safeStrLen(str);
		sInt newLength=getData()->nDataLength;

		if (insertLength>0)
		{
			copyBeforeWrite();
			if (index>newLength)
				index=newLength;

			newLength+=insertLength;
			if (getData()->nAllocLength<newLength)
			{
				stringData *pOldData=getData();
				const sChar *str=strData;
				if(!allocBuffer(newLength))
					return -1;

				memcpy(strData, str, (pOldData->nDataLength+1)*sizeof(sChar));
				release(pOldData);
			}

			memmove(strData+index+insertLength, strData+index, (newLength-index-insertLength+1)*sizeof(sChar));
			memcpy(strData+index, str, insertLength*sizeof(sChar));
			getData()->nDataLength=newLength;
		}

		return newLength;
	}

	sInt string::remove(sInt index, sInt count)
	{
		if (index<0)
			index=0;

		sInt newLength=getLength();
		if (count>0 && index<newLength)
		{
			copyBeforeWrite();
			sInt charsToCopy=newLength-(index+count)+1;

			memmove(strData+index, strData+index+count, charsToCopy*sizeof(sChar));
			getData()->nDataLength=newLength-count;
		}

		return newLength;
	}
}
