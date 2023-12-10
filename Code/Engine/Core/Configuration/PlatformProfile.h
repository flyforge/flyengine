#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plChunkStreamWriter;
class plChunkStreamReader;

struct plProfileTargetPlatform
{
  enum Enum
  {
    PC,
    UWP,
    Android,

    Default = PC
  };

  typedef plUInt8 StorageType;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plProfileTargetPlatform);

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class PLASMA_CORE_DLL plProfileConfigData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProfileConfigData, plReflectedClass);

public:
  plProfileConfigData();
  ~plProfileConfigData();

  virtual void SaveRuntimeData(plChunkStreamWriter& stream) const;
  virtual void LoadRuntimeData(plChunkStreamReader& stream);
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_CORE_DLL plPlatformProfile : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPlatformProfile, plReflectedClass);

public:
  plPlatformProfile();
  ~plPlatformProfile();

  plStringView GetConfigName() const { return m_sName; }

  void Clear();
  void AddMissingConfigs();

  template <typename TYPE>
  const TYPE* GetTypeConfig() const
  {
    return static_cast<const TYPE*>(GetTypeConfig(plGetStaticRTTI<TYPE>()));
  }

  template <typename TYPE>
  TYPE* GetTypeConfig()
  {
    return static_cast<TYPE*>(GetTypeConfig(plGetStaticRTTI<TYPE>()));
  }

  const plProfileConfigData* GetTypeConfig(const plRTTI* pRtti) const;
  plProfileConfigData* GetTypeConfig(const plRTTI* pRtti);

  plResult SaveForRuntime(plStringView sFile) const;
  plResult LoadForRuntime(plStringView sFile);

  plString m_sName;
  plEnum<plProfileTargetPlatform> m_TargetPlatform;
  plDynamicArray<plProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
