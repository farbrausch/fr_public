// This code is in the public domain. See LICENSE for details.

// typed stream

#ifndef __fr_tstream_h_
#define __fr_tstream_h_

#include "types.h"
#include "stream.h"

namespace fr
{
  class string;

	class streamWrapper
	{
		stream& m_hostStream;
		sBool		m_write;

	public:
		streamWrapper(stream &host, sBool write);
		~streamWrapper();

		streamWrapper& operator << (sInt& val);
    streamWrapper& operator << (sUInt& val);
		streamWrapper& operator << (sChar& val);
		streamWrapper& operator << (sS8& val);
		streamWrapper& operator << (sS16& val);
		streamWrapper& operator << (sS32& val);
		streamWrapper& operator << (sS64& val);
		streamWrapper& operator << (sU8& val);
		streamWrapper& operator << (sU16& val);
		streamWrapper& operator << (sU32& val);
		streamWrapper& operator << (sU64& val);
		streamWrapper& operator << (sF32& val);
		streamWrapper& operator << (sF64& val);
		streamWrapper& operator << (string& val);

		sBool isWrite() const
		{
			return m_write;
		}
	};
}

#endif
