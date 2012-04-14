#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include "../tool/file.h"
#include "../sounddef.h"
#include "../v2mconv.h"

extern int patchesused[128];


int main(int argc, const char **argv)
{
	if (argc<2)
	{
		printf("\nCONV2M (C) kb^fr 2003\n\nUsage: conv2m <infile> [outfile]\n\n");
		return 1;
	}

	const char *infile=argv[1];
	char *outfile;

	sdInit();

	if (argc<3)
	{
		outfile=new char[strlen(infile)+5];
		const char *pos=strrchr(infile,'.');
		if (pos)
		{
			strncpy(outfile,infile,pos-infile);
			strcpy(outfile+(pos-infile),"_new");
			strcat(outfile,pos);
		}
		else
		{
			strcpy(outfile,infile);
			strcat(outfile,"_new");
		}		
	}
	else
	{
    const char *of=argv[2];
		outfile=new char[strlen(of)+1];
		strcpy(outfile,of);
	}

	fileS in;
	in.open(infile);
	fileM inmemf;
	inmemf.open(in);
	sU32 inlen=inmemf.size();
	sU8 *inptr=(sU8*)inmemf.detach();
	in.close();

  /*
	ConvertV2M(inptr,inlen,&outptr,&outlen);
  */

	unsigned char *outptr;
//	int outlen;

  printf("exporting patches...\n");
	const unsigned char *pdata[128];
  sU32 np=GetV2MPatchData(inptr,inlen,&outptr,pdata);

	for (sU32 i=0; i<np; i++) if (patchesused[i])
	{
		sInt p=(v2curpatch+i)%128;
		//memcpy(soundmem+128*4+v2soundsize*p,v2mpatches[i],v2soundsize);

		char buf[256];
		sprintf(buf,"%s.%03d.v2p",infile,i);
		buf[31]=0;
    fileS out;
    out.open(buf,fileS::wr|fileS::cr);

    out.puts("v2p1");
	  out.write(buf,32);

    out.putsU32(v2version);
	  out.write(pdata[i],v2soundsize);

    out.close();
	}
	
  
	printf("writing %s...\n",outfile);

  /*
	fileS out;
	out.open(outfile,fileS::wr|fileS::cr);
	out.write(outptr,outlen);
	out.close();
  */

	delete[] outptr;
	delete[] outfile;
}
