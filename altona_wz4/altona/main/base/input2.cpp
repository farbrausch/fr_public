/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "base/input2.hpp"

/****************************************************************************/
/* Scheme class implementations                                             */
/****************************************************************************/

sInput2Scheme::~sInput2Scheme()
{
  sInput2Key* k;
  sFORALL(Bindings, k) {
    if (k) {
      sInput2Key* key = k->Next;
      while (key) {
        sInput2Key* key2 = key;
        key = key->Next;
        sDelete(key2);
      }
      sDelete(k);
    }
  }
}

/****************************************************************************/

void sInput2Scheme::Init(sInt capacity)
{
  sVERIFY2(Bindings.GetSize() == 0, L"sInput2Scheme::Init(): already initialized");
  Bindings.HintSize(capacity);
  Bindings.AddManyInit(capacity, sNULL);
  RetriggerTime.HintSize(capacity);
  RetriggerTime.AddMany(capacity);
  for (sInt i=0; i<capacity; i++)
    RetriggerTime[i] = -1;
  RetriggerRate.HintSize(capacity);
  RetriggerRate.AddManyInit(capacity, 0);
  RetriggerDelay.HintSize(capacity);
  RetriggerDelay.AddManyInit(capacity, 0);
  RetriggerPressed.HintSize(capacity);
  RetriggerPressed.AddManyInit(capacity, 0);
}

/****************************************************************************/

void sInput2Scheme::OnGameStep(sInt delta)
{
  // update retrigger timers
  for (sInt i=0; i<Bindings.GetCount(); i++) {
    if (RetriggerRate[i] && RetriggerTime[i] == 0 && RetriggerPressed[i])
      RetriggerTime[i] = RetriggerRate[i];

    sInput2Key* key = Bindings[i];
    sBool connected = sFALSE;
    while (key) {
      if (!(key->Device->GetStatusWord() & sINPUT2_STATUS_ERROR)) {
        connected = sTRUE;
        break;
      }
      key = key->Next;
    }

    if (RetriggerTime[i] > 0 && connected) {
      RetriggerTime[i] -= delta;
      if (RetriggerTime[i] < 0)
        RetriggerTime[i] = 0;
    } else {
      // disable retrigger
      RetriggerTime[i] = -1;
    }
  }
}

/****************************************************************************/

void sInput2Scheme::Bind(sInt id, const sInput2Key& key)
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Bind(): id out of range specified with sInput2Scheme::Init()");
  // silently ignore keys with null-devices. this may happen a lot when binding to controllers that are not available on the current platform
  if (!key.Device)
    return;

  sInput2Key* k = new sInput2Key(key.Device, key.KeyId, key.ThresholdAnalog, key.ThresholdDigitalLo, key.ThresholdDigitalHi);
  if (Bindings[id]) {
    // if a key already exists for this id, add it to the end of the list
    sInput2Key* next = Bindings[id];
    while (next->Next)
      next = next->Next;
    next->Next = k;
  } else {
    // if no key exists for this id, just put it into the slot
    Bindings[id] = k;
  }
}

/****************************************************************************/

void sInput2Scheme::Bind(sInt id, const sInput2Device* device, sInt keyId, sF32 thresholdAnalog, sF32 thresholdDigitalLo, sF32 thresholdDigitalHi)
{
  Bind(id, sInput2Key(device, keyId, thresholdAnalog, thresholdDigitalLo, thresholdDigitalHi));
}

/****************************************************************************/

void sInput2Scheme::Bind(sInt id, sInput2Key* key)
{
  if(!key || !key->Device)
    return;

  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Bind(): id out of range specified with sInput2Scheme::Init()");
  Unbind(id);

  // copy all keys
  sInput2Key* k = key;
  sInput2Key** k2 = &Bindings[id];
  while (k) {
    *k2 = new sInput2Key(*k);
    k = k->Next;
    k2 = &((*k2)->Next);
  }
}

/****************************************************************************/

void sInput2Scheme::SetRetrigger(sInt id, sInt rate, sInt delay)
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::SetRetrigger(): id out of range specified with sInput2Scheme::Init()");
  RetriggerRate[id]   =   sMax(rate,  0);
  RetriggerDelay[id]  =   sMax(delay, 0);
}

/****************************************************************************/

void sInput2Scheme::SetRetriggerPermanent(sInt id, sInt rate, sInt delay)
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::SetRetriggerPermanent(): id out of range specified with sInput2Scheme::Init()");
  RetriggerRate[id]   =   sMax(rate,  0);
  RetriggerDelay[id]  = - sMax(delay, 0);  // the sign codes the permanent regtriggering
  RetriggerTime[id]   =   sMax(delay, 0); 
}

/****************************************************************************/

void sInput2Scheme::Unbind(sInt id)
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Unbind(): id out of range specified with sInput2Scheme::Init()");

  sInput2Key* k = Bindings[id];
  if (k) {
    sInput2Key* key = k->Next;
    while (key) {
      sInput2Key* key2 = key;
      key = key->Next;
      sDelete(key2);
    }
    sDelete(k);
    Bindings[id] = sNULL;
  }
}

/****************************************************************************/

sInt sInput2Scheme::GetTimestamp() const
{
  sInt timestamp = 0;
  for (int i=0; i<Bindings.GetCount(); i++) {
    sInput2Key* key = Bindings[i];
    if (!key)
      continue;
    do {
      timestamp = sMax(timestamp, key->Device->GetTimestamp());
      key = key->Next;
    } while (key);
  }
  // return latest timestamp
  return timestamp;
}

/****************************************************************************/

sBool sInput2Scheme::Hold(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Hold(): id out of range specified with sInput2Scheme::Init()");

  sBool value = sFALSE;
  sInput2Key* key = Bindings[id];
  if (!key)
    return value;
  do {
    sF32 v = key->Device->GetAbs(key->KeyId);
    if (sFAbs(v) > key->ThresholdDigitalHi)
      value = sMax(value, v != 0.0f ? sTRUE : sFALSE);
    key = key->Next;
  } while (key);
  return value;
}

/****************************************************************************/
sInt sInput2Scheme::Pressed(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Pressed(): id out of range specified with sInput2Scheme::Init()");

  sInt value = 0;
  sF32 valuePositive = 0;
  sF32 valueNegative = 0;
  sF32 analog;
  sInput2Key* key = Bindings[id];
  if (!key)
    return value;
  do {
    value += key->Device->Pressed(key->KeyId, key->ThresholdDigitalHi);
    analog = key->Device->GetAbs(key->KeyId);
    if (key->ThresholdDigitalLo > 0 && analog > key->ThresholdDigitalLo)
      valuePositive += analog;
    if (key->ThresholdDigitalLo < 0 && analog < key->ThresholdDigitalLo)
      valueNegative += analog;
    key = key->Next;
  } while (key);

  // retrigger setup on actual keypress. set to delay time  
  if (value)
    RetriggerTime[id] = sAbs(RetriggerDelay[id]);
  // retriggering. retrigger if RetriggerTime == 0 and key is still pressed
  sBool permanentRetrigger = RetriggerDelay[id]<0;
  RetriggerPressed[id] = valuePositive || valueNegative || permanentRetrigger;
  if (RetriggerRate[id] && RetriggerTime[id] == 0 && RetriggerPressed[id])
    return (valuePositive || permanentRetrigger ? 1 : 0) + (valueNegative ? -1 : 0);
  return value;
}

/****************************************************************************/

sInt sInput2Scheme::Released(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Released(): id out of range specified with sInput2Scheme::Init()");

  sInt value = 0;
  sInput2Key* key = Bindings[id];
  if (!key)
    return value;
  do {
    value += key->Device->Released(key->KeyId, key->ThresholdDigitalLo);
    key = key->Next;
  } while (key);
  return value;
}

/****************************************************************************/

sF32 sInput2Scheme::Analog(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Analog(): id out of range specified with sInput2Scheme::Init()");

  sF32 valuePositive = 0.0f;
  sF32 valueNegative = 0.0f;
  sInput2Key* key = Bindings[id];
  if (!key)
    return valuePositive + valueNegative;
  do {
    sF32 v = key->Device->GetAbs(key->KeyId);
    v = sFAbs(v) > key->ThresholdAnalog ? v : 0.0f;
    if (v > 0.0f)
      valuePositive = sMax(valuePositive, v);
    else
      valueNegative = sMin(valueNegative, v);
    key = key->Next;
  } while (key);
  return valuePositive + valueNegative;
}

/****************************************************************************/

sF32 sInput2Scheme::Relative(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Relative(): id out of range specified with sInput2Scheme::Init()");

  sF32 value = 0;
  sInput2Key* key = Bindings[id];
  if (!key)
    return value;
  do {
    sF32 v = key->Device->GetAbs(key->KeyId);
    if (sFAbs(v) > key->ThresholdAnalog)
      value += key->Device->GetRel(key->KeyId);
    key = key->Next;
  } while (key);
  return value;
}

/****************************************************************************/

sVector2 sInput2Scheme::Coords(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Coords(): id out of range specified with sInput2Scheme::Init()");

  sVector2 valuePositive(0.0f, 0.0f);
  sVector2 valueNegative(0.0f, 0.0f);
  sInput2Key* key = Bindings[id];
  if (!key)
    return valuePositive + valueNegative;
  do {
    sVERIFY2(key->KeyId+1 < key->Device->KeyCount(), L"invalid access function was used with this keybinding");
    for (int j=0; j<2; j++) {
      sF32 v = key->Device->GetAbs(key->KeyId+j);
      if (v > 0.0f)
        valuePositive[j] = sMax(valuePositive[j], v);
      else
        valueNegative[j] = sMin(valueNegative[j], v);
    }
    key = key->Next;
  } while (key);
  return valuePositive + valueNegative;
}

/****************************************************************************/

sVector31 sInput2Scheme::Position(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Position(): id out of range specified with sInput2Scheme::Init()");

  sVector30 valuePositive(0.0f, 0.0f, 0.0f);
  sVector30 valueNegative(0.0f, 0.0f, 0.0f);
  sInput2Key* key = Bindings[id];
  if (!key)
    return sVector31(valuePositive + valueNegative);
  do {
    sVERIFY2(key->KeyId+2 < key->Device->KeyCount(), L"invalid access function was used with this keybinding");
    for (int j=0; j<3; j++) {
      sF32 v = key->Device->GetAbs(key->KeyId+j);
      if (v > 0.0f)
        valuePositive[j] = sMax(valuePositive[j], v);
      else
        valueNegative[j] = sMin(valueNegative[j], v);
    }
    key = key->Next;
  } while (key);
  return sVector31(valuePositive + valueNegative);
}

/****************************************************************************/

sVector30 sInput2Scheme::Normal(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Normal(): id out of range specified with sInput2Scheme::Init()");

  sVector30 valuePositive(0.0f, 0.0f, 0.0f);
  sVector30 valueNegative(0.0f, 0.0f, 0.0f);
  sInput2Key* key = Bindings[id];
  if (!key)
    return valuePositive + valueNegative;
  do {
    sVERIFY2(key->KeyId+2 < key->Device->KeyCount(), L"invalid access function was used with this keybinding");
    for (int j=0; j<3; j++) {
      sF32 v = key->Device->GetRel(key->KeyId+j);
      if (v > 0.0f)
        valuePositive[j] = sMax(valuePositive[j], v);
      else
        valueNegative[j] = sMin(valueNegative[j], v);
    }
    key = key->Next;
  } while (key);
  return valuePositive + valueNegative;
}

/****************************************************************************/

sQuaternion sInput2Scheme::Quaternion(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Quaternion(): id out of range specified with sInput2Scheme::Init()");

  sQuaternion valuePositive(0.0f, 0.0f, 0.0f, 0.0f);
  sQuaternion valueNegative(0.0f, 0.0f, 0.0f, 0.0f);
  sInput2Key* key = Bindings[id];
  if (!key)
    return valuePositive + valueNegative;
  do {
    sVERIFY2(key->KeyId+3 < key->Device->KeyCount(), L"invalid access function was used with this keybinding");
    sF32 qr = key->Device->GetAbs(key->KeyId+0);
    if (qr > 0.0f)
      valuePositive.r = sMax(valuePositive.r, qr);
    else
      valueNegative.r = sMin(valueNegative.r, qr);
    sF32 qi = key->Device->GetAbs(key->KeyId+1);
    if (qi > 0.0f)
      valuePositive.i = sMax(valuePositive.i, qi);
    else
      valueNegative.i = sMin(valueNegative.i, qi);
    sF32 qj = key->Device->GetAbs(key->KeyId+2);
    if (qj > 0.0f)
      valuePositive.j = sMax(valuePositive.j, qj);
    else
      valueNegative.j = sMin(valueNegative.j, qj);
    sF32 qk = key->Device->GetAbs(key->KeyId+3);
    if (qk > 0.0f)
      valuePositive.k = sMax(valuePositive.k, qk);
    else
      valueNegative.k = sMin(valueNegative.k, qk);
    key = key->Next;
  } while (key);
  return valuePositive + valueNegative;
}

/****************************************************************************/

sVector4 sInput2Scheme::Vector4(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::Vector4(): id out of range specified with sInput2Scheme::Init()");

  sVector4 valuePositive(0.0f, 0.0f, 0.0f, 0.0f);
  sVector4 valueNegative(0.0f, 0.0f, 0.0f, 0.0f);
  sInput2Key* key = Bindings[id];
  if (!key)
    return valuePositive + valueNegative;
  do {
    sVERIFY2(key->KeyId+3 < key->Device->KeyCount(), L"invalid access function was used with this keybinding");
    for (int j=0; j<4; j++) {
      sF32 v = key->Device->GetAbs(key->KeyId+j);
      if (v > 0.0f)
        valuePositive[j] = sMax(valuePositive[j], v);
      else
        valueNegative[j] = sMin(valueNegative[j], v);
    }
    key = key->Next;
  } while (key);
  return valuePositive + valueNegative;
}

/****************************************************************************/

const sInput2Device* sInput2Scheme::GetDevice(sInt id) const
{
  sVERIFY2(id >= 0 && id < Bindings.GetSize(), L"sInput2Scheme::GetDevice(): id out of range specified with sInput2Scheme::Init()");
  const sInput2Device* device = sNULL;
  sInput2Key* key = Bindings[id];
  if (!key)
    return device;
  do {
    sF32 v = key->Device->GetAbs(key->KeyId);
    if (sFAbs(v) > key->ThresholdDigitalHi && v != 0.0f) {
      device = key->Device;
      break;
    }
    key = key->Next;
  } while (key);
  return device;
}

/****************************************************************************/
/* Mapping class implementations                                            */
/****************************************************************************/

sInput2Mapping::sInput2Mapping() 
{ 
  MaxPlayers = 0; 
  MaxKeys = 0; 
}

/****************************************************************************/

sInput2Mapping::~sInput2Mapping() 
{ 
  Clear(); 
}

/****************************************************************************/

void sInput2Mapping::Init(sInt maxPlayers, sInt maxKeys) 
{ 
  MaxPlayers = maxPlayers; 
  MaxKeys = maxKeys; 
  Mapping.AddManyInit(MaxPlayers * MaxKeys, 0); 
}

/****************************************************************************/

void sInput2Mapping::Clear() 
{
  for (int i=0; i<MaxKeys; i++)
    for (int j=0; j<MaxPlayers; j++)
      Rem(i, j);
}

/****************************************************************************/

void sInput2Mapping::Rem(sInt player, sInt logicalKey)
{
  sVERIFY2(player >= 0 && player < MaxPlayers, L"sInput2Mapping::Clear(): player number out of range specified with sInput2Mapping::Init()");
  sVERIFY2(logicalKey >= 0 && logicalKey < MaxKeys, L"sInput2Mapping::Clear(): logical key number out of range specified with sInput2Mapping::Init()");
  sInt index = logicalKey * MaxPlayers + player;
  sInput2Key* k = Mapping[index];
  if (k) {
    sInput2Key* key = k->Next;
    while (key) {
      sInput2Key* key2 = key;
      key = key->Next;
      sDelete(key2);
    }
    sDelete(k);
    Mapping[index] = sNULL;
  }
}

/****************************************************************************/

void sInput2Mapping::Add(sInt player, sInt logicalKey, sInput2Key key)
{
  sVERIFY2(player >= 0 && player < MaxPlayers, L"sInput2Mapping::Set(): player number out of range specified with sInput2Mapping::Init()");
  sVERIFY2(logicalKey >= 0 && logicalKey < MaxKeys, L"sInput2Mapping::Set(): logical key number out of range specified with sInput2Mapping::Init()");

  sInt index = logicalKey * MaxPlayers + player;
  if (!key.Device)
    return;

  sInput2Key* k = new sInput2Key(key.Device, key.KeyId, key.ThresholdAnalog, key.ThresholdDigitalLo, key.ThresholdDigitalHi);
  if (Mapping[index]) {
    // if a key already exists for this id, add it to the end of the list
    sInput2Key* next = Mapping[index];
    while (next->Next)
      next = next->Next;
    next->Next = k;
  } else {
    // if no key exists for this id, just put it into the slot
    Mapping[index] = k;
  }

}

/****************************************************************************/

void sInput2Mapping::Add(sInt player, sInt logicalKey, const sInput2Device* device, sInt key, sF32 ThresholdAnalog, sF32 ThresholdDigital)
{
  Add(logicalKey, player, sInput2Key(device, key, ThresholdAnalog, ThresholdDigital));
}

/****************************************************************************/

sInput2Key* sInput2Mapping::Get(sInt player, sInt logicalKey) 
{ 
  sVERIFY2(player >= 0 && player < MaxPlayers, L"sInput2Mapping::Get(): player number out of range specified with sInput2Mapping::Init()");
  sVERIFY2(logicalKey >= 0 && logicalKey < MaxKeys, L"sInput2Mapping::Get(): logical key number out of range specified with sInput2Mapping::Init()");
  return Mapping[logicalKey * MaxPlayers + player]; 
}
