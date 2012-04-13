// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "_types.hpp"
#include "materialdirect.hpp"
#include "shadercodegen.hpp"

/****************************************************************************/

sMaterialDirect::sMaterialDirect(const sU32 *vShader,const sU32 *pShader,
  const sU32 *states,const sU32 *codegenData)
{
  sShaderCodeGen *gen = new sShaderCodeGen;

  sU32 *vs = gen->GenCode(vShader,codegenData);
  sU32 *ps = gen->GenCode(pShader,codegenData);
  Setup = sSystem->MtrlAddSetup(states,vs,ps);
  delete[] vs;
  delete[] ps;
  delete gen;

  Instance = sMaterialInstance::Null;
}

sMaterialDirect::~sMaterialDirect()
{
  SetInstance(sMaterialInstance::Null);
  if(Setup != sINVALID)
    sSystem->MtrlRemSetup(Setup);
}

void sMaterialDirect::SetInstance(const sMaterialInstance &inst)
{
  for(sInt i=0;i<inst.NumTextures;i++)
    if(inst.Textures[i] != sINVALID)
      sSystem->AddRefTexture(inst.Textures[i]);

  for(sInt i=0;i<Instance.NumTextures;i++)
    if(Instance.Textures[i] != sINVALID)
      sSystem->RemTexture(Instance.Textures[i]);

  Instance = inst;
}

void sMaterialDirect::Set(const sMaterialEnv &env)
{
  // set wvp matrix (and that's all we do in terms of constant processing)
  sMatrix *vc = (sMatrix *) Instance.VSConstants;

  if(Instance.NumVSConstants >= 4)
  {
    vc[0].Mul4(env.ModelSpace,sSystem->LastViewProject);    // c0-c3: wvp
    vc[0].Trans4();
  }

  sSystem->MtrlSetSetup(Setup);
  sSystem->MtrlSetInstance(Instance);
}

/****************************************************************************/
