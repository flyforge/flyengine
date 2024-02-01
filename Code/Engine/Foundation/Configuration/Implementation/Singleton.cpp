#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

plMap<size_t, plSingletonRegistry::SingletonEntry> plSingletonRegistry::s_Singletons;

const plMap<size_t, plSingletonRegistry::SingletonEntry>& plSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}


