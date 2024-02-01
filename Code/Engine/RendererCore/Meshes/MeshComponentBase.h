#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;
struct plInstanceData;

class PL_RENDERERCORE_DLL plMeshRenderData : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plMeshRenderData, plRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;
  plColor m_Color = plColor::White;

  plUInt32 m_uiSubMeshIndex : 30;
  plUInt32 m_uiFlipWinding : 1;
  plUInt32 m_uiUniformScale : 1;

  plUInt32 m_uiUniqueID = 0;

protected:
  PL_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(plUInt32 uiAdditionalBatchData)
  {
    m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
    m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

    const plUInt32 uiMeshIDHash = plHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
    const plUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? plHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

    // Generate batch id from mesh, material and part index.
    plUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, m_uiSubMeshIndex, m_uiFlipWinding, uiAdditionalBatchData};
    m_uiBatchId = plHashingUtils::xxHash32(data, sizeof(data));

    // Sort by material and then by mesh
    m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
  }
};

/// \brief This message is used to replace the material on a mesh.
struct PL_RENDERERCORE_DLL plMsgSetMeshMaterial : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgSetMeshMaterial, plMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  /// The material to be used.
  plMaterialResourceHandle m_hMaterial;

  /// The slot on the mesh component where the material should be set.
  plUInt32 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(plStreamWriter& inout_stream) const override;
  virtual void Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion) override;
};

/// \brief Base class for components that render static or animated meshes.
class PL_RENDERERCORE_DLL plMeshComponentBase : public plRenderComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plMeshComponentBase, plRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderMeshComponent

public:
  plMeshComponentBase();
  ~plMeshComponentBase();

  /// \brief Changes which mesh to render.
  void SetMesh(const plMeshResourceHandle& hMesh);
  PL_ALWAYS_INLINE const plMeshResourceHandle& GetMesh() const { return m_hMesh; }

  /// \brief Sets the material that should be used for the sub-mesh with the given index.
  void SetMaterial(plUInt32 uiIndex, const plMaterialResourceHandle& hMaterial);
  plMaterialResourceHandle GetMaterial(plUInt32 uiIndex) const;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void SetColor(const plColor& color); // [ property ]
  const plColor& GetColor() const;     // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;       // [ property ]

  void OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(plMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  virtual plMeshRenderData* CreateRenderData() const;

  plUInt32 Materials_GetCount() const;                          // [ property ]
  const char* Materials_GetValue(plUInt32 uiIndex) const;       // [ property ]
  void Materials_SetValue(plUInt32 uiIndex, const char* value); // [ property ]
  void Materials_Insert(plUInt32 uiIndex, const char* value);   // [ property ]
  void Materials_Remove(plUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plMeshResourceHandle m_hMesh;
  plDynamicArray<plMaterialResourceHandle> m_Materials;
  plColor m_Color = plColor::White;
  float m_fSortingDepthOffset = 0.0f;
};
