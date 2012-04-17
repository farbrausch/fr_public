// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "exporter.h"
#include "frOpGraph.h"
#include "debug.h"
#include "animbase.h"
#include "plugbase.h"
#include "texbase.h"
#include "WindowMapper.h"
#include "resource.h"
#include "viruzII.h"
#include "ViruzII/v2mconv.h"
#include "exportTool.h"
#include <algorithm>
#include <stdarg.h>

// ---- support

class exportException
{
public:
  exportException(const sChar* desc)    { strcpy(m_description, desc); }
  
  const sChar* getDescription() const   { return m_description; }
  
private:
	sChar		m_description[2048];
};

static void throwExportException(const sChar* fmt, ...)
{
  static sChar buf[2048];
  va_list arg;

  va_start(arg, fmt);
  vsprintf(buf, fmt, arg);
  va_end(arg);

  throw exportException(buf);
}

static sF32 adjustFloat(sF32 val, sBool roundDir)
{
  sU32 fb = *((sU32 *) &val);
  if (fb >> 31)
    roundDir = !roundDir;

  if (!roundDir && (fb & 0x7fffff) < 255)
    roundDir = !roundDir;

  if (roundDir)
    fb += 0xff;
  else
    fb -= 0xff;

  fb &= 0xffffff00;

  return *((sF32 *) &fb);
}

static void putPackedFloatNoRound(fr::stream& f, const sF32 value)
{
  if (value == 0.0f)
    f.putsU8(0);
  else if (value == 1.0f)
    f.putsU8(1);
  else if (value == -1.0f)
    f.putsU8(0xff);
  else
  {
    const sU32 fltVal = *reinterpret_cast<const sU32*>(&value);
    const sU8 exp = fltVal >> 23;
    FRASSERT(exp != 0x00 && exp != 0xff && exp != 0x01);

    f.putsU8(exp); // put exponent
    f.putsU8(fltVal >> 8); // put mantissa 2
    f.putsU8(((fltVal >> 24) & 128) | ((fltVal >> 16) & 127)); // put sign+mantissa 1
  }
}

static sInt doShift(sU32 v,sInt shift)
{
  while(shift>0)
    v<<=1,shift--;

  while(shift<0)
    v>>=1,shift++;

  return v;
}

static void putToterFloat(fr::stream& f, const sF32 v)
{
  sU32 value;
  sU16 dest;
  sInt esrc,edst;

  value = *(sU32 *) &v;
  esrc = ((value>>23) & 255) - 128;
  edst = (esrc < -16) ? -16 : (esrc > 15) ? 15 : esrc;

  dest = ((value>>16) & 32768) // sign
    | ((edst+16)<<10)
    | (doShift(value>>13,edst-esrc) & 1023);

  f.putsU16(dest);
}

static sU32 relocatePointer(sU32 fileOffset, const sU8* image)
{
  // fileOffset is relative to start of file. what we need is a final relocated address, though.
  // get PE header and find the section that contains the pointer, then apply its virtual base address.

  const IMAGE_DOS_HEADER* doshdr = (const IMAGE_DOS_HEADER*) image;
  const IMAGE_NT_HEADERS* nthdrs = (const IMAGE_NT_HEADERS*) (image + doshdr->e_lfanew);
  const IMAGE_SECTION_HEADER* sections = (const IMAGE_SECTION_HEADER*) (image + doshdr->e_lfanew + sizeof(IMAGE_NT_HEADERS));
  sBool found = sFALSE;

  for (sU32 i = 0; i < nthdrs->FileHeader.NumberOfSections; i++)
  {
    sS32 relOffset = fileOffset - sections[i].PointerToRawData;

    if (relOffset >= 0 && relOffset < sections[i].SizeOfRawData) // inside the section?
    {
      found = sTRUE;
      fileOffset = relOffset + sections[i].VirtualAddress + nthdrs->OptionalHeader.ImageBase; // calc the new offset
      break; // and stop
    }
  }

  if (!found)
    throwExportException("Data pointer relocation failed (makedemo.dat broken)");

  return fileOffset;
}

// ---- frGraphExporter::privateData

struct frGraphExporter::privateData
{
  typedef std::vector<sU32>           dwordVector;
  typedef dwordVector::iterator       dwordVectorIt;
  typedef dwordVector::const_iterator dwordVectorCIt;

  dwordVector   opExportSet;
  dwordVector   clipExportSet;

  dwordVector   curveList;

  sInt          opTypeStats[256];
  sInt          opTypeSizes[256];
  sInt          opTypeRemap[256];

	sInt					curveStats[12];
	sInt					clipStats[8];
  
  std::map<sU32, sU32>  opRemap;

  privateData()
  {
    cleanup();
  }

  void cleanup()
  {
    opExportSet.clear();
    clipExportSet.clear();
    curveList.clear();
    opRemap.clear();

    for (sInt i = 0; i < 256; i++)
    {
      opTypeStats[i] = 0;
      opTypeSizes[i] = 0;
    }

		for (sInt i = 0; i < 12; i++)
			curveStats[i] = 0;

		for (sInt i = 0; i < 8; i++)
			clipStats[i] = 0;
  }

  // ---- tool functions

  sBool opInExportSet(sU32 id)
  {
		return std::find(opExportSet.begin(), opExportSet.end(), id) != opExportSet.end();
  }

  sInt findClip(const frAnimationClip* clip)
  {
    sU32 cldw = (sU32) clip;

		dwordVectorCIt it(std::find(clipExportSet.begin(), clipExportSet.end(), cldw));
		return (it != clipExportSet.end()) ? it - clipExportSet.begin() : -1;
  }

  sBool clipInExportSet(const frAnimationClip* clip)
  {
    return findClip(clip) != -1;
  }

  sU32 eliminateNop(sU32 id)
  {
    id &= 0x7fffffff;

    while (id && g_graph->m_ops[id].def->isNop)
      id = g_graph->m_ops[id].inputs[0].id & 0x7fffffff;

    return id;
  }

  void addToOpExportSet(sU32 id)
  {
    // add the op to the export set if it's not there.
    if (!opInExportSet(id))
    {
      opTypeStats[g_graph->m_ops[id].def->ID]++;

      opExportSet.push_back(id);
      opRemap[id] = opExportSet.size() - 1;
    }
  }

  void addToClipExportSet(const frAnimationClip* clip)
  {
    // add the clip to the export set if it's not there.
    if (!clipInExportSet(clip))
      clipExportSet.push_back((sU32) clip);
  }

  sInt findCurve(sU32 id, sU32 curveID)
  {
		for (sInt x = 0; x < curveList.size(); x += 2)
		{
			if (curveList[x] == id && curveList[x + 1] == curveID)
				return x / 2;
		}

		return -1;
  }

  void addCurve(sU32 id, sU32 curveID)
  {
    sInt x = findCurve(id, curveID);

    if (x < 0) // only add if the element wasn't found!
    {
			curveList.push_back(id);
			curveList.push_back(curveID);
    }
  }

  void putLink(fr::stream& out, sU32 linkOp, sU32 currentOp)
  {
    const sU32 linkedOp = eliminateNop(linkOp);
    if (!linkedOp && linkOp)
      throwExportException("Link missing in export set for operator %d\n", currentOp);

    FRASSERT(!linkedOp || opInExportSet(linkedOp));

    if (linkedOp)
    {
      sU32 linkRef = opRemap[linkedOp];
      if (linkRef >= currentOp)
        throwExportException("Graph is not correctly topologically sorted, either we have cycles or the export code is buggy! Send the file to ryg immediately.");

      putSmallInt(out, currentOp - linkRef);
    }
    else
      putSmallInt(out, 0);
  }

  // ---- actual export code

  void recurseBuildRootSet(const frAnimationClip* startClip)
  {
    for (frAnimationClip::elementListCIt it = startClip->m_elements.begin(); it != startClip->m_elements.end(); ++it)
    {
      if (it->type == 1) // subclip, simply recurse
        recurseBuildRootSet(g_graph->m_clips->getClip(it->id));
      else // fcurve. check whether this is for a compositing op or not.
      {
        FRASSERT(startClip->isSimple()); // only simple clips may refer to. curves.

        frOpGraph::opMapIt oit = g_graph->m_ops.find(it->id); // get the animated op

        if (oit == g_graph->m_ops.end() || oit->second.def->type != 2) // not existant or not a compositing op?
          continue; // skip it

        addToOpExportSet(it->id);
      }
    }
  }

  void recurseFinishOpExportSet(sU32 startOp) // includes the dependencies for a given op and recurses
  {
    // if the op is already in the export set, all its children are too and we can stop here.
    if (!startOp || opInExportSet(startOp))
      return;

    // add the ops' inputs.
    frOpGraph::frOperator& op = g_graph->m_ops[startOp];
    sInt nIn = 0;
    for (sInt i = 0; i < op.nInputs; i++)
    {
      if ((op.inputs[i].id & 0x80000000) || nIn < op.def->nInputs)
      {
        recurseFinishOpExportSet(eliminateNop(op.inputs[i].id & 0x7fffffff));
        if (!(op.inputs[i].id & 0x80000000))
          nIn++;
      }
    }

    // then add this op to the export set.
    addToOpExportSet(startOp);
  }

  void finishOpExportSet() // builds the complete list of exported ops based on the root set.
  {
    // copy root set
    const sInt rootSetSize = opExportSet.size();
    sU32* rootSet = new sU32[rootSetSize];
    for (sInt i = 0; i < rootSetSize; i++)
      rootSet[i] = opExportSet[i];

    opExportSet.clear();

    // add all ops from the root set to the export set (including all ops they depend on)
    for (sInt i = 0; i < rootSetSize; i++)
      recurseFinishOpExportSet(rootSet[i]);

    // free the temporary copy of the root set
    delete[] rootSet;
  }

  sInt recurseFinishCurveExportSet(const frAnimationClip* startClip)
  {
    if (!startClip || clipInExportSet(startClip))
      return 1;

    sInt numRefs = 0;

    // add the subclips
    for (frAnimationClip::elementListCIt it = startClip->m_elements.begin(); it != startClip->m_elements.end(); ++it)
    {
      if (it->type == 1) // subclip
        numRefs += recurseFinishCurveExportSet(g_graph->m_clips->getClip(it->id));
      else
      {
        if (opInExportSet(it->id))
        {
          numRefs++;
          addCurve(it->id, it->curveID); // add the curve to the export
        }
      }
    }

    if (numRefs) // only "pure" clips get exported, simple clips are encoded as curve refs!
      addToClipExportSet(startClip);

    return numRefs;
  }

  void buildExportSet() // builds the export set (all relevant operators)
  {
    const frAnimationClip* rootClip = g_graph->m_clips->findClipByName("Timeline");
    if (!rootClip)
      throwExportException("No 'Timeline' clip present!");

    recurseBuildRootSet(rootClip);

    if (!opExportSet.size())
      throwExportException("Root export set empty (there's nothing gonna happen in the intro).\nDid you forget to add & animate a Camera?");

    // from the root set, build the complete set of operators that's gonna be used
    finishOpExportSet();

    // we now have a topologically sorted operator export set in opExportSet. great.
    // build the clips export list and we're done.
    recurseFinishCurveExportSet(rootClip);
  }

  void writeOpHeaders(fr::stream& out) // writes the output headers 
  {
    putSmallInt(out, opExportSet.size());

    // build operator ID remap table (and write the headers)
    sInt opTypeRemapCounter = 0;
    for (sInt i = 0; i < 256; i++)
    {
      opTypeRemap[i] = opTypeRemapCounter;
      if (opTypeStats[i])
      {
        out.putsU8(i);
        opTypeRemapCounter++;
      }
    }

    out.putsU8(0xff); // end mark
  }

  void writeOpConnections(fr::stream& out) // writes the op types & connections
  {
    // write the type bytes and connections for all ops.
    for (sInt i = 0; i < opExportSet.size(); i++)
    {
      const sS32 startWritePtr = out.tell();

      frOpGraph::frOperator& op = g_graph->m_ops[opExportSet[i]];
      sU8 typeByte = opTypeRemap[op.def->ID];

      sInt nInputs = 0;
      for (sInt in = 0; in < op.nInputs; in++)
      {
        if (!(op.inputs[in].id & 0x80000000))
          nInputs++;
      }

      if (nInputs < op.def->minInputs)
        throwExportException("Operator %d has %d inputs and needs at minimum %d!", op.opID, nInputs, op.def->minInputs);

      if (nInputs > op.def->nInputs)
        nInputs = op.
				def->nInputs;

      if (nInputs == 1 && opRemap[eliminateNop(op.plg->getInID(0))] == i - 1) // linked?
        out.putsU8(typeByte | 0x80);
      else
      {
        out.putsU8(typeByte);

        if (op.def->nInputs != op.def->minInputs) // write number of inputs if the count is variable.
          out.putsU8(nInputs);

        // write the individual input links
        for (sInt ind = 0; ind < nInputs; ind++)
        {
          // uses the plugins connections this time
          sU32 linkOp = opRemap[eliminateNop(op.plg->getInID(ind))];
          FRASSERT(linkOp < i);

          putSmallInt(out, i - linkOp - 1);
        }
      }

			for (sInt jj = 0; jj < nInputs; jj++)
				sU32 linkOp = opRemap[eliminateNop(op.plg->getInID(jj))];

      // write the param links
      for (sInt ind = op.def->nInputs; ind < op.plg->getNTotalInputs(); ind++)
        putLink(out, op.plg->getInID(ind), i);

      if (nInputs == 0 && op.def->ID < 0x80) // texture generator?
      {
        sU8 sizeByte = 0;
        frTexturePlugin* plg = (frTexturePlugin*) op.plg;
        sInt i;

        for (i = 8; i < plg->targetWidth; i <<= 1, sizeByte++);
        for (i = 8; i < plg->targetHeight; i <<= 1, sizeByte += 8);

        out.putsU8(sizeByte);
      }

      opTypeSizes[op.def->ID] += out.tell() - startWritePtr;
    }
  }

  void writeOpData(fr::stream& out, frGraphExporter& exp) // write op data (wow.)
  {
    fr::streamMTmp tmpStreams[256];
    
    // init streams
    for (sInt type = 0; type < 256; type++) // ops are ordered by type (to increase coherency in data layout)
    {
      if (!opTypeStats[type]) // if that type isn't used, skip the open.
        continue;

      tmpStreams[type].open();
    }

    // store individual ops
    for (sInt i = 0; i < opExportSet.size(); i++)
    {
      frOpGraph::frOperator& op = g_graph->m_ops[opExportSet[i]];
      
      const sS32 startWritePtr = tmpStreams[op.def->ID].tell();
      op.plg->exportTo(tmpStreams[op.def->ID], exp);

      opTypeSizes[op.def->ID] += tmpStreams[op.def->ID].tell() - startWritePtr;
    }

    // write combined output
    for (sInt type = 0; type < 256; type++)
    {
      if (!opTypeStats[type])
        continue;

      out.copy(tmpStreams[type][0]);
      tmpStreams[type].close();
    }
  }

  void writeCurves(fr::stream& out) // does what the name says...
  {
		const sS32 curveStartOffs = out.tell();

    putSmallInt(out, curveList.size() / 2);

    for (sInt crvi = 0; crvi < curveList.size(); crvi += 2)
    {
      const sU32 opID = curveList[crvi + 0];
      const sU32 curveID = curveList[crvi + 1];

      frCurveContainer::opCurves& crv = g_graph->m_curves->getOperatorCurves(opID);
      const fr::fCurveGroup* curveGroup = crv.getCurve(curveID);

      if (!opInExportSet(opID))
        throwExportException("Operator in curve set that's not in export set. Send the file to ryg immediately.");

      if (opRemap.find(opID) == opRemap.end())
        throwExportException("Operator in export set, but not in remap table! Send the file to ryg immediately.");

			out.putsU16(opRemap[opID]);

      FRASSERT(crv.getAnimIndex(curveID) < 64);
      out.putsU8(crv.getAnimIndex(curveID) | (curveGroup->getNChannels() << 6)); // write anim index+# of channels

      for (sInt i = 0; i < curveGroup->getNChannels(); i++) // for each channel...
      {
				const sS32 chanStartOffs = out.tell();

        const fr::fCurve* curve = curveGroup->getCurve(i);
        const sInt nKeys = curve->getNKeys();

        sF32 cMin, cMax; // determine value range for curve

        for (sInt j = 0; j < nKeys; j++)
        {
          sF32 val = curve->getKey(j)->value;
          if (!j || val < cMin) cMin = val;
          if (!j || val > cMax) cMax = val;
        }

				const sBool firstIsLast = fabs(curve->getKey(0)->value - curve->getKey(nKeys - 1)->value) < 1e-3f;

				if (nKeys == 2 && firstIsLast)
					putSmallInt(out, 0); // force curve type to constant in this case.
				else
					putSmallInt(out, curve->getType() + (firstIsLast ? 0 : 4) + (nKeys - 2) * 8); // put curve type/number of keys

        fr::streamMTmp loDeltaStream;
        loDeltaStream.open();

        sU32 oldFrame = 0;
        for (sInt j = 0; j < nKeys; j++) // new format: first, store all frame deltas
        {
          sS32 frame = curve->getKey(j)->frame >> 3;
          sS32 delta = frame - oldFrame;
          
          if (delta >= 65536)
            throwExportException("Keys too skewed in curve %d/%d (nothing happened in 65 secs)", opID, curveID);
          else if (delta < 0 || (delta != 0 && j == 0))
            throwExportException("Curve %d/%d seriously damaged", opID, curveID);

          if (j)
          {
						out.putsU8(delta >> 8);
						loDeltaStream.putsU8(delta & 0xff);
          }

          oldFrame = frame;
        }

        out.copy(loDeltaStream[0]);
        loDeltaStream.close();

        if (nKeys >= 6000000)
        {
          cMin = adjustFloat(cMin, sFALSE);
          cMax = adjustFloat((cMax - cMin) / 65535.0f, sTRUE);

          putToterFloat(out,cMin);
          putToterFloat(out,cMax);
          /*putPackedFloatNoRound(out, cMin);
          putPackedFloatNoRound(out, cMax);*/

          cMax = 1.0f / cMax;

					for (sInt j = 0; j < nKeys - (firstIsLast ? 1 : 0); j++) // store key values
          {
            sS32 val = (curve->getKey(j)->value - cMin) * cMax;
            val = (val < 0) ? 0 : (val > 65535) ? 65535 : val;
            out.putsU16(val);
          }

					curveStats[0]++;
					curveStats[1] += out.tell() - chanStartOffs;
        }
        else
        {
					for (sInt j = 0; j < nKeys - (firstIsLast ? 1 : 0); j++) // store key values
            putToterFloat(out, curve->getKey(j)->value);
						//putPackedFloatNoRound(out, curve->getKey(j)->value);

					curveStats[2]++;
					curveStats[3] += out.tell() - chanStartOffs;
        }

				if (nKeys == 2)
				{
					if (firstIsLast)
						curveStats[9]++; // practically redundant curves
					else
						curveStats[8]++; // very small curves
				}

				curveStats[4]++;
				curveStats[5] += out.tell() - chanStartOffs;
      }
    }

		// finish curve stats
		curveStats[6] = curveList.size() / 2;
		curveStats[7] = out.tell() - curveStartOffs;

		// write the curves out seperately
		fr::streamF ctemp("curves.dat", fr::streamF::wr | fr::streamF::cr);
		out.seek(curveStartOffs);
		ctemp.copy(out, curveStats[7]);
	}

  void writeClips(fr::stream& out) // again, does just what the name says.
  {
		sS32 clipStartOffs = out.tell();

    putSmallInt(out, clipExportSet.size()); // write number of clips

    fr::streamMTmp clipDataStream;
    clipDataStream.open();

		sInt onePastMaxCurve = 0;

    for (sInt i = 0; i < clipExportSet.size(); i++)
    {
      const frAnimationClip* clip = (const frAnimationClip*) clipExportSet[i];

      if (clip->m_elements.size() > 255)
        throwExportException("Clip '%s' has more than 255 elements (tell ryg to increase the limit)", (const sChar*) clip->getName());

      out.putsU8(clip->m_elements.size());

			sInt curRelStart = 0;

      for (frAnimationClip::elementListCIt eit = clip->m_elements.begin(); eit != clip->m_elements.end(); ++eit) // for each element
      {
				sInt inRelStart = eit->relStart / 8;

				if (inRelStart >= curRelStart)
					clipDataStream.putsU32((inRelStart - curRelStart) * 2); // write relative start offset
				else
					clipDataStream.putsU32(inRelStart * 2 + 1); // write reset offs.

				curRelStart = inRelStart;

        // calc id
        sInt id = 0;

        if (eit->type == 0) // curve ref
        {
          const sInt refCurve = findCurve(eit->id, eit->curveID);
          if (refCurve < 0)
            throwExportException("Trying to write reference to non-existant curve (send the file to ryg)");

					if (refCurve > onePastMaxCurve)
						throwExportException("Curve export set not correctly sorted (send the file to ryg) %d vs %d", refCurve, onePastMaxCurve);

          id = (onePastMaxCurve - refCurve) * 2 + 0;
					
					if (refCurve == onePastMaxCurve)
						onePastMaxCurve++;
        }
        else // normal clip, write clip ref
        {
          const sInt refClip = findClip(g_graph->m_clips->getClip(eit->id));
          if (refClip < 0)
            throwExportException("Clip export set damaged (send the file to ryg)");

          if (refClip >= i)
            throwExportException("Clip export set not correctly topologically sorted (send the file to ryg)");

          id = (i - 1 - refClip) * 2 + 1;
        }

        if (id >= 65536)
          throwExportException("Clip/Curve ID limit exceeded (tell ryg to increase it)");

        clipDataStream.putsU16(id);
      }
    }

    for (sInt co = 0; co < 6; co++) // interleave actual clip data for output
    {
      for (sInt ofs = co; ofs < clipDataStream.size(); ofs += 6)
        out.putsU8(clipDataStream[ofs].getsU8());
    }

    clipDataStream.close();

		clipStats[0] = clipExportSet.size();
		clipStats[1] = out.tell() - clipStartOffs;

		clipStats[7] = clipStartOffs;
  }

  void writeTunes(fr::stream& out, sU32 loaderTuneOffs)
  {
    // first write the main tune...
    out.write(g_graph->m_tuneBuf, g_graph->m_tuneSize);

    sU32 loaderTuneBytes = 0;

    // ...and then try to do the same for the loader tune. first, check if it's existant.
    fr::streamF loaderTuneStream;
    if (loaderTuneStream.open(g_graph->m_loaderTuneName))
    {
      // it exists. good. now get its size and load it, then close the source file.
      const sU32 loaderTuneSize = loaderTuneStream.size();
      sU8* loaderTuneBuf = new sU8[loaderTuneSize];
      loaderTuneStream.read(loaderTuneBuf, loaderTuneSize);
      loaderTuneStream.close();

      // convert it to the current v2m format...
      sInt loaderTuneSizeFinal;
      sU8* loaderTuneBufFinal;
      ConvertV2M(loaderTuneBuf, loaderTuneSize, &loaderTuneBufFinal, &loaderTuneSizeFinal);

      // delete old format data.
      delete[] loaderTuneBuf;

      // append the new format data, if it exists
      if (loaderTuneBufFinal)
      {
        const sU32 tuneOffset = out.tell();
        out.write(loaderTuneBufFinal, loaderTuneSizeFinal);
        delete[] loaderTuneBufFinal;

        loaderTuneBytes = loaderTuneSizeFinal;

        // and write the loader tune ptr
        out.seek(loaderTuneOffs);
        out.putsU32(tuneOffset);
        out.seekend(0);
      }
    }
  }

  void linkDemo(fr::stream& out, sU32 loaderTuneOffs)
  {
    // step 1: read viewer exe
    TCHAR pbuf[256];
    GetModuleFileName(0, pbuf, 256);
    if (strrchr(pbuf, '\\'))
      strrchr(pbuf, '\\')[1] = 0;
    else
      pbuf[0] = 0;

    TCHAR fname[256];
    lstrcpy(fname, pbuf);
    lstrcat(fname, "MakeDemo.dat");

    fr::streamF viewer;
    if (!viewer.open(fname, fr::streamF::rd | fr::streamF::ex))
      throwExportException("Make demo overlays are missing");

    sU32 viewerSize = viewer.size();
    sU8* viewerBuffer = new sU8[viewerSize];
    viewer.read(viewerBuffer, viewerSize);
    viewer.close();

    // step 2: find data insert mark
    const sChar insertMark[] = "{data insert mark here}";
    const sInt insertLen = strlen(insertMark);
    sInt insertPos;

    for (insertPos = 0; insertPos < viewerSize - insertLen; insertPos++)
    {
      if (!memcmp(viewerBuffer + insertPos, insertMark, insertLen))
        break;
    }

    if (insertPos >= viewerSize - insertLen)
      throwExportException("Make demo overlays are invalid");

    const sU32 insertSize = *((sU32 *) (viewerBuffer + insertPos + insertLen));
    if (out.size() > insertSize)
      throwExportException("Make demo overlay dataspace too small");

    const sBool viewerIsDebug = viewerBuffer[insertPos + insertLen + 4];

    // step 3: copy data
    out.seek(0);
    out.read(viewerBuffer + insertPos, out.size());

    // step 4: loader tune ptr needs to be relocated now, but only if its nonzero
    sU32 *loaderTunePtr = (sU32 *) (viewerBuffer + insertPos + loaderTuneOffs);
    if (*loaderTunePtr)
      *loaderTunePtr = relocatePointer(*loaderTunePtr + insertPos, viewerBuffer); // relocate the loader tune pointer

    // step 5: write file
    lstrcpy(fname, pbuf);
    lstrcat(fname, "demo.exe");

    if (!viewer.open(fname, fr::streamF::wr | fr::streamF::cr))
      throwExportException("Demo executable open failed");

    viewer.write(viewerBuffer, viewerSize);
    viewer.close();

    delete[] viewerBuffer;

    // step 6: pack it & execute
    /*if (!viewerIsDebug && spawnlp(_P_WAIT, "frp", "frp", "--rausch", fname, 0) == -1)
      throwExportException("Demo executable pack failed");*/
    if(!viewerIsDebug && spawnlp(_P_WAIT, "kkrunchy", "kkrunchy", "--best", fname, 0) == -1)
      throwExportException("Demo executable pack failed");

    if (!viewer.open(fname, fr::streamF::rd | fr::streamF::ex))
      throwExportException("Demo executable reopen failed");

    sInt outSize = viewer.size();
    viewer.close();

    TCHAR msgBuf[512];
    wsprintf(msgBuf, "Size: %d bytes. Execute?", outSize);
    if (::MessageBox(g_winMap.Get(ID_MAIN_VIEW), msgBuf, "RG2", MB_ICONQUESTION|MB_YESNO) == IDYES)
      spawnl(_P_NOWAIT, fname, fname, 0);

    // dump data output for further analysis :)
    fr::streamF exported("export.dat", fr::streamF::wr | fr::streamF::cr);
    exported.copy(out[0]);
    exported.close();
  }

  void printStats() // prints the oh-so-famous export statistics
  {
    fr::debugOut("op export statistics:\n\n");
    fr::debugOut("Name                     Count   Bytes   Average\n");
    fr::debugOut("------------------------------------------------\n");

		sInt totalCount = 0, totalSize = 0;

    for (sInt i = 0; i < 256; i++)
    {
      if (opTypeStats[i])
        fr::debugOut("%-22s %7d %7d %9.2f\n", frGetPluginByID(i)->name, opTypeStats[i], opTypeSizes[i], 1.0f * opTypeSizes[i] / opTypeStats[i]);

			totalCount += opTypeStats[i];
			totalSize += opTypeSizes[i];
    }

		fr::debugOut("------------------------------------------------\n");
    fr::debugOut("%-22s %7d %7d %9.2f\n", "Total", totalCount, totalSize, 1.0f * totalSize / totalCount);

		fr::debugOut("\n%d curve groups with %d bytes, %.2f avg\n", curveStats[6], curveStats[7], 1.0f * curveStats[7] / curveStats[6]);
		fr::debugOut("  %d big curves (%.2f%%) with %d bytes, %.2f avg\n", curveStats[0], 100.0f * curveStats[0] / curveStats[4],
			curveStats[1], 1.0f * curveStats[1] / curveStats[0]);
		fr::debugOut("  %d small curves (%.2f%%) with %d bytes, %.2f avg\n", curveStats[2], 100.0f * curveStats[2] / curveStats[4],
			curveStats[3], 1.0f * curveStats[3] / curveStats[2]);
		fr::debugOut("    %d very small (%.2f%%)\n", curveStats[8], 100.0f * curveStats[8] / curveStats[2]);
		fr::debugOut("    %d redundant (%.2f%%)\n", curveStats[9], 100.0f * curveStats[9] / curveStats[2]);

		fr::debugOut("\n%d clips with %d bytes, %.2f avg (fpos %d)\n", clipStats[0], clipStats[1], 1.0f * clipStats[1] / clipStats[0],
			clipStats[7]);
  }
};

// ---- frGraphExporter

frGraphExporter::frGraphExporter()
{
  d = new privateData;
}

frGraphExporter::~frGraphExporter()
{
  delete d;
}

void frGraphExporter::makeDemo(sInt xRes, sInt yRes)
{
  try
  {
    g_graph->m_curves->applyAllBasePoses();

    // build the export set
    d->buildExportSet();

    // start writing the output stream.
    fr::streamMTmp outputStream;
    outputStream.open();

    outputStream.putsInt(xRes); // x res
    outputStream.putsInt(yRes); // y res
    const sU32 loaderTunePos = outputStream.tell();
    outputStream.putsU32(0); // dummy loader tune offset

    d->writeOpHeaders(outputStream); // writes the op headers (id + count)
    d->writeOpConnections(outputStream); // writes the op connections.
    d->writeOpData(outputStream, *this); // writes the data.

    fr::streamF dump("ops.dat",fr::streamF::wr|fr::streamF::cr);
    dump.copy(outputStream[0]);
    dump.close();

    d->writeCurves(outputStream); // the curves...
    d->writeClips(outputStream);  // and the clips.

    // primary export is done now, here comes the bonus rounds
    d->writeTunes(outputStream, loaderTunePos); // the tunes.

    // write export data...
    fr::streamF outData("export.dat",fr::streamF::cr|fr::streamF::wr);
    outputStream.seek(0);
    outData.copy(outputStream);
    outData.close();
    outputStream.seek(0);

    // print some stats and link the demo
    d->printStats();
    d->linkDemo(outputStream, loaderTunePos); // do the linking and write output file

    // and that's it!
  }
  catch (exportException& e)
  {
    ::MessageBox(g_winMap.Get(ID_MAIN_VIEW), e.getDescription(), "RG2 Export", MB_ICONERROR|MB_OK);
  }
}
