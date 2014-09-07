/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#ifndef FILE_ALTONA2_LIBS_UTIL_COMPRESSION_HPP
#define FILE_ALTONA2_LIBS_UTIL_COMPRESSION_HPP

#include "Altona2/Libs/Base/Base.hpp"
namespace Altona2 {

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

class sHuffman
{
    struct sHufNode
    {
        int Symbol;                         // word in the alphabeth to encode, or -1 for leaf)
        int Code;                           // word in the encoded alphabeth
        int CodeBits;                       // number of bits in code
        int Left;                           // left node ('0')
        int Right;                          // right node ('1')
        int Prop;                           // propability of this node (left + right, or leaf)
    };

    class sHufCtx
    {
    public:
        sHufCtx(int bits);
        ~sHufCtx();

        int Max;                            // Symbols from 0..Max-1, max must be power of two
        int Mask;                           // bitmask for symbols.

        int *Hist;                          // Histogram counter for propabilities
        int *Code;                          // symbol -> code map for encoding
        int *Bits;                          // symbol -> number of bits in code for encoding

        int MaxProp;                        // max propability encountered
        int TotalProp;                      // sum of all propabilities
        int MaxBits;                        // number of bits of longest code
        sHufNode *Nodes;                    // tree of nodes that represents the code
        int NodeUsed;                       // number of nodes used
        int RootNode;                       // the root node

        uint64 Bandwidth;                   // while encoding, count the number of bits written

        void Clear();
        void MakeTree();
        void AssignSymbol(int node,int value,int bits);
    };

    uint64 PackWord;                        // buffer for packing / unpacking
    int PackShift;                          // buffer for packing / unpacking
    int DataRead;                           // read pointer for unpacking

    sArray<sHufCtx *> Ctx;
public:
    sHuffman();
    ~sHuffman();
    void AddContext(int context,int bits);

    // packer interface

    void BeginPack();
    void HistPack(int context,uint value) { auto ctx = Ctx[context]; ctx->Hist[value & ctx->Mask]++; }
    void PreparePack(int size=0);
    void Pack(int context,uint value);
    void EndPack();

    // depacker interface

    void BeginUnpack();
    uint Unpack(int context);
    void EndUnpack();

    // data

    uint8 *Data;
    int DataUsed;
    int DataAlloc;
    void Realloc(int size);
    int GetBandwidth(int context) { return int(Ctx[context]->Bandwidth/8); }
};


/****************************************************************************/
}
#endif  // FILE_ALTONA2_LIBS_UTIL_COMPRESSION_HPP

