#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

struct plMsgExtractGeometry;
struct plMsgBuildStaticMesh;
struct plResourceEvent;
class plKrautRenderData;
class plAbstractObjectNode;

using plKrautTreeResourceHandle = plTypedResourceHandle<class plKrautTreeResource>;
using plKrautGeneratorResourceHandle = plTypedResourceHandle<class plKrautGeneratorResource>;

class PLASMA_KRAUTPLUGIN_DLL plKrautTreeComponentManager : public plComponentManager<class plKrautTreeComponent, plBlockStorageType::Compact>
{
public:
  typedef plComponentManager<plKrautTreeComponent, plBlockStorageType::Compact> SUPER;

  plKrautTreeComponentManager(plWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const plWorldModule::UpdateContext& context);
  void EnqueueUpdate(plComponentHandle hComponent);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  mutable plMutex m_Mutex;
  plDeque<plComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class PLASMA_KRAUTPLUGIN_DLL plKrautTreeComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plKrautTreeComponent, plRenderComponent, plKrautTreeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

protected:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // plKrautTreeComponent

public:
  plKrautTreeComponent();
  ~plKrautTreeComponent();

  // see plKrautTreeComponent::GetLocalBounds for details
  static const int s_iLocalBoundsScale = 3;

  void OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) const;
  void OnBuildStaticMesh(plMsgBuildStaticMesh& ref_msg) const;

  void SetKrautFile(const char* szFile); // [ property ]
  const char* GetKrautFile() const;      // [ property ]

  void SetVariationIndex(plUInt16 uiIndex); // [ property ]
  plUInt16 GetVariationIndex() const;       // [ property ]

  void SetCustomRandomSeed(plUInt16 uiSeed); // [ property ]
  plUInt16 GetCustomRandomSeed() const;      // [ property ]

  void SetKrautGeneratorResource(const plKrautGeneratorResourceHandle& hTree);
  const plKrautGeneratorResourceHandle& GetKrautGeneratorResource() const { return m_hKrautGenerator; }

private:
  plResult CreateGeometry(plGeometry& geo, plWorldGeoExtractionUtil::ExtractionMode mode) const;
  void EnsureTreeIsGenerated();

  plUInt16 m_uiVariationIndex = 0xFFFF;
  plUInt16 m_uiCustomRandomSeed = 0xFFFF;
  plKrautTreeResourceHandle m_hKrautTree;
  plKrautGeneratorResourceHandle m_hKrautGenerator;

  void ComputeWind() const;

  mutable plUInt64 m_uiLastWindUpdate = (plUInt64)-1;
  mutable plVec3 m_vWindSpringPos;
  mutable plVec3 m_vWindSpringVel;
};
