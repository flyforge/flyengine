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

class PLASMA_RENDERERCORE_DLL plDecalComponentManager final : public plComponentManager<class plDecalComponent, plBlockStorageType::Compact>
{
public:
  plDecalComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class plDecalComponent;
  plDecalAtlasResourceHandle m_hDecalAtlas;
};

class PLASMA_RENDERERCORE_DLL plDecalRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDecalRenderData, plRenderData);

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

class PLASMA_RENDERERCORE_DLL plDecalComponent final : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plDecalComponent, plRenderComponent, plDecalComponentManager);

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

  void SetExtents(const plVec3& value); // [ property ]
  const plVec3& GetExtents() const;     // [ property ]

  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  void SetColor(plColorGammaUB color); // [ property ]
  plColorGammaUB GetColor() const;     // [ property ]

  void SetEmissiveColor(plColor color); // [ property ]
  plColor GetEmissiveColor() const;     // [ property ]

  void SetInnerFadeAngle(plAngle fadeAngle);  // [ property ]
  plAngle GetInnerFadeAngle() const;          // [ property ]

  void SetOuterFadeAngle(plAngle fadeAngle);  // [ property ]
  plAngle GetOuterFadeAngle() const;          // [ property ]

  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  void SetWrapAround(bool bWrapAround); // [ property ]
  bool GetWrapAround() const;           // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  void SetDecal(plUInt32 uiIndex, const plDecalResourceHandle& hResource); // [ property ]
  const plDecalResourceHandle& GetDecal(plUInt32 uiIndex) const;           // [ property ]

  plVarianceTypeTime m_FadeOutDelay;                      // [ property ]
  plTime m_FadeOutDuration;                               // [ property ]
  plEnum<plOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  void SetProjectionAxis(plEnum<plBasisAxis> projectionAxis); // [ property ]
  plEnum<plBasisAxis> GetProjectionAxis() const;              // [ property ]

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
  plAngle m_InnerFadeAngle = plAngle::Degree(50.0f);
  plAngle m_OuterFadeAngle = plAngle::Degree(80.0f);
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
