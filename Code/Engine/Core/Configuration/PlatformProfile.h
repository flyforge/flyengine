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

  using StorageType = plUInt8;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plProfileTargetPlatform);

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class PL_CORE_DLL plProfileConfigData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plProfileConfigData, plReflectedClass);

public:
  plProfileConfigData();
  ~plProfileConfigData();

  virtual void SaveRuntimeData(plChunkStreamWriter& inout_stream) const;
  virtual void LoadRuntimeData(plChunkStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

class PL_CORE_DLL plPlatformProfile : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plPlatformProfile, plReflectedClass);

public:
  plPlatformProfile();
  ~plPlatformProfile();

  void SetConfigName(plStringView sName) { m_sName = sName; }
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

  /// \brief Returns a number indicating when the profile counter changed last. By storing and comparing this value, other code can update their state if necessary.
  plUInt32 GetLastModificationCounter() const { return m_uiLastModificationCounter; }

private:
  plUInt32 m_uiLastModificationCounter = 0;
  plString m_sName;
  plEnum<plProfileTargetPlatform> m_TargetPlatform;
  plDynamicArray<plProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
