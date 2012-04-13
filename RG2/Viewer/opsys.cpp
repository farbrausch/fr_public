// This code is in the public domain. See LICENSE for details.

#include "opsys.h"
#include "types.h"
#include "texture.h"
#include "geometry.h"
#include "opdata.h"
#include "directx.h"
#include "debug.h"

// ---- animation stuff

struct curve
{
  sInt    type;
  sInt    nKeys;
  sInt    len;

  sU32*   data;
};

struct curveGroup
{
  sInt        nChannels;
  sInt        len;
  frOperator* op;
  sInt        parm;
  curve*      channel;
};

#pragma pack (push, 1)
struct clipElem
{
	sU32			relStart;
  sU16			id;
};
#pragma pack (pop)

struct clip
{
  sInt      nElements;
  sInt      len;
  clipElem* elements;
};

struct curveEvalElem
{
  sInt      curveNum;
  sInt      time;
  sInt      evalTime;
};

static curveGroup* curves = 0;
static clip* clips = 0;
static sInt nCurves, nClips;
static curveEvalElem* evalBuf;
static sInt evalInd;

// ---- frOperator

extern frOperatorDef operatorDefs[];
static sU32 globalRevCounter = 0;
static sInt nOperators = 0;

frOperator::frOperator()
  : revision(0), input(0), nInputs(0), dirty(sTRUE), opType(0), refNum(0)
{
}

frOperator::~frOperator()
{
}

void frOperator::doProcess()
{
}

void frOperator::process()
{
  // just checks revisions and doesn't recursively process (it simply isn't necessary the way it works here)
  for (sInt i = 0; i < nInputs; i++)
  {
    if (input[i] && revision <= input[i]->revision)
      dirty = sTRUE;
  }

  if (dirty)
  {
    dirty = sFALSE;
    doProcess();
    revision = ++globalRevCounter;
  }
}

void frOperator::setAnim(sInt parm, const sF32* val)
{
}

const sU8* frOperator::readData(const sU8* strm)
{
  param = strm;
  strm += def->dataBytes;
  return strm;
}

void frOperator::freeData()
{
}

// ---- frTextureOperator

frTextureOperator::frTextureOperator()
  : data(0), tex(0), texRev(0)
{
}

frTextureOperator::~frTextureOperator()
{
	if (tex)
	{
		delete tex;
		tex = 0;
	}
}

void frTextureOperator::create(sU32 w, sU32 h)
{
  data = new frBitmap(w, h);
}

void frTextureOperator::freeData()
{
  if (data)
  {
    delete data;
    data = 0;
  }
}

void frTextureOperator::updateTexture()
{
  if (texRev != revision)
  {
    if (!tex)
      tex = new fvs::texture(data->w, data->h, fvs::texfRGB | fvs::texfAlpha | fvs::texfMipMaps | fvs::texfWrap);

    tex->upload(data->data);
    texRev = revision;
  }
}

// ---- frGeometryOperator

frGeometryOperator::frGeometryOperator()
{
  data = new frModel;
}

void frGeometryOperator::freeData()
{
  if (data)
  {
    delete data;
    data = 0;
  }
}

// ---- frCompositingOperator

void frCompositingOperator::doPaint()
{
}

void frCompositingOperator::paint()
{
  for (sInt i = 0; i < nInputs - def->nLinks; i++)
    ((frCompositingOperator*) input[i])->paint();

  doPaint();
}

// ---- operator tree

static frOperator** operatorTree = 0;

const sU8* opsInitialize(const sU8* strm)
{
  nOperators = getSmallInt(strm);
  operatorTree = new frOperator*[nOperators];

  const sU8* opIDs = strm;
  while (*strm++ != 0xff); // skip op ids

  // read op types & connections (and create the tree)
  for (sInt i = 0; i < nOperators; i++)
  {
    const sU8 typeByte = *strm++;
    const sU8 opID = opIDs[typeByte & 0x7f];

    // find the definition for the type
    const frOperatorDef* def = operatorDefs;
    while (def->id != opID)
      def++;

    frOperator* thisOp = def->createFunc();
    operatorTree[i] = thisOp;

    thisOp->def = def;
    sInt inCount = 1;

    if (!(typeByte & 0x80)) // not 1-input direct link op?
    {
      if (def->nInputs == 0xff) // variable number of inputs?
        inCount = *strm++; // read the number of ins
      else
        inCount = def->nInputs; // use default number of inputs
    }

		FRDASSERT(def->nInputs == 0xff || inCount >= def->nInputs);

    thisOp->nInputs = inCount + def->nLinks;
    thisOp->input = new frOperator*[inCount + def->nLinks];
    for (sInt in = 0; in < inCount; in++) // get the links
    {
      sInt delta = (typeByte & 0x80) ? 0 : getSmallInt(strm);

      // get input operator
      frOperator* inputOp = operatorTree[i - 1 - delta];

      // make the connection
      thisOp->input[in] = inputOp;

      // increment ref count on input
      inputOp->refNum++;
    }

    for (sInt lnk = 0; lnk < def->nLinks; lnk++) // get the hard links
    {
      sInt delta = getSmallInt(strm);

      // get the input operator
      frOperator* inputOp = delta ? operatorTree[i - delta] : 0;

      // make the connection
      thisOp->input[inCount + lnk] = inputOp;

      // increment ref count on input
      if (inputOp)
        inputOp->refNum++;
    }

    if (opID < 0x80 && !inCount) // texture operator without input (generator), so read the size byte and create it!
    {
      sU8 sizeByte = *strm++;
      static_cast<frTextureOperator*>(thisOp)->create(8 << (sizeByte & 7), 8 << (sizeByte >> 3));
    }
  }

  // tree and ops are created and linked now, start reading the operator data
  // (ordered by op types)
  for (sInt type = 0; opIDs[type] != 0xff; type++)
  {
    const sU8 opID = opIDs[type];

    for (sInt j = 0; j < nOperators; j++)
    {
      frOperator* op = operatorTree[j];

      if (op->def->id == opID) // read data if id matches
        strm = op->readData(strm);
    }
  }

  // read the curves
  nCurves = getSmallInt(strm);
  curves = new curveGroup[nCurves];
  for (sInt i = 0; i < nCurves; i++)
  {
    // get animated op and number of channels
    sInt curvLen = 0;

    frOperator* animOp = operatorTree[*((sU16 *) strm)];
    strm += 2;
    animOp->anim = sTRUE; // set animation flag

    const sInt nChan = strm[0] >> 6;

    // prepare the curve
    curves[i].op = animOp;
    curves[i].parm = strm[0] & 63;
    curves[i].nChannels = nChan;
    curves[i].channel = new curve[nChan];
    strm++;

    // read the channels
    for (sInt j = 0; j < nChan; j++)
    {
      // read type byte
			const sInt typeNum = getSmallInt(strm);
      sInt nKeys = (typeNum >> 3) + 1; // really +2, but nKeys-1 is quite convenient actually.
			sInt firstIsLast = ((typeNum >> 2) & 1) ^ 1;

      // prepare channel
      curve* thisChan = curves[i].channel + j;
      thisChan->type = typeNum & 3;
      thisChan->data = new sU32[nKeys * 2 * 2];
      thisChan->nKeys = nKeys;

      sU32* outData = thisChan->data;

      // read the keyframe numbers
      sU32 time = 0;
      for (sInt k = 0; k < nKeys; k++)
      {
        outData[k * 2] = time;
        time += (strm[k] << 8) + strm[k + nKeys];
      }

      outData[nKeys * 2] = time;
      thisChan->len = time;
      if (time > curvLen)
        curvLen = time;

      strm += nKeys * 2;

      // read the keyframe values
      if (1/*nKeys < 5*/) // unpacked format
      {
        for (sInt k = 0; k <= nKeys - firstIsLast; k++)
          ((sF32 *) outData)[k * 2 + 1] = getToterFloat(strm);
      }
      /*else // ranged format
      {
        const sF32 min = getPackedFloat(strm);
        const sF32 range = getPackedFloat(strm);

        for (sInt k = 0; k <= nKeys - firstIsLast; k++)
        {
          const sU16 v = *((sU16 *) strm);
          strm += 2;

          ((sF32 *) outData)[k * 2 + 1] = v * range + min;
        }
      }*/

			if (firstIsLast)
				outData[nKeys * 2 + 1] = outData[1];
    }

    curves[i].len = curvLen;
  }

  // read the clips
  nClips = getSmallInt(strm);
  sInt clipSize = 0;

  clips = new clip[nClips];
  for (sInt i = 0; i < nClips; i++)
  {
    clips[i].elements = (clipElem*) (clipSize * sizeof(clipElem));
    clipSize += (clips[i].nElements = *strm++);
  }

  clipElem* clipData = new clipElem[clipSize];
  for (sInt i = 0; i < clipSize; i++)
  {
    for (sInt j = 0; j < sizeof(clipElem); j++)
      ((sU8 *) clipData)[i * sizeof(clipElem) + j] = strm[j * clipSize + i];
  }

  strm += clipSize * sizeof(clipElem);

  sU32 totalClipSize = clipSize;
	sInt curCurve = 0;

  for (sInt i = 0; i < nClips; i++)
  {
    // adjust element pointer
    clip* theClip = clips + i;
    theClip->elements = (clipElem*) ((sU32) theClip->elements + (sU32) clipData);

    // calculate clip length (using the fact that the clips are stored in topologically sorted order)
    sInt len = 0;
		sInt curStart = 0;

    for (sInt j = 0; j < theClip->nElements; j++)
    {
      // get end point of element
      clipElem* element = theClip->elements + j;
      sInt elemEnd;

			// adjust relative start
			if (element->relStart & 1)
				element->relStart >>= 1;
			else
				element->relStart = (element->relStart >> 1) + curStart;

			curStart = element->relStart;

      if (element->id & 1)
      {
				element->id = i * 2 - element->id;

        elemEnd = clips[element->id >> 1].len;
        totalClipSize += clips[element->id >> 1].nElements;
      }
      else
			{
				sInt curveDelta = element->id >> 1;
				element->id = (curCurve - curveDelta) * 2;
				if (!curveDelta)
					curCurve++;

        elemEnd = curves[element->id >> 1].len;
			}

      elemEnd += curStart;
      
      if (elemEnd > len)
        len = elemEnd;
    }

    theClip->len = len;
  }

  // allocate the evaluation buffer
  evalBuf = new curveEvalElem[totalClipSize];

  // distribute the animation flag
  for (sInt i = 0; i < nOperators; i++)
  {
    frOperator* theOp = operatorTree[i];

    for (sInt j = 0; j < theOp->nInputs; j++)
    {
      frOperator* in = theOp->input[j];
      if (in)
        theOp->anim |= in->anim;
    }
  }

  // that's it
  return strm;
}

void opsCalculate(void (__stdcall *poll)(sInt counter, sInt max))
{
  if (poll)
    poll(0, nOperators);

  for (sInt i = 0; i < nOperators; i++)
  {
    frOperator* op = operatorTree[i];
    frTextureOperator* texOp = static_cast<frTextureOperator*>(op);

    if (op->def->id < 0x80 && op->refNum && !texOp->getData()) // texture operator that wasn't created yet?
    {
      frBitmap* inData = texOp->getInputData(0);
      texOp->create(inData->w, inData->h); // create it with appropriate size
    }

    op->process();

    if (!op->anim)
    {
      for (sInt j = 0; j < op->nInputs; j++) // an op uses all of its inputs (per definition), so...
      {
        frOperator* in = op->input[j];

        if (in && --in->refNum == 0)
          in->freeData();
      }
    }

    if (poll)
      poll(i + 1, nOperators);
  }
}

void opsCleanup() // free all used memory
{
  for (sInt i = 0; i < nCurves; i++)
	{
		for (sInt j = 0; j < curves[i].nChannels; j++)
			delete[] curves[i].channel[j].data;

    delete[] curves[i].channel;
	}
  
  delete[] curves;

  if (clips)
    delete[] clips[0].elements;
  
  delete[] clips;
  delete[] evalBuf;

  for (sInt i = 0; i < nOperators; i++)
	{
		delete[] operatorTree[i]->input;
    operatorTree[i]->freeData();
    delete operatorTree[i];
	}

  delete[] operatorTree;
}

// ---- animation support

static void processCurve(const curve* crv, sF32* out, sInt frame)
{
  const sInt* fData = (const sInt*) crv->data;
  const sF32* vData = (const sF32*) (crv->data + 1);
  sInt nk1 = crv->nKeys;

  if (frame <= fData[0])
    *out = vData[0];
  else if (frame >= fData[nk1 * 2])
    *out = vData[nk1 * 2];
  else
  {
    sInt i = 0;

    while (frame > fData[2])
    {
      fData += 2;
      vData += 2;
      i++;
    }

    frame -= fData[0];

    const sF32 tdelta = fData[2] - fData[0];
    const sF32 d10 = vData[2] - vData[0];
    sF32 tf = frame / tdelta;
      
    if (crv->type == 0) // constant
      tf = 0.0f;

    if (crv->type != 2) // not catmull-rom?
      *out = vData[0] + d10 * tf; // lerp
    else // catmull-rom.
    {
      sF32 dd0, ds1;
        
      if (i == 0)
        dd0 = d10;
      else
        dd0 = 0.5f * (vData[2] - vData[-2]) / (fData[2] - fData[-2]);

      if (i + 1 == nk1)
        ds1 = d10;
      else
        ds1 = 0.5f * (vData[4] - vData[0]) / (fData[4] - fData[0]);

      const sF32 tt = tf * tf, ttt = tt * tf;
      const sF32 h2 = 3.0f * tt - 2.0f * ttt;

      *out = (1.0f - h2) * vData[0] + h2 * vData[2] + (ttt - 2.0f * tt + tf) * dd0 + (ttt - tt) * ds1;
    }
  }
}

static void processCurveGroup(sInt crv, sInt frame)
{
  sF32 output[3];
  curveGroup* theGroup = curves + crv;

  for (sInt i = 0; i < theGroup->nChannels; i++)
    processCurve(theGroup->channel + i, output + i, frame);
  
  frOperator* theOp = theGroup->op;
  theOp->setAnim(theGroup->parm, output);
  theOp->dirty = sTRUE;
}

static void processClip(sInt clp, sInt sFrame, sInt eFrame, sInt bFrame)
{
  clip* theClip = clips + clp;

  for (sInt i = 0; i < theClip->nElements; i++)
  {
    clipElem* element = theClip->elements + i;

    const sU32 cRelStart = element->relStart;
    sInt elemLen;

    if (element->id & 1)
      elemLen = clips[element->id >> 1].len;
    else
      elemLen = curves[element->id >> 1].len;

    sInt startFrame = sFrame - cRelStart;
    if (startFrame < 0)
      startFrame = 0;

    sInt endFrame = eFrame - cRelStart;
    if (endFrame > elemLen)
      endFrame = elemLen;

		const sInt subBaseFrame = bFrame + cRelStart;

    if (startFrame < endFrame)
    {
      if (element->id & 1)
        processClip(element->id >> 1, startFrame, endFrame, subBaseFrame);
      else
      {
        evalBuf[evalInd].curveNum = element->id >> 1;
        evalBuf[evalInd].evalTime = endFrame;
				evalBuf[evalInd].time = endFrame + subBaseFrame;
        evalInd++;
      }
    }
  }
}

// ---- animation main

sBool opsAnimate(sInt clp, sInt startFrame, sInt endFrame)
{
  // get clip and process it
  if (clp < 0)
    clp += nClips;

  evalInd = 0;
  processClip(clp, startFrame, endFrame, 0);

  // sort the eval buffer (this is a stupid bubblesort)
  for (sInt i = evalInd; i > 0; i--)
  {
    for (sInt j = 1; j < i; j++)
    {
      if (evalBuf[j - 1].time > evalBuf[j].time)
      {
        curveEvalElem t = evalBuf[j - 1];
        evalBuf[j - 1] = evalBuf[j];
        evalBuf[j] = t;
      }
    }
  }

  // then apply it
  for (sInt i = 0; i < evalInd; i++)
    processCurveGroup(evalBuf[i].curveNum, evalBuf[i].evalTime);

  // calc all ops
  for (sInt i = 0; i < nOperators; i++)
    operatorTree[i]->process();

  // and return
  return endFrame < clips[clp].len;
}
