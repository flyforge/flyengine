#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

struct plPhantomRttiManagerEvent;
class plExposedParameters;
struct plAssetCuratorEvent;

/// \brief Lazily converts plExposedParameters into phantom types.
/// Call GetExposedParametersType to create a type for a sub-asset ID.
class plExposedParametersTypeRegistry
{
  PL_DECLARE_SINGLETON(plExposedParametersTypeRegistry);

public:
  plExposedParametersTypeRegistry();
  ~plExposedParametersTypeRegistry();
  /// \brief Returns null if the curator can find the asset or if the asset
  /// does not have any plExposedParameters meta data.
  const plRTTI* GetExposedParametersType(const char* szResource);
  /// \brief All exposed parameter types derive from this.
  const plRTTI* GetExposedParametersBaseType() const { return m_pBaseType; }

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorFramework, ExposedParametersTypeRegistry);

  struct ParamData
  {
    ParamData()

      = default;

    plUuid m_SubAssetGuid;
    bool m_bUpToDate = true;
    const plRTTI* m_pType = nullptr;
  };
  void UpdateExposedParametersType(ParamData& data, const plExposedParameters& params);
  void AssetCuratorEventHandler(const plAssetCuratorEvent& e);
  void PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e);

  plMap<plUuid, ParamData> m_ShaderTypes;
  const plRTTI* m_pBaseType;
  ParamData* m_pAboutToBeRegistered = nullptr;
};
