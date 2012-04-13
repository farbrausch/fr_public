// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "guitool.h"

sF32 gridMapStep(sF32 mx, sInt cnt, sF32 radix)
{
	const sF64 lgr = log(radix);
	const sF64 refv = exp(floor(log(mx / cnt) / lgr) * lgr);

	const sF64 sc[3] = { 1.0, 2.0, 5.0 };
	sInt bm = 0;
	sF64 bo = 0.0f;

	for (sInt i = 0; i < 3; i++)
	{
		sF64 off = fabs(cnt - mx / (refv * sc[i]));

		if (off < bo || i == 0)
		{
			bo = off;
			bm = i;
		}
	}

	return refv * sc[bm];
}
