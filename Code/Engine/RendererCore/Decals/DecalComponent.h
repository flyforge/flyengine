#pragma once

#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/VarianceTypes.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

class plAbstractObjectNode;
struct plMsgComponentInternalTrigger;
struct plMsgOnlyApplyToObject;
struct plMsgSetColor;

class PL_RENDERERCORE_DLL plDecalComponentManager final : public plComponentManager<class plDecalComponent, plBlockStorageType::Compact>
{
public:
  plDecalComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class plDecalComponent;
  plDecalAtlasResourceHandle m_hDecalAtlas;
};

class PL_RENDERERCORE_DLL plDecalRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalRenderData, plRenderData);

public:
  plUInt32 m_uiApplyOnlyToId;
  plUInt32 m_uiFlags;
  plUInt32 m_uiAngleFadeParams;

  plColorLinearUB m_BaseColor;
  plColorLinear16f m_EmissiveColor;

  plUInt32 m_uiBaseColorAtlasScale;
  plUInt32 m_uiBaseColorAtlasOffset;

  plUInt32 m_uiNormalAtlasScale;
  plUInt32 m_uiNormalAtlasOffset;

  plUInt32 m_uiORMAtlasScale;
  plUInt32 m_uiORMAtlasOffset;
};

/// \brief Projects a decal texture onto geometry within a box volume.
///
/// This is used to add dirt, scratches, signs and other surface imperfections to geometry.
/// The component uses a box shape to define the position and volume and projection direction.
/// This can be set up in a level to add detail, but it can also be used by dynamic effects such as bullet hits,
/// to visualize the impact. To add variety a prefab may use different textures and vary in size.
class PL_RENDERERCORE_DLL plDecalComponent final : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plDecalComponent, plRenderComponent, plDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

protected:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // plDecalComponent

public:
  plDecalComponent();
  ~plDecalComponent();

  /// \brief Sets the extents of the box inside which to project the decal.
  void SetExtents(const plVec3& value); // [ property ]
  const plVec3& GetExtents() const;     // [ property ]

  /// \brief The size variance defines how much the size may randomly deviate, such that the decals look different.
  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  /// \brief An additional tint color for the decal.
  void SetColor(plColorGammaUB color); // [ property ]
  plColorGammaUB GetColor() const;     // [ property ]

  /// \brief An additional emissive color to make the decal glow.
  void SetEmissiveColor(plColor color); // [ property ]
  plColor GetEmissiveColor() const;     // [ property ]

  /// \brief At which angle between the decal orientation and the surface it is projected onto, to start fading the decal out.
  void SetInnerFadeAngle(plAngle fadeAngle); // [ property ]
  plAngle GetInnerFadeAngle() const;         // [ property ]

  /// \brief At which angle between the decal orientation and the surface it is projected onto, to fully fade out the decal.
  void SetOuterFadeAngle(plAngle fadeAngle); // [ property ]
  plAngle GetOuterFadeAngle() const;         // [ property ]

  /// \brief If multiple decals are in the same location, this allows to tweak which one is rendered on top.
  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  /// \brief Whether the decal projection should use a kind of three-way texture mapping to wrap the image around curved geometry.
  void SetWrapAround(bool bWrapAround); // [ property ]
  bool GetWrapAround() const;           // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  /// \brief Sets the decal resource to use. If more than one is set, a random one will be chosen.
  ///
  /// Indices that are written to will be created on-demand.
  void SetDecal(plUInt32 uiIndex, const plDecalResourceHandle& hResource); // [ property ]
  const plDecalResourceHandle& GetDecal(plUInt32 uiIndex) const;           // [ property ]

  /// If non-zero, the decal fades out after this time and then vanishes.
  plVarianceTypeTime m_FadeOutDelay; // [ property ]

  /// How much time the fade out takes.
  plTime m_FadeOutDuration; // [ property ]

  /// If fade-out is used, the decal may delete itself afterwards.
  plEnum<plOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  /// \brief Sets the cardinal axis into which the decal projection should be.
  void SetProjectionAxis(plEnum<plBasisAxis> projectionAxis); // [ property ]
  plEnum<plBasisAxis> GetProjectionAxis() const;              // [ property ]

  /// \brief If set, the decal only appears on the given object.
  ///
  /// This is typically used to limit the decal to a single dynamic object, such that damage decals don't project
  /// onto static geometry and other objects.
  void SetApplyOnlyTo(plGameObjectHandle hObject);
  plGameObjectHandle GetApplyOnlyTo() const;

  plUInt32 DecalFile_GetCount() const;                         // [ property ]
  const char* DecalFile_Get(plUInt32 uiIndex) const;           // [ property ]
  void DecalFile_Set(plUInt32 uiIndex, const char* szFile);    // [ property ]
  void DecalFile_Insert(plUInt32 uiIndex, const char* szFile); // [ property ]
  void DecalFile_Remove(plUInt32 uiIndex);                     // [ property ]


protected:
  void SetApplyToRef(const char* szReference); // [ property ]
  void UpdateApplyTo();

  void OnTriggered(plMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(plMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(plMsgSetColor& msg);

  plVec3 m_vExtents = plVec3(1.0f);
  float m_fSizeVariance = 0;
  plColorGammaUB m_Color = plColor::White;
  plColor m_EmissiveColor = plColor::Black;
  plAngle m_InnerFadeAngle = plAngle::MakeFromDegree(50.0f);
  plAngle m_OuterFadeAngle = plAngle::MakeFromDegree(80.0f);
  float m_fSortOrder = 0;
  bool m_bWrapAround = false;
  bool m_bMapNormalToGeometry = false;
  plUInt8 m_uiRandomDecalIdx = 0xFF;
  plEnum<plBasisAxis> m_ProjectionAxis;
  plHybridArray<plDecalResourceHandle, 1> m_Decals;

  plGameObjectHandle m_hApplyOnlyToObject;
  plUInt32 m_uiApplyOnlyToId = 0;

  plTime m_StartFadeOutTime;
  plUInt32 m_uiInternalSortKey = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
