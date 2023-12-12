#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef plComponentManager<class plVolumetricCloudsComponent, plBlockStorageType::Compact> plVolumetricCloudsComponentManager;

class PLASMA_RENDERERCORE_DLL plVolumetricCloudsComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plVolumetricCloudsComponent, plRenderComponent, plVolumetricCloudsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // plVolumetricCloudsComponent

public:
  plVolumetricCloudsComponent();
  ~plVolumetricCloudsComponent();

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void UpdateMaterials() const;

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;

  plTexture3DResourceHandle m_hNoiseLut;
  plTexture3DResourceHandle m_hDetailNoiseLut;
};
