#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct plMsgSetColor;
struct plInstanceData;

class PLASMA_RENDERERCORE_DLL plMeshRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshRenderData, plRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;
  plColor m_Color = plColor::White;
  plVec3 m_Wind = plVec3::ZeroVector();

  plUInt32 m_uiSubMeshIndex : 30;
  plUInt32 m_uiFlipWinding : 1;
  plUInt32 m_uiUniformScale : 1;

  plUInt32 m_uiUniqueID = 0;

protected:
  PLASMA_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(plUInt32 uiAdditionalBatchData)
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

struct PLASMA_RENDERERCORE_DLL plMsgSetMeshMaterial : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgSetMeshMaterial, plMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  plMaterialResourceHandle m_hMaterial;
  plUInt32 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(plStreamWriter& inout_stream) const override;
  virtual void Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion) override;
};

class PLASMA_RENDERERCORE_DLL plMeshComponentBase : public plRenderComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plMeshComponentBase, plRenderComponent);

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

  void SetMesh(const plMeshResourceHandle& hMesh);
  PLASMA_ALWAYS_INLINE const plMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void SetMaterial(plUInt32 uiIndex, const plMaterialResourceHandle& hMaterial);
  plMaterialResourceHandle GetMaterial(plUInt32 uiIndex) const;

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetColor(const plColor& color); // [ property ]
  const plColor& GetColor() const;     // [ property ]

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

private:
  void ComputeWind() const;

  mutable plUInt64 m_uiLastWindUpdate = (plUInt64)-1;
  mutable plVec3 m_vWindSpringPos;
  mutable plVec3 m_vWindSpringVel;
};
