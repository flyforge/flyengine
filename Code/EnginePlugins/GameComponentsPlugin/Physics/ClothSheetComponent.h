#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plDynamicMeshBufferResourceHandle = plTypedResourceHandle<class plDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class PL_GAMECOMPONENTS_DLL plClothSheetComponentManager : public plComponentManager<class plClothSheetComponent, plBlockStorageType::FreeList>
{
public:
  plClothSheetComponentManager(plWorld* pWorld);
  ~plClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
  void UpdateBounds(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class PL_GAMECOMPONENTS_DLL plClothSheetRenderData final : public plRenderData
{
  PL_ADD_DYNAMIC_REFLECTION(plClothSheetRenderData, plRenderData);

public:
  plUInt32 m_uiUniqueID = 0;
  plArrayPtr<plVec3> m_Positions;
  plArrayPtr<plUInt16> m_Indices;
  plUInt16 m_uiVerticesX;
  plUInt16 m_uiVerticesY;
  plColor m_Color;

  plMaterialResourceHandle m_hMaterial;
};

class PL_GAMECOMPONENTS_DLL plClothSheetRenderer : public plRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plClothSheetRenderer, plRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plClothSheetRenderer);

public:
  plClothSheetRenderer();
  ~plClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  plDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

/// \brief Flags for how a piece of cloth should be simulated.
struct PL_GAMECOMPONENTS_DLL plClothSheetFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    FixedCornerTopLeft = PL_BIT(0),     ///< This corner can't move.
    FixedCornerTopRight = PL_BIT(1),    ///< This corner can't move.
    FixedCornerBottomRight = PL_BIT(2), ///< This corner can't move.
    FixedCornerBottomLeft = PL_BIT(3),  ///< This corner can't move.
    FixedEdgeTop = PL_BIT(4),           ///< This entire edge can't move.
    FixedEdgeRight = PL_BIT(5),         ///< This entire edge can't move.
    FixedEdgeBottom = PL_BIT(6),        ///< This entire edge can't move.
    FixedEdgeLeft = PL_BIT(7),          ///< This entire edge can't move.

    Default = FixedEdgeTop
  };

  struct Bits
  {
    StorageType FixedCornerTopLeft : 1;
    StorageType FixedCornerTopRight : 1;
    StorageType FixedCornerBottomRight : 1;
    StorageType FixedCornerBottomLeft : 1;
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMECOMPONENTS_DLL, plClothSheetFlags);

/// \brief Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class PL_GAMECOMPONENTS_DLL plClothSheetComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plClothSheetComponent, plRenderComponent, plClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // plClothSheetComponent

public:
  plClothSheetComponent();
  ~plClothSheetComponent();

  /// Sets the world-space size of the cloth.
  void SetSize(plVec2 vVal);                 // [ property ]
  plVec2 GetSize() const { return m_vSize; } // [ property ]

  /// Sets of how many pieces the cloth is made up.
  ///
  /// More pieces cost more performance to simulate the cloth.
  /// A size of 32x32 is already quite performance intensive. USe as few segments as possible.
  /// For many cases 8x8 or 12x12 should already be good enough.
  /// Also the more segments there are, the more the cloth will sag.
  void SetSegments(plVec2U32 vVal);                     // [ property ]
  plVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  /// How much sag the cloth should have along each axis.
  void SetSlack(plVec2 vVal);                  // [ property ]
  plVec2 GetSlack() const { return m_vSlack; } // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// Tint color for the cloth material.
  plColor m_Color = plColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(plBitflags<plClothSheetFlags> flags);                // [ property ]
  plBitflags<plClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  plMaterialResourceHandle m_hMaterial; // [ property ]

private:
  void Update();
  void SetupCloth();

  plVec2 m_vSize;
  plVec2 m_vSlack;
  plVec2U32 m_vSegments;
  plBitflags<plClothSheetFlags> m_Flags;

  plUInt8 m_uiSleepCounter = 0;
  mutable plUInt8 m_uiVisibleCounter = 0;
  plUInt8 m_uiCheckEquilibriumCounter = 0;
  plClothSimulator m_Simulator;

  plBoundingBox m_Bbox;
};
