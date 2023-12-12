#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Math/Vec2.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

class plGeometry;
struct plMsgExtractRenderData;
struct plMsgBuildStaticMesh;
struct plMsgExtractGeometry;
class plHeightfieldComponent;
class plMeshResourceDescriptor;

using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plImageDataResourceHandle = plTypedResourceHandle<class plImageDataResource>;

class PLASMA_GAMEENGINE_DLL plHeightfieldComponentManager : public plComponentManager<plHeightfieldComponent, plBlockStorageType::Compact>
{
public:
  plHeightfieldComponentManager(plWorld* pWorld);
  ~plHeightfieldComponentManager();

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);
  void AddToUpdateList(plHeightfieldComponent* pComponent);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  plDeque<plComponentHandle> m_ComponentsToUpdate;
};

/// \brief This component utilizes a greyscale image to generate an elevation mesh, which is typically used for simple terrain
///
/// The component always creates a mesh for rendering, which uses a single material.
/// For different layers of grass, dirt, etc. the material can combine multiple textures and a mask.
///
/// If the "GenerateCollision" property is set, the component also generates a static collision mesh during scene export.
class PLASMA_GAMEENGINE_DLL plHeightfieldComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plHeightfieldComponent, plRenderComponent, plHeightfieldComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent
protected:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // plHeightfieldComponent

public:
  plHeightfieldComponent();
  ~plHeightfieldComponent();

  plVec2 GetHalfExtents() const { return m_vHalfExtents; } // [ property ]
  void SetHalfExtents(plVec2 value);                       // [ property ]

  float GetHeight() const { return m_fHeight; } // [ property ]
  void SetHeight(float value);                  // [ property ]

  plVec2 GetTexCoordOffset() const { return m_vTexCoordOffset; } // [ property ]
  void SetTexCoordOffset(plVec2 value);                          // [ property ]

  plVec2 GetTexCoordScale() const { return m_vTexCoordScale; } // [ property ]
  void SetTexCoordScale(plVec2 value);                         // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  void SetMaterial(const plMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  plMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void SetHeightfieldFile(const char* szFile); // [ property ]
  const char* GetHeightfieldFile() const;      // [ property ]

  void SetHeightfield(const plImageDataResourceHandle& hResource);
  plImageDataResourceHandle GetHeightfield() const { return m_hHeightfield; }

  plVec2U32 GetTesselation() const { return m_vTesselation; } // [ property ]
  void SetTesselation(plVec2U32 value);                       // [ property ]

  void SetGenerateCollision(bool b);                                 // [ property ]
  bool GetGenerateCollision() const { return m_bGenerateCollision; } // [ property ]

  plVec2U32 GetColMeshTesselation() const { return m_vColMeshTesselation; } // [ property ]
  void SetColMeshTesselation(plVec2U32 value);                              // [ property ]

  void SetIncludeInNavmesh(bool b);                                // [ property ]
  bool GetIncludeInNavmesh() const { return m_bIncludeInNavmesh; } // [ property ]

protected:
  void OnBuildStaticMesh(plMsgBuildStaticMesh& msg) const;    // [ msg handler ]
  void OnMsgExtractGeometry(plMsgExtractGeometry& msg) const; // [ msg handler ]

  void InvalidateMesh();
  void BuildGeometry(plGeometry& geom) const;
  plResult BuildMeshDescriptor(plMeshResourceDescriptor& desc) const;

  template <typename ResourceType>
  plTypedResourceHandle<ResourceType> GenerateMesh() const;

  plUInt32 m_uiHeightfieldChangeCounter = 0;
  plImageDataResourceHandle m_hHeightfield;
  plMaterialResourceHandle m_hMaterial;

  plVec2 m_vHalfExtents = plVec2(100.0f);
  float m_fHeight = 50.0f;

  plVec2 m_vTexCoordOffset = plVec2::ZeroVector();
  plVec2 m_vTexCoordScale = plVec2(1);

  plVec2U32 m_vTesselation = plVec2U32(128);
  plVec2U32 m_vColMeshTesselation = plVec2U32(64);

  bool m_bGenerateCollision = true;
  bool m_bIncludeInNavmesh = true;

  plMeshResourceHandle m_hMesh;
};
