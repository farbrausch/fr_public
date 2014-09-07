/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Util/Compression.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sHuffman::sHufCtx::sHufCtx(int bits)
{
    sASSERT(bits>0 && bits<=16);
    Max = 1<<bits;
    Mask = Max-1;

    Hist = new int[Max];
    Code = new int[Max];
    Bits = new int[Max];
    Nodes = new sHufNode[Max*2-1];

    Clear();
}

sHuffman::sHufCtx::~sHufCtx()
{
    delete[] Hist;
    delete[] Code;
    delete[] Bits;
    delete[] Nodes;
}

void sHuffman::sHufCtx::Clear()
{
    for(int i=0;i<Max;i++)
    {
        Hist[i] = 0;
        Code[i] = 0;
        Bits[i] = 0;
    }
    for(int i=0;i<Max*2-1;i++)
        sClear(Nodes[i]);

    MaxProp = 0;
    Bandwidth =0 ;
    NodeUsed = 0;
    RootNode = -1;
    MaxProp = 0;
    TotalProp = 0;
    MaxBits = 0;
}

void sHuffman::sHufCtx::MakeTree()
{
    sArray<int> Active;
    Active.HintSize(Max);

    // even the odds to prevent ultra long codes

    for(int i=0;i<Max;i++)
    {
        if(Hist[i]>0)
        {
            TotalProp += Hist[i];
            MaxProp = sMax(MaxProp,Hist[i]);
        }
    }

    if(TotalProp==0)
        return;

    if(MaxProp>65536)
    {
        for(auto i=0;i<Max;i++)
            if(Hist[i]>0)
                Hist[i] = ((int64(Hist[i])<<16) + (MaxProp-1)) / MaxProp;
    }

    // create symbol nodes

    for(int i=0;i<Max;i++)
    {
        if(Hist[i]>0)
        {
            Nodes[NodeUsed].Symbol = i;
            Nodes[NodeUsed].Prop = Hist[i];
            Active.Add(NodeUsed);
            NodeUsed++;
        }
    }

    // create tree

    while(Active.GetCount()>=2)
    {
        int n0 = Active[0];                     // most improbable node
        int n1 = Active[1];                     // second most improbable node
        int n0pos = 0;
        int n1pos = 1;
        if(Nodes[n0].Prop > Nodes[n1].Prop)
        {
            sSwap(n0,n1);
            sSwap(n0pos,n1pos);
        }
        for(int ipos=2;ipos<Active.GetCount();ipos++)
        {
            int i = Active[ipos];
            if(Nodes[i].Prop < Nodes[n1].Prop)
            {
                if(Nodes[i].Prop < Nodes[n0].Prop)
                {
                    n1 = n0;
                    n0 = i;
                    n1pos = n0pos;
                    n0pos = ipos;
                }
                else
                {
                    n1 = i;
                    n1pos = ipos;
                }
            }
            sASSERT(Nodes[n0].Prop <= Nodes[n1].Prop);
        }

        if(n0pos < n1pos)
        {
            Active.RemAt(n1pos);
            Active.RemAt(n0pos);
        }
        else
        {
            Active.RemAt(n0pos);
            Active.RemAt(n1pos);
        }

        Nodes[NodeUsed].Symbol = -1;
        Nodes[NodeUsed].Left = n1;
        Nodes[NodeUsed].Right = n0;
        Nodes[NodeUsed].Prop = Nodes[n0].Prop + Nodes[n1].Prop;
        Active.Add(NodeUsed);
        NodeUsed++;
    }

    sASSERT(Active.GetCount()==1);
    RootNode = Active[0];

    // assign symbols

    AssignSymbol(RootNode,0,0);

    sASSERT(MaxBits<32);        // actually, 32 would be ok.
}

void sHuffman::sHufCtx::AssignSymbol(int node,int value,int bits)
{
    if(Nodes[node].Symbol==-1)
    {
        AssignSymbol(Nodes[node].Left,value,bits+1);
        AssignSymbol(Nodes[node].Right,value|(1<<bits),bits+1);
    }
    else
    {
        Nodes[node].Code = value;
        Nodes[node].CodeBits = bits;

        Code[Nodes[node].Symbol] = value;
        Bits[Nodes[node].Symbol] = bits;

        MaxBits = sMax(MaxBits,bits);
    }
}

/****************************************************************************/

sHuffman::sHuffman()
{
    Data = 0;
    DataUsed = 0;
    DataAlloc = 0;

    PackWord = 0;
    PackShift = 0;
    DataRead = 0;
}

void sHuffman::AddContext(int context,int bits)
{
    Ctx.SetAndGrow(context,new sHufCtx(bits));
}

sHuffman::~sHuffman()
{
    Ctx.DeleteAll();
    delete[] Data;
}

void sHuffman::Realloc(int size)
{
    if(DataUsed>size)
        DataUsed = size;
    uint8 *newdata = new uint8[size];
    sCopyMem(newdata,Data,DataUsed);
    sDelete(Data);
    Data = newdata;
    DataAlloc = size;
}

/****************************************************************************/

void sHuffman::BeginPack()
{
    DataUsed = 0;
    for(auto ctx : Ctx)
        ctx->Clear();
}

void sHuffman::PreparePack(int size)
{
    if(size==0)
    {
        for(auto ctx : Ctx)
            for(int j=0;j<ctx->Max;j++)
                size += ctx->Hist[j];
    }
    Realloc(size);

    for(auto ctx : Ctx)
        ctx->MakeTree();

    PackWord = 0;
    PackShift = 0;
}

void sHuffman::Pack(int context,uint value)
{
    auto ctx = Ctx[context];
    value &= ctx->Mask;
    PackWord |= uint64(ctx->Code[value]) << PackShift;
    PackShift += ctx->Bits[value];
    ctx->Bandwidth +=  ctx->Bits[value];
    if(PackShift >= 32)
    {
        if(DataUsed+4 > DataAlloc)
            Realloc(sMax(DataUsed+4,DataAlloc*2));
        *((uint *)(&Data[DataUsed])) = PackWord & 0xffffffff;
        PackWord = PackWord >> 32;
        PackShift -= 32;
        DataUsed += 4;
    }
}

void sHuffman::EndPack()
{
    if(DataUsed+8 > DataAlloc)
        Realloc(sMax(DataUsed+8,DataAlloc*2));
    *((uint *)(&Data[DataUsed])) = PackWord & 0xffffffff;
    DataUsed += 4;
    *((uint *)(&Data[DataUsed])) = 0;
}

/****************************************************************************/

void sHuffman::BeginUnpack()
{
    PackWord = 0;
    PackShift = 0;
    DataRead = 0;
}

uint sHuffman::Unpack(int context)
{
    if(PackShift<32)
    {
        uint64 newdata = *((uint *)(&Data[DataRead]));
        PackWord |= newdata<<PackShift;

        PackShift += 32;
        DataRead += 4;
        sASSERT(DataRead<=DataUsed+4);
    }

    auto ctx = Ctx[context];
    int node = ctx->RootNode;
    int bits = 0;
    uint mask = 1;
    while(ctx->Nodes[node].Symbol==-1)
    {
        bits++;
        node = (PackWord & mask) ? ctx->Nodes[node].Right : ctx->Nodes[node].Left;
        mask = mask+mask;
    }
    PackWord = PackWord >> bits;
    PackShift = PackShift - bits;

    return ctx->Nodes[node].Symbol;
}

void sHuffman::EndUnpack()
{
}


/****************************************************************************/

