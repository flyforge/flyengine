#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;

typedef plComponentManager<class plPlayerStartPointComponent, plBlockStorageType::Compact> plPlayerStartPointComponentManager;

class PLASMA_GAMEENGINE_DLL plPlayerStartPointComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plPlayerStartPointComponent, plComponent, plPlayerStartPointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plPlayerStartPointComponent

public:
  plPlayerStartPointComponent();
  ~plPlayerStartPointComponent();

  void SetPlayerPrefabFile(const char* szFile); // [ property ]
  const char* GetPlayerPrefabFile() const;      // [ property ]

  void SetPlayerPrefab(const plPrefabResourceHandle& hPrefab); // [ property ]
  const plPrefabResourceHandle& GetPlayerPrefab() const;       // [ property ]

  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ] (exposed parameter)

  plArrayMap<plHashedString, plVariant> m_Parameters;

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number

protected:
  plPrefabResourceHandle m_hPlayerPrefab;
};
