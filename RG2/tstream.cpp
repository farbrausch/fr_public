// This code is in the public domain. See LICENSE for details.

// typed stream implementation

#include "stdafx.h"
#include "tstream.h"
#include "tool.h"

namespace fr
{
  // ---- streamWrapper

  streamWrapper::streamWrapper(stream& host, sBool write)
    : m_hostStream(host), m_write(write)
  {
  }

  streamWrapper::~streamWrapper()
  {
  }

  streamWrapper& streamWrapper::operator << (sInt& val)
  {
    if (m_write)
      m_hostStream.putsInt(val);
    else
      val = m_hostStream.getsInt();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sUInt& val)
  {
    if (m_write)
      m_hostStream.putsUInt(val);
    else
      val = m_hostStream.getsUInt();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sChar& val)
  {
    if (m_write)
      m_hostStream.putsChar(val);
    else
      val = m_hostStream.getsChar();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sS8& val)
  {
    if (m_write)
      m_hostStream.putsS8(val);
    else
      val = m_hostStream.getsS8();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sS16& val)
  {
    if (m_write)
      m_hostStream.putsS16(val);
    else
      val = m_hostStream.getsS16();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sS32& val)
  {
    if (m_write)
      m_hostStream.putsS32(val);
    else
      val = m_hostStream.getsS32();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sS64& val)
  {
    if (m_write)
      m_hostStream.putsS64(val);
    else
      val = m_hostStream.getsS64();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sU8& val)
  {
    if (m_write)
      m_hostStream.putsU8(val);
    else
      val = m_hostStream.getsU8();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sU16& val)
  {
    if (m_write)
      m_hostStream.putsU16(val);
    else
      val = m_hostStream.getsU16();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sU32& val)
  {
    if (m_write)
      m_hostStream.putsU32(val);
    else
      val = m_hostStream.getsU32();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sU64& val)
  {
    if (m_write)
      m_hostStream.putsU64(val);
    else
      val = m_hostStream.getsU64();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sF32& val)
  {
    if (m_write)
      m_hostStream.putsF32(val);
    else
      val = m_hostStream.getsF32();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (sF64& val)
  {
    if (m_write)
      m_hostStream.putsF64(val);
    else
      val = m_hostStream.getsF64();

    return *this;
  }

  streamWrapper& streamWrapper::operator << (string& val)
  {
    if (m_write)
    {
      m_hostStream.putsU32(val.getLength());
      m_hostStream.puts(val);
    }
    else
    {
      sU32 len = m_hostStream.getsU32();
      sChar* buf = val.getBuffer(len);
      m_hostStream.read(buf, len);
      buf[len] = 0;
      val.releaseBuffer();
    }

    return *this;
  }
}
