#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;
using plDynamicMeshBufferResourceHandle = plTypedResourceHandle<class plDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltClothSheetComponentManager : public plComponentManager<class plJoltClothSheetComponent, plBlockStorageType::FreeList>
{
public:
  plJoltClothSheetComponentManager(plWorld* pWorld);
  ~plJoltClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
  void UpdateBounds(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltClothSheetRenderData final : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltClothSheetRenderData, plRenderData);

public:
  plUInt32 m_uiUniqueID = 0;
  plVec2 m_vTextureScale = plVec2(1.0f);
  plArrayPtr<plVec3> m_Positions;
  plArrayPtr<plUInt16> m_Indices;
  plUInt16 m_uiVerticesX;
  plUInt16 m_uiVerticesY;
  plColor m_Color = plColor::White;

  plMaterialResourceHandle m_hMaterial;
};

class PLASMA_JOLTPLUGIN_DLL plJoltClothSheetRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltClothSheetRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plJoltClothSheetRenderer);

public:
  plJoltClothSheetRenderer();
  ~plJoltClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  plDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

/// \brief Flags for how a piece of cloth should be simulated.
struct PLASMA_JOLTPLUGIN_DLL plJoltClothSheetFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    FixedCornerTopLeft = PLASMA_BIT(0),     ///< This corner can't move.
    FixedCornerTopRight = PLASMA_BIT(1),    ///< This corner can't move.
    FixedCornerBottomRight = PLASMA_BIT(2), ///< This corner can't move.
    FixedCornerBottomLeft = PLASMA_BIT(3),  ///< This corner can't move.
    FixedEdgeTop = PLASMA_BIT(4),           ///< This entire edge can't move.
    FixedEdgeRight = PLASMA_BIT(5),         ///< This entire edge can't move.
    FixedEdgeBottom = PLASMA_BIT(6),        ///< This entire edge can't move.
    FixedEdgeLeft = PLASMA_BIT(7),          ///< This entire edge can't move.

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltClothSheetFlags);

/// \brief Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class PLASMA_JOLTPLUGIN_DLL plJoltClothSheetComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltClothSheetComponent, plRenderComponent, plJoltClothSheetComponentManager);

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
  // plJoltClothSheetComponent

public:
  plJoltClothSheetComponent();
  ~plJoltClothSheetComponent();

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

  /// The collision layer determines with which other actors this actor collides. \see plJoltActorComponent
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// \brief Adjusts how strongly gravity affects the soft body.
  float m_fGravityFactor = 1.0f; // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// Tint color for the cloth material.
  plColor m_Color = plColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(plBitflags<plJoltClothSheetFlags> flags);                // [ property ]
  plBitflags<plJoltClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  plMaterialResourceHandle m_hMaterial; // [ property ]

private:
  void Update();

  void ApplyWind();

  void SetupCloth();
  void RemoveBody();
  void UpdateBodyBounds();

  plVec2 m_vSize = plVec2(1.0f, 1.0f);
  plVec2 m_vTextureScale = plVec2(1.0f);
  plVec2U32 m_vSegments = plVec2U32(16, 16);
  plBitflags<plJoltClothSheetFlags> m_Flags;
  mutable plRenderData::Category m_RenderDataCategory;
  plUInt8 m_uiSleepCounter = 0;

  plUInt32 m_uiJoltBodyID = plInvalidIndex;
  plBoundingSphere m_BSphere;
};
