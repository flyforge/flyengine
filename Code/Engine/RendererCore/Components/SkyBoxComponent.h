#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef plComponentManager<class plSkyBoxComponent, plBlockStorageType::Compact> plSkyBoxComponentManager;
using plTextureCubeResourceHandle = plTypedResourceHandle<class plTextureCubeResource>;

class PLASMA_RENDERERCORE_DLL plSkyBoxComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSkyBoxComponent, plRenderComponent, plSkyBoxComponentManager);

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
  // plSkyBoxComponent

public:
  plSkyBoxComponent();
  ~plSkyBoxComponent();

  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]

  void SetCubeMap(const plTextureCubeResourceHandle& hCubeMap);
  const plTextureCubeResourceHandle& GetCubeMap() const;

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias = 64000.0f;
  float m_fVirtualDistance = 1000.0f;
  bool m_bInverseTonemap = false;
  bool m_bUseFog = true;

  plTextureCubeResourceHandle m_hCubeMap;

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hCubeMapMaterial;
};
