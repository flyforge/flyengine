#pragma once

#include <Core/Collection/CollectionResource.h>
#include <Core/CoreDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using plCollectionComponentManager = plComponentManager<class plCollectionComponent, plBlockStorageType::Compact>;

/// \brief An plCollectionComponent references an plCollectionResource and triggers resource preloading when needed
///
/// Placing an plCollectionComponent in a scene or a model makes it possible to tell the engine to preload certain resources
/// that are likely to be needed soon.
///
/// If a deactivated plCollectionComponent is part of the scene, it will not trigger a preload, but will do so once
/// the component is activated.
class PL_CORE_DLL plCollectionComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plCollectionComponent, plComponent, plCollectionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent
public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plCollectionComponent
public:
  plCollectionComponent();
  ~plCollectionComponent();

  void SetCollectionFile(const char* szFile); // [ property ]
  const char* GetCollectionFile() const;      // [ property ]

  void SetCollection(const plCollectionResourceHandle& hPrefab);
  PL_ALWAYS_INLINE const plCollectionResourceHandle& GetCollection() const { return m_hCollection; }

protected:
  /// \brief Triggers the preload on the referenced plCollectionResource
  void InitiatePreload();

  bool m_bRegisterNames = false;
  plCollectionResourceHandle m_hCollection;
};
