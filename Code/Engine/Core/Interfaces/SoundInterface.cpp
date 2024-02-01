#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Foundation/Configuration/Singleton.h>

plResult plSoundInterface::PlaySound(plStringView sResourceID, const plTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (plSoundInterface* pSoundInterface = plSingletonRegistry::GetSingletonInstance<plSoundInterface>())
  {
    return pSoundInterface->OneShotSound(sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return PL_FAILURE;
}


