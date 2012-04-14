#ifndef _V2MCONV_H_
#define _V2MCONV_H_

// gives version deltas
int CheckV2MVersion(const unsigned char *inptr, const int inlen);

// converts V2M to newest version
void ConvertV2M(const unsigned char *inptr, const int inlen, unsigned char **outptr, int *outlen);

extern const char * const v2mconv_errors[];

unsigned long GetV2MPatchData(const unsigned char *inptr, const int inlen, 
										 unsigned char **outptr, const unsigned char **patchmap);

#endif