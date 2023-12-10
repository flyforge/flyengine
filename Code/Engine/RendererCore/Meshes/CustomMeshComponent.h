#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Renderer.h>

using plDynamicMeshBufferResourceHandle = plTypedResourceHandle<class plDynamicMeshBufferResource>;
using plCustomMeshComponentManager = plComponentManager<class plCustomMeshComponent, plBlockStorageType::Compact>;

/// \brief This component is used to render custom geometry.
///
/// Sometimes game code needs to build geometry on the fly to visualize dynamic things.
/// The plDynamicMeshBufferResource is an easy to use resource to build geometry and change it frequently.
/// This component takes such a resource and takes care of rendering it.
/// The same resource can be set on multiple components to instantiate it in different locations.
class PLASMA_RENDERERCORE_DLL plCustomMeshComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plCustomMeshComponent, plRenderComponent, plCustomMeshComponentManager);

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
  // plCustomMeshComponent

public:
  plCustomMeshComponent();
  ~plCustomMeshComponent();

  /// \brief Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  plDynamicMeshBufferResourceHandle CreateMeshResource(plGALPrimitiveTopology::Enum topology, plUInt32 uiMaxVertices, plUInt32 uiMaxPrimitives, plGALIndexType::Enum indexType);

  /// \brief Returns the currently set mesh resource.
  plDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// \brief Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple plCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const plDynamicMeshBufferResourceHandle& hMesh);

  /// \brief Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(plUInt32 uiFirstPrimitive = 0, plUInt32 uiNumPrimitives = plMath::MaxValue<plUInt32>());

  /// \brief Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const plBoundingBoxSphere& bounds);

  /// \brief Sets the material for rendering.
  void SetMaterial(const plMaterialResourceHandle& hMaterial);

  /// \brief Returns the material that is used for rendering.
  plMaterialResourceHandle GetMaterial() const;

  void SetMaterialFile(const char* szMaterial); // [ property ]
  const char* GetMaterialFile() const;          // [ property ]

  /// \brief Sets the mesh instance color.
  void SetColor(const plColor& color); // [ property ]

  /// \brief Returns the mesh instance color.
  const plColor& GetColor() const; // [ property ]

  void OnMsgSetMeshMaterial(plMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(plMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plMaterialResourceHandle m_hMaterial;
  plColor m_Color = plColor::White;
  plUInt32 m_uiFirstPrimitive = 0;
  plUInt32 m_uiNumPrimitives = 0xFFFFFFFF;
  plBoundingBoxSphere m_Bounds;

  plDynamicMeshBufferResourceHandle m_hDynamicMesh;

  virtual void OnActivated() override;
};

/// \brief Temporary data used to feed the plCustomMeshRenderer.
class PLASMA_RENDERERCORE_DLL plCustomMeshRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomMeshRenderData, plRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  plDynamicMeshBufferResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;
  plColor m_Color = plColor::White;

  plUInt32 m_uiFlipWinding : 1;
  plUInt32 m_uiUniformScale : 1;

  plUInt32 m_uiFirstPrimitive = 0;
  plUInt32 m_uiNumPrimitives = 0xFFFFFFFF;

  plUInt32 m_uiUniqueID = 0;
};

/// \brief A renderer that handles all plCustomMeshRenderData.
class PLASMA_RENDERERCORE_DLL plCustomMeshRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomMeshRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plCustomMeshRenderer);

public:
  plCustomMeshRenderer();
  ~plCustomMeshRenderer();

  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;
};
