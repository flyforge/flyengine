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

class PLASMA_GAMEENGINE_DLL plClothSheetComponentManager : public plComponentManager<class plClothSheetComponent, plBlockStorageType::FreeList>
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

class PLASMA_GAMEENGINE_DLL plClothSheetRenderData final : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClothSheetRenderData, plRenderData);

public:
  plUInt32 m_uiUniqueID = 0;
  plArrayPtr<plVec3> m_Positions;
  plArrayPtr<plUInt16> m_Indices;
  plUInt16 m_uiVerticesX;
  plUInt16 m_uiVerticesY;
  plColor m_Color;

  plMaterialResourceHandle m_hMaterial;
};

class PLASMA_GAMEENGINE_DLL plClothSheetRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClothSheetRenderer, plRenderer);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plClothSheetRenderer);

public:
  plClothSheetRenderer();
  ~plClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const override;
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const override;
  virtual void RenderBatch(const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  plDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

struct PLASMA_GAMEENGINE_DLL plClothSheetFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    FixedCornerTopLeft = PLASMA_BIT(0),
    FixedCornerTopRight = PLASMA_BIT(1),
    FixedCornerBottomRight = PLASMA_BIT(2),
    FixedCornerBottomLeft = PLASMA_BIT(3),
    FixedEdgeTop = PLASMA_BIT(4),
    FixedEdgeRight = PLASMA_BIT(5),
    FixedEdgeBottom = PLASMA_BIT(6),
    FixedEdgeLeft = PLASMA_BIT(7),

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plClothSheetFlags);

class PLASMA_GAMEENGINE_DLL plClothSheetComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plClothSheetComponent, plRenderComponent, plClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // plClothSheetComponent

public:
  plClothSheetComponent();
  ~plClothSheetComponent();

  void SetSize(plVec2 val);                  // [ property ]
  plVec2 GetSize() const { return m_vSize; } // [ property ]

  void SetSlack(plVec2 val);                   // [ property ]
  plVec2 GetSlack() const { return m_vSlack; } // [ property ]

  void SetSegments(plVec2U32 val);                      // [ property ]
  plVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  float m_fWindInfluence = 0.3f;    // [ property ]
  float m_fDamping = 0.5f;          // [ property ]
  plColor m_Color = plColor::White; // [ property ]

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
