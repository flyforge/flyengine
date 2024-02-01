#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgUpdateLocalBounds;

using plBlackboardTemplateResourceHandle = plTypedResourceHandle<class plBlackboardTemplateResource>;

/// \brief A volume component can hold generic values either from a blackboard template or set directly on the component.
///
/// The values can be sampled with an plVolumeSampler and then used for things like e.g. post-processing, reverb etc.
/// They can also be used to represent knowledge in a scene, like e.g. smell or threat, and can be detected by an plSensorComponent and then processed by AI.
class PL_GAMEENGINE_DLL plVolumeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plVolumeComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plVolumeComponent

public:
  plVolumeComponent();
  ~plVolumeComponent();

  /// \brief Sets the blackboard template file to use.
  void SetTemplateFile(const char* szFile); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  /// \brief Sets the blackboard template to use.
  void SetTemplate(const plBlackboardTemplateResourceHandle& hResource);
  plBlackboardTemplateResourceHandle GetTemplate() const { return m_hTemplateResource; }

  /// \brief In case two volumes overlap, the one with a higher sort order value has precedence.
  void SetSortOrder(float fOrder);                    // [ property ]
  float GetSortOrder() const { return m_fSortOrder; } // [ property ]

  /// \brief Sets the spatial category under which this volume can be detected.
  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

  /// \brief Adds or replaces a value with a given name.
  void SetValue(const plHashedString& sName, const plVariant& value); // [ scriptable ]
  plVariant GetValue(plTempHashedString sName) const                  // [ scriptable ]
  {
    plVariant v;
    m_Values.TryGetValue(sName, v);
    return v;
  }

protected:
  const plRangeView<const plString&, plUInt32> Reflection_GetKeys() const;
  bool Reflection_GetValue(const char* szName, plVariant& value) const;
  void Reflection_InsertValue(const char* szName, const plVariant& value);
  void Reflection_RemoveValue(const char* szName);

  void InitializeFromTemplate();
  void ReloadTemplate();
  void RemoveReloadFunction();

  plBlackboardTemplateResourceHandle m_hTemplateResource;
  plHashTable<plHashedString, plVariant> m_Values;
  plSmallArray<plHashedString, 1> m_OverwrittenValues; // only used in editor
  float m_fSortOrder = 0.0f;
  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;
  bool m_bReloadFunctionAdded = false;
};

//////////////////////////////////////////////////////////////////////////

using plVolumeSphereComponentManager = plComponentManager<class plVolumeSphereComponent, plBlockStorageType::Compact>;

/// \brief A sphere implementation of the plVolumeComponent
class PL_GAMEENGINE_DLL plVolumeSphereComponent : public plVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plVolumeSphereComponent, plVolumeComponent, plVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plVolumeComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plVolumeSphereComponent

public:
  plVolumeSphereComponent();
  ~plVolumeSphereComponent();

  void SetRadius(float fRadius);
  float GetRadius() const { return m_fRadius; }

  /// \brief Values above 1 make the sphere influence drop off more rapidly, below 1 more slowly.
  void SetFalloff(float fFalloff);
  float GetFalloff() const { return m_fFalloff; }

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;

  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using plVolumeBoxComponentManager = plComponentManager<class plVolumeBoxComponent, plBlockStorageType::Compact>;

/// \brief A box implementation of the plVolumeComponent
class PL_GAMEENGINE_DLL plVolumeBoxComponent : public plVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plVolumeBoxComponent, plVolumeComponent, plVolumeBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plVolumeComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plVolumeBoxComponent

public:
  plVolumeBoxComponent();
  ~plVolumeBoxComponent();

  /// \brief Sets the size of the box.
  void SetExtents(const plVec3& vExtents);
  const plVec3& GetExtents() const { return m_vExtents; }

  /// \brief Values above 1 make the box influence drop off more rapidly, below 1 more slowly.
  ///
  /// Falloff is per cardinal axis.
  void SetFalloff(const plVec3& vFalloff);
  const plVec3& GetFalloff() const { return m_vFalloff; }

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;

  plVec3 m_vExtents = plVec3(10.0f);
  plVec3 m_vFalloff = plVec3(0.5f);
};
