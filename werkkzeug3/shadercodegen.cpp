// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "shadercodegen.hpp"
#include "_startdx.hpp"

/****************************************************************************/

// Control structure state
struct sShaderCodeGen::Control
{
  sBool Exec;       // execute instrs yes/no
  sBool Free;       // no if in a chain yet taken?
  const sU32 *Loop; // loop point
  sU32 Condition;   // while condition
};

// Evaluates a condition
// Encoding:
// Bit   0-4: Bitfield start
// Bit   5-7: Bitfield len (minus 1)
// Bit  8-15: Compare ref value
// Bit 16-24: Word offset (>=496 reserved)
// Bit 25-28: Index register specifier
// Bit 29-30: Comparision op (0 = "==", 1 = "<", 2 = ">")
// Bit    31: Comparision negate
sU32 sShaderCodeGen::GetFlagWord(sU32 addr)
{
  sU32 wordOffs = ((addr >> 16) + Index[(addr >> 25) & 0xf]) & 0x1ff;

  if(wordOffs >= 0x1f0) // special
    return ((sU32 *) Index)[wordOffs - 0x1f0];
  else
    return DataArea[wordOffs];
}

sBool sShaderCodeGen::EvalCond(sU32 condition)
{
  // get the input value
  sU32 fieldStart = condition & 0x1f;
  sU32 fieldLen = (condition >> 5) & 0x7;
  sU32 value = (GetFlagWord(condition) >> fieldStart) & ((2 << fieldLen) - 1);

  // perform the comparision
  sU32 ref = (condition >> 8) & 0xff;
  sBool result;

  switch((condition >> 29) & 3)
  {
  case 0:   result = value == ref;  break;
  case 1:   result = value < ref;   break;
  case 2:   result = value > ref;   break;
  default:  result = sFALSE;        break;
  }

  // negate if required
  if(condition & 0x80000000)
    result = !result;

  return result;
}

// Phase 1 reads in the shader, evaluates if statements, and resolves aliases
sBool sShaderCodeGen::Phase1(const sU32 *input)
{
  sU32 aliasTable[MAXTEMP];
  sU32 remapUp[MAXTEMP];
  Control control[MAXNEST+1],*ctrl=control;
  sU32 *codePtr = Code;
  sInt tempReg = MAXTEMP;

  // Check whether input actually *is* a shader
  if((*input & 0xfffe0000) != 0xfffe0000)
    return sFALSE;

  IsPS11 = *input == 0xffff0101;
  IsV1Shader = (*input & 0xff00) == 0x100;

  // copy first (version) word, initialize tables and global control scope
  *codePtr++ = *input++;

  sSetMem(aliasTable,0,sizeof(aliasTable));
  sSetMem(remapUp,0,sizeof(remapUp));
  sSetMem(Index,0,sizeof(Index));
  FirstInstr = 0;

  ctrl->Exec = sTRUE;
  ctrl->Loop = input;
  ctrl->Condition = 0;

  // Loop through the shader instruction by instruction, throwing every-
  // thing in non-taken ifs away and processing virtual moves
  sU32 opcode;

  do
  {
    sU32 *outOpStart = codePtr;
    if(codePtr + LONGESTINSTR > &Code[MAXCODEWORDS])
      return sFALSE;

    opcode = *input++;
    sInt nOperands = (opcode >> 24) & 0xf;
    const sU32 *opNext = input + nOperands;

    sBool exec = ctrl->Exec, taken;

    switch(opcode)
    {
      // pseudoinstructions
    case XO_EXT_IF:
      taken = EvalCond(*input++);
      (++ctrl)->Exec = taken & exec;
      ctrl->Free = !taken;
      ctrl->Loop = 0;
      break;

    case XO_EXT_ELSE:
      ctrl->Exec = ctrl->Free & ctrl[-1].Exec;
      break;

    case XO_EXT_ELIF:
      taken = EvalCond(*input++);
      ctrl->Exec = ctrl->Free & taken & ctrl[-1].Exec;
      if(taken)
        ctrl->Free = sFALSE;
      break;

    case XO_EXT_WHILE:
      (++ctrl)->Condition = *input++;
      ctrl->Exec = EvalCond(ctrl->Condition) & exec;
      ctrl->Loop = input;
      break;

    case XO_EXT_END:
      if(exec && ctrl->Loop && EvalCond(ctrl->Condition))
        opNext = ctrl->Loop;
      else
        ctrl--;  
      break;

    case XO_EXT_VMOV:
      if(exec)
      {
        sU32 destReg = *input++ & 0x70001fff;
        sU32 src = *input;
        sVERIFY(destReg < MAXTEMP);

        // resolve aliasing here, the mapping may change later
        if(src & 0x80000000) // register source
        {
          sU32 srcn = src & 0x70001fff;
          if(srcn < MAXTEMP && aliasTable[srcn])
            srcn = aliasTable[srcn];

          aliasTable[destReg] = (src & 0x8fffe000) | srcn;
        }
        else
          aliasTable[destReg] = GetFlagWord(src);
      }
      break;

    case XO_EXT_INDEXED:
      if(exec)
      {
        sU32 code = *input;
        codePtr[-sInt(code & 0xff)] += Index[(code >> 8) & 0xf];
      }
      break;

    case XO_EXT_IADD:
      if(exec)
      {
        sU32 code = *input;
        Index[(code >> 8) & 0xf] = Index[(code >> 12) & 0xf] + (code & 0xff);
      }
      break;

    case XO_EXT_ERROR:
      if(exec)
        return sFALSE;
      break;

    case XO_EXT_FREE:
      if(exec)
      {
        sU32 reg = *input & 0x70001fff;
        if(reg < MAXTEMP)
        {
          remapUp[reg] = ++tempReg;
          aliasTable[reg] = tempReg | XSALL;
          if(tempReg >= MAXTEMP2)
            return sFALSE;
        }
      }
      break;

      // instructions
    case XO_DEF: // needs special handling because the operands are floats
      if(exec)
      {
        // just copy 6 words straight, starting from the opcode
        sCopyMem(codePtr,input-1,6*sizeof(sU32));
        codePtr += 6;
      }
      break;

    default:
      if(exec)
      {
        if(opcode != XO_DCL && !FirstInstr)
          FirstInstr = codePtr;

        *codePtr++ = opcode;
        sU32 destReg = MAXTEMP;

        // go through operands, processing aliasing
        for(sInt i=0;i<nOperands;i++)
        {
          sU32 op = *input++;
          sU32 reg = op & 0x70001fff;

          // save destination register
          if(i == 0)
          {
            if(reg < MAXTEMP && remapUp[reg])
              reg = remapUp[reg];

            destReg = reg;
          }
          
          // process aliases for source registers
          if(i != 0 && reg < MAXTEMP && aliasTable[reg])
          {
            reg = aliasTable[reg];
            if((reg & 0xff0000) == 0xe40000) // not swizzled?
              reg &= 0xff00ffff;
            else if((op & 0xff0000) == 0xe40000) // op not swizzled
              op &= 0xff00ffff; // remove swizzle from original operand
            else
              sVERIFYFALSE;
          }

          // lifetime tracking
          if(reg < MAXTEMP2)
          {
            LifeEnd[reg] = outOpStart;

            // lrp and nrm require destination registers to be different
            // from source
            if(opcode == XO_LRP)
              LifeEnd[reg] += 5;
            else if(opcode == XO_NRM)
              LifeEnd[reg] += 3;
          }

          *codePtr++ = (op & 0x8fffe000) | reg;
        }

        // destination may not be an alias
        if(destReg < MAXTEMP)
          aliasTable[destReg] = 0;
      }
      break;
    }

    input = opNext;
  }
  while(opcode != XO_END);

  // compute code length
  CodeLen = codePtr - Code;

  return sTRUE;
}

// Phase 2 performs register allocation
sBool sShaderCodeGen::Phase2()
{
  sInt logReg[MAXTEMP2];
  sInt physReg[MAXPHYSREG+1];
  sU32 *lifeEnd[MAXPHYSREG];
  sU32 *lastUsed[MAXPHYSREG];

  // fill register maps
  sSetMem(logReg,0xff,sizeof(logReg));
  sSetMem(physReg,0xff,sizeof(physReg));
  sSetMem(lifeEnd,0,sizeof(lifeEnd));
  sSetMem(lastUsed,0,sizeof(lastUsed));

  // register allocation
  sU32 *current = FirstInstr;

  while(*current != XO_END)
  {
    // free all registers whose lifetime ends this instr
    for(sInt i=0;i<MAXPHYSREG;i++)
      if(lifeEnd[i] == current)
        physReg[i] = -1;

    // get opcode
    sU32 opcode = *current++;
    sInt nOperands = (opcode >> 24) & 0xf;

    // remap registers, alloc registers whose lifetime starts this instr
    sInt physRegCtr = 0;

    for(sInt i=0;i<nOperands;i++)
    {
      sU32 op = *current;
      sU32 reg = op & 0x70001fff;

      if(reg < MAXTEMP2) // temp register?
      {
        if(logReg[reg] == -1) // lifetime starts here, alloc physical reg
        {
          // find free physical register
          while(physReg[physRegCtr] != -1)
            physRegCtr++;

          if(physRegCtr == MAXPHYSREG) // out of (real) physical registers
            return sFALSE;

          // specialized allocation for texture loads to avoid unnecessary
          // dependencies
          sInt phys = physRegCtr;
          if((opcode & 0xfff0ffff) == XO_TEXLD)
          {
            // use free physical register whose last use was longest ago
            // (to minimize dependency chains generated)
            for(sInt j=0;j<MAXPHYSREG;j++)
              if(physReg[j] == -1 && lastUsed[j] < lastUsed[phys])
                phys = j;
          }

          physReg[phys] = reg;
          lifeEnd[phys] = LifeEnd[reg];
          logReg[reg] = phys;
        }

        if(!IsPS11)
          reg = logReg[reg];

        lastUsed[reg] = current;
      }

      *current++ = (op & 0x8fffe000) | reg;
    }
  }

  return sTRUE;
}

// Remove opcode lengths from opcodes. Must be last. Required for 1.x shaders
void sShaderCodeGen::RemoveOpcodeLengths()
{
  sU32 *current = &Code[1];
  while(*current != XO_END)
  {
    sInt nOperands = (*current >> 24) & 0xf;

    *current &= 0xf0ffffff;
    current += nOperands + 1;
  }
}

/****************************************************************************/

sU32 *sShaderCodeGen::GenCode(const sU32 *input,const sU32 *data)
{
  sU32 *result = 0;

  DataArea = data;
  if(Phase1(input) && Phase2())
  {
    if(IsV1Shader)
      RemoveOpcodeLengths();

    result = new sU32[CodeLen];
    sCopyMem4(result,Code,CodeLen);
  }

  return result;
}

/****************************************************************************/
