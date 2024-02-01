#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

using plSkyBoxComponentManager = plComponentManager<class plSkyBoxComponent, plBlockStorageType::Compact>;
using plTextureCubeResourceHandle = plTypedResourceHandle<class plTextureCubeResource>;

/// \brief Adds a static image of a sky to the scene.
///
/// This is used to fill the scene background with a picture of a sky.
/// The sky image comes from a cubemap texture.
///
/// Position and scale of the game object are irrelevant, the sky always appears behind all other objects.
/// The rotation, however, is used to rotate the sky image.
class PL_RENDERERCORE_DLL plSkyBoxComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSkyBoxComponent, plRenderComponent, plSkyBoxComponentManager);

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

  /// \brief Changes the brightness of the sky image. Mainly useful when an HDR skybox is used.
  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  /// \brief For HDR skyboxes this should stay off. For LDR skyboxes, enabling this will improve brightness and contrast.
  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  /// \brief Enables that fog is applied to the sky. See SetVirtualDistance().
  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  /// \brief If fog is enabled, the virtual distance is used to determine how foggy the sky should be.
  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]

  void SetCubeMap(const plTextureCubeResourceHandle& hCubeMap);
  const plTextureCubeResourceHandle& GetCubeMap() const;

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias = 0.0f;
  float m_fVirtualDistance = 1000.0f;
  bool m_bInverseTonemap = false;
  bool m_bUseFog = true;

  plTextureCubeResourceHandle m_hCubeMap;

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hCubeMapMaterial;
};
