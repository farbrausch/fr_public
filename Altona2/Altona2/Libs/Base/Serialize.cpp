/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Serialize.hpp"

namespace Altona2 {

#define sSerMaxAlign 16   // change this and break all

/****************************************************************************/
/***                                                                      ***/
/***  Reader                                                              ***/
/***                                                                      ***/
/****************************************************************************/

sReader::sReader(const char *name)
{
    Alloc = 0x10000;
    Used = 0;
    EndPtr = 0;
    Limit = 0;
    Buffer = new uint8[Alloc];
    PtrMap.HintSize(4096);
    PtrMap.Add(0);
    File = sOpenFile(name,sFA_Read);
    Ok = File!=0;
    if(Ok)
        Left = File->GetSize();
    Flush();
}

sReader::~sReader()
{
    Finish();
    delete[] Buffer;
}

bool sReader::Finish()
{
    if(Left!=0 || EndPtr!=Used)
        Ok = 0;
    if(File)
    {
        if(!File->Close())
            Ok = 0;
        delete File;
        File = 0;
    }
    return Ok;
}

void sReader::Flush()
{
    // move away already read data
    sASSERT(Used<=EndPtr);
    if(!Ok)
    {
        Used = 0;
        EndPtr = Alloc;
        return;
    }
    sptr chunk = Used & ~(sSerMaxAlign-1);
    sMoveMem(Buffer,Buffer+chunk,EndPtr-chunk);
    EndPtr = EndPtr-chunk;
    Used -= chunk;
    Limit -= chunk;

    // add some new data

    chunk = (sptr)sMin<int64>(Left,Alloc-EndPtr);
    if(chunk>0)
    {
        if(!File->Read(Buffer+EndPtr,chunk))
            Ok = 0;
        EndPtr += chunk;
        Left -= chunk;
    }
}

int sReader::Begin(uint id,int currentversion,int minversion)
{
    Check(12);
    int _magic,_id,_version;
    *this | _magic | _id | _version;
    if(_magic != 0x5b5b5b5b) Ok = 0;
    if(_id != id) Ok = 0;
    if(_version<minversion || _version>currentversion) Ok = 0;
    return Ok ? _version : 0;
}

bool sReader::End()
{
    Check(4);
    int _magic;
    *this | _magic;
    if(_magic != 0x5d5d5d5d) Ok = 0;
    return Ok;
}

void sReader::RegisterPtr(void *ptr)
{
    PtrMap.Add(ptr);
}

void sReader::VoidPtr(void *&obj)
{
    int n;
    Value(n);
    obj = PtrMap[n];
}

void sReader::SmallData(uptr size,void *v_)
{
    uint8 *v = (uint8 *) v_;
    while(size>0)
    {
        uptr chunk = sMin<uptr>(size,Alloc-Used);
        Check(chunk);
        sCopyMem(v,Buffer+Used,chunk);
        Used += chunk;
        v += chunk;
        size -= chunk;
        if(size)
            Flush();
    }
    Check(1024);
}

void sReader::LargeData(uptr size_,void *v_)
{
    Align(sSerMaxAlign);
    uint8 *v = (uint8 *) v_;
    sptr size = sptr(size_);

    do
    {
        sptr chunk = sMin<sptr>(size,EndPtr-Used);
        sCopyMem(v,Buffer+Used,chunk);
        Used += chunk;
        v += chunk;
        size -= chunk;

        if(size>Alloc)
        {
            sASSERT(Used==EndPtr);
            if(!File->Read(v,size))
                Ok = 0;
            v+=size;
            Left -= size;
            size = 0;
        }
        Flush();
    }
    while(size>0);
    Limit = 0;
}

void sReader::Align(int n)
{
    sASSERT(sIsPower2(n) && n<=sSerMaxAlign);
    uint8 data[16];

    int a = n-(Used&(n-1));
    if(a<n)
        SmallData(a,data);
}

void sReader::String(char *data,uptr max)
{
    uint n = 0;
    Align(4);
    Value(n);
    sASSERT(n<max);
    SmallData(n,data);
    data[n] = 0;
    Align(4);
}

const char *sReader::ReadString()
{
    uint n = 0;
    Align(4);
    Value(n);
    char *data = new char[n+1];
    SmallData(n,data);
    data[n] = 0;
    Align(4);
    return data;
}

/****************************************************************************/
/***                                                                      ***/
/***   Writer                                                             ***/
/***                                                                      ***/
/****************************************************************************/

sWriter::sWriter(const char *name)
{
    Alloc = 0x10000;
    Used = 0;
    Limit = 0;
    Buffer = new uint8[Alloc];
    File = sOpenFile(name,sFA_Write);
    ObjId = 1;
    Ok = File!=0;
}

sWriter::~sWriter()
{
    Finish();
    delete[] Buffer;
}

bool sWriter::Finish()
{
    if(File)
    {
        if(Used>0 && Ok)
        {
            if(!File->Write(Buffer,Used))
                Ok = 0;
        }

        if(!File->Close())
            Ok = 0;
        delete File;
        File = 0;
    }
    return Ok;
}

void sWriter::Flush()
{
    sptr chunk = Used & ~(sSerMaxAlign-1);
    if(chunk>0 && Ok)
    {
        if(!File->Write(Buffer,chunk))
            Ok = 0;
        for(sptr i=0;i<Used-chunk;i++)
            Buffer[i] = Buffer[chunk+i];
    }
    Used = Used-chunk;
    Limit = Used-chunk;
}

int sWriter::Begin(uint id,int currentversion,int minversion)
{
    Check(12);
    *this | uint(0x5b5b5b5b) | id | currentversion;
    return currentversion;
}

bool sWriter::End()
{
    Check(4);
    *this | 0x5d5d5d5d;
    return Ok;
}

void sWriter::SmallData(uptr size,const void *v_)
{
    const uint8 *v = (uint8 *) v_;
    while(size>0)
    {
        uptr chunk = sMin<uptr>(size,Alloc-Used);
        Check(chunk);
        sCopyMem(Buffer+Used,v,chunk);
        Used += chunk;
        v += chunk;
        size -= chunk;
        if(size)
            Flush();
    }
    Check(1024);
}

void sWriter::LargeData(uptr size_,const void *v_)
{
    if(Ok)
    {
        Align(sSerMaxAlign);
        uint8 *v = (uint8 *) v_;
        sptr size = sptr(size_);

        sptr chunk = sMin<sptr>(size,Alloc-Used);
        sCopyMem(Buffer+Used,v,chunk);
        Used += chunk;
        v += chunk;
        size -= chunk;
        Flush();

        sASSERT(Used == 0);
        if(size>Alloc)
        {
            if(!File->Write(v,size))
                Ok = 0;
        }
        else
        {
            sCopyMem(Buffer,v,size);
            Used = size;
        }
        Limit = Used;
    }
}

void sWriter::Align(int n)
{
    sASSERT(sIsPower2(n) && n<=sSerMaxAlign);
    const static uint8 data[16] = { };

    int a = n-(Used&(n-1));
    if(a<n)
        SmallData(a,data);
}

void sWriter::String(const char *data,uptr max)
{
    uint n = (uint) sGetStringLen(data);
    sASSERT(n<0x80000000LL);
    Align(4);
    Value(n);
    sASSERT(n<max);
    SmallData(int(n),data);
    Align(4);
}

void sWriter::WriteString(const char *data)
{
    uint n = (uint) sGetStringLen(data);
    sASSERT(n<0x80000000LL);
    Align(4);
    Value(n);
    SmallData(int(n),data);
    Align(4);
}

/****************************************************************************/

}
