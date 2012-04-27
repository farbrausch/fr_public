/****************************************************************************/

#include "main.hpp"
#include "base/system.hpp"
#include "util/bitio.hpp"

/****************************************************************************/

static void TestBitIO()
{
  static const sInt bufferSize = 65536;
  static const sInt seed = 1234;

  static const sInt numValues = 1000000; // for the second (disk-based io) run

  // ---- first a bunch of random reads/writes to a memory buffer to see whether that works

  sU8 buffer[bufferSize];
  sRandomMT rand;

  // writer
  sPrintF(L"running in-memory writer test...\n");
  {
    sBitWriter writer;
    writer.Start(buffer,bufferSize);
    rand.Seed(seed);
    for(sInt i=0;i<(bufferSize*8)/24;i++)
    {
      sInt len = rand.Int(23) + 1;
      sU32 value = rand.Int32() & ((1 << len) - 1);
      writer.PutBits(value,len);
    }
    sDInt bytes = writer.Finish();
    sVERIFY(bytes != -1);
  }

  // reader
  sPrintF(L"running in-memory reader test...\n");
  {
    sBitReader reader;
    reader.Start(buffer,bufferSize);
    rand.Seed(seed);
    for(sInt i=0;i<(bufferSize*8)/24;i++)
    {
      sInt len = rand.Int(23) + 1;
      sU32 checkValue = rand.Int32() & ((1 << len) - 1);
      sU32 getValue = reader.GetBits(len);
      sVERIFY(getValue == checkValue);
    }

    sBool ok = reader.Finish();
    sVERIFY(ok);
  }

  // ---- then a lot more of the same to a file
  sDInt bytes;

  sPrintF(L"running file-based writer test...\n");
  {
    sBitWriter writer;
    sFile *file = sCreateFile(L"test.dat",sFA_WRITE);

    writer.Start(file);
    rand.Seed(seed+1);
    for(sInt i=0;i<numValues;i++)
    {
      sInt len = rand.Int(23) + 1;
      sU32 value = rand.Int32() & ((1 << len) - 1);
      writer.PutBits(value,len);
    }

    bytes = writer.Finish();
    sVERIFY(bytes != -1);
    sVERIFY(bytes == file->GetOffset());

    static const sChar8 endTag[] = "0123";
    file->Write(endTag,4);

    sDelete(file);
  }

  sPrintF(L"running file-based reader test...\n");
  {
    sBitReader reader;
    sFile *file = sCreateFile(L"test.dat",sFA_READ);

    reader.Start(file);
    rand.Seed(seed+1);

    for(sInt i=0;i<numValues;i++)
    {
      sInt len = rand.Int(23) + 1;
      sU32 checkValue = rand.Int32() & ((1 << len) - 1);
      sU32 getValue = reader.GetBits(len);
      sVERIFY(getValue == checkValue);
    }

    sBool ok = reader.Finish();
    sVERIFY(ok);
    sVERIFY(file->GetOffset() == bytes);

    // now try reading the end tag bitwise
    reader.Start(file);
    for(sInt i=0;i<4;i++)
    {
      sBool ok = reader.GetBits(8) == (i + '0');
      sVERIFY(ok);
      sVERIFY(reader.IsOk());
    }

    // check whether reading one bit past the end signals an error
    sInt value = reader.GetBits(1);
    sVERIFY(value == 0);
    sVERIFY(!reader.IsOk());

    // all ok
    reader.Finish();
    sDelete(file);
  }

  sDeleteFile(L"test.dat");
  sPrintF(L"all clear!\n");
}

static void TestHuffman()
{
  sBool ok;

  // ---- writing
  sPrintF(L"testing huffman encoding...\n");

  // load test file
  sDInt size;
  sU8 *testData = sLoadFile(L"../../main/base/types.cpp",size);
  sVERIFY(testData != 0);

  // count character frequencies
  sU32 freq[256];
  sClear(freq);
  for(sInt i=0;i<size;i++)
    freq[testData[i]]++;

  // build huffman codes
  sU32 code[256];
  sInt lens[256];
  sBuildHuffmanCodes(code,lens,freq,256,24);

  // write file
  sBitWriter writer;
  sFile *file = sCreateFile(L"types_huff.dat",sFA_WRITE);
  writer.Start(file);

  ok = sWriteHuffmanCodeLens(writer,lens,256);
  sVERIFY(ok);

  for(sInt i=0;i<size;i++)
    writer.PutBits(code[testData[i]],lens[testData[i]]);

  sDInt bytes = writer.Finish();
  sVERIFY(bytes != -1);
  sDelete(file);

  // ---- reading
  sPrintF(L"testing huffman decoding...\n");

  sDInt packedSize;
  sU8 *packedData = sLoadFile(L"types_huff.dat",packedSize);
  sVERIFY(packedData != 0);

  sBitReader reader;
  reader.Start(packedData,packedSize);

  sInt newLens[256];
  ok = sReadHuffmanCodeLens(reader,newLens,256);
  sVERIFY(ok);
  for(sInt i=0;i<256;i++)
    sVERIFY(lens[i] == newLens[i]);

  sFastHuffmanDecoder huffdec;
  ok = huffdec.Init(lens,256);
  sVERIFY(ok);

  for(sInt i=0;i<size;i++)
  {
    sInt sym = huffdec.DecodeSymbol(reader);
    sVERIFY(sym == testData[i]);
  }

  ok = reader.Finish();
  sVERIFY(ok);

  // cleanup
  sDeleteFile(L"types_huff.dat");
  delete[] packedData;
  delete[] testData;
}

void sMain()
{
  TestBitIO();
  TestHuffman();
}

/****************************************************************************/

