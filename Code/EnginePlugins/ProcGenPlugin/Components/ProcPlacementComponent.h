#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

class plProcPlacementComponent;
struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class PLASMA_PROCGENPLUGIN_DLL plProcPlacementComponentManager : public plComponentManager<plProcPlacementComponent, plBlockStorageType::Compact>
{
public:
  plProcPlacementComponentManager(plWorld* pWorld);
  ~plProcPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class plProcPlacementComponent;

  void FindTiles(const plWorldModule::UpdateContext& context);
  void PreparePlace(const plWorldModule::UpdateContext& context);
  void PlaceObjects(const plWorldModule::UpdateContext& context);

  void DebugDrawTile(const plProcGenInternal::PlacementTileDesc& desc, const plColor& color, plUInt32 uiQueueIndex = plInvalidIndex);

  void AddComponent(plProcPlacementComponent* pComponent);
  void RemoveComponent(plProcPlacementComponent* pComponent);

  plUInt32 AllocateTile(const plProcGenInternal::PlacementTileDesc& desc, plSharedPtr<const plProcGenInternal::PlacementOutput>& pOutput);
  void DeallocateTile(plUInt32 uiTileIndex);

  plUInt32 AllocateProcessingTask(plUInt32 uiTileIndex);
  void DeallocateProcessingTask(plUInt32 uiTaskIndex);
  plUInt32 GetNumAllocatedProcessingTasks() const;

  void RemoveTilesForComponent(plProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved = nullptr);
  void OnResourceEvent(const plResourceEvent& resourceEvent);

  void AddVisibleComponent(const plComponentHandle& hComponent, const plVec3& cameraPosition, const plVec3& cameraDirection) const;
  void ClearVisibleComponents();

  struct VisibleComponent
  {
    plComponentHandle m_hComponent;
    plVec3 m_vCameraPosition;
    plVec3 m_vCameraDirection;
  };

  mutable plMutex m_VisibleComponentsMutex;
  mutable plDynamicArray<VisibleComponent> m_VisibleComponents;

  plDynamicArray<plComponentHandle> m_ComponentsToUpdate;

  plDynamicArray<plProcGenInternal::PlacementTile, plAlignedAllocatorWrapper> m_ActiveTiles;
  plDynamicArray<plUInt32> m_FreeTiles;

  struct ProcessingTask
  {
    PLASMA_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != plInvalidIndex; }
    PLASMA_ALWAYS_INLINE bool IsScheduled() const { return m_PlacementTaskGroupID.IsValid(); }
    PLASMA_ALWAYS_INLINE void Invalidate()
    {
      m_uiScheduledFrame = -1;
      m_PlacementTaskGroupID.Invalidate();
      m_uiTileIndex = plInvalidIndex;
    }

    plUInt64 m_uiScheduledFrame;
    plUniquePtr<plProcGenInternal::PlacementData> m_pData;
    plSharedPtr<plProcGenInternal::PreparePlacementTask> m_pPrepareTask;
    plSharedPtr<plProcGenInternal::PlacementTask> m_pPlacementTask;
    plTaskGroupID m_PlacementTaskGroupID;
    plUInt32 m_uiTileIndex;
  };

  plDynamicArray<ProcessingTask> m_ProcessingTasks;
  plDynamicArray<plUInt32> m_FreeProcessingTasks;

  struct SortedProcessingTask
  {
    plUInt64 m_uiScheduledFrame = 0;
    plUInt32 m_uiTaskIndex = 0;
  };

  plDynamicArray<SortedProcessingTask> m_SortedProcessingTasks;

  plDynamicArray<plProcGenInternal::PlacementTileDesc, plAlignedAllocatorWrapper> m_NewTiles;
  plTaskGroupID m_UpdateTilesTaskGroupID;
};

//////////////////////////////////////////////////////////////////////////

struct plProcGenBoxExtents
{
  plVec3 m_vOffset = plVec3::ZeroVector();
  plQuat m_Rotation = plQuat::IdentityQuaternion();
  plVec3 m_vExtents = plVec3(10);

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PROCGENPLUGIN_DLL, plProcGenBoxExtents);

class PLASMA_PROCGENPLUGIN_DLL plProcPlacementComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plProcPlacementComponent, plComponent, plProcPlacementComponentManager);

public:
  plProcPlacementComponent();
  ~plProcPlacementComponent();

  plProcPlacementComponent& operator=(plProcPlacementComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const plProcGenGraphResourceHandle& hResource);
  const plProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

private:
  plUInt32 BoxExtents_GetCount() const;
  const plProcGenBoxExtents& BoxExtents_GetValue(plUInt32 uiIndex) const;
  void BoxExtents_SetValue(plUInt32 uiIndex, const plProcGenBoxExtents& value);
  void BoxExtents_Insert(plUInt32 uiIndex, const plProcGenBoxExtents& value);
  void BoxExtents_Remove(plUInt32 uiIndex);

  void UpdateBoundsAndTiles();

  plProcGenGraphResourceHandle m_hResource;

  plDynamicArray<plProcGenBoxExtents> m_BoxExtents;

  // runtime data
  friend class plProcGenInternal::FindPlacementTilesTask;

  struct Bounds
  {
    PLASMA_DECLARE_POD_TYPE();

    plSimdBBox m_GlobalBoundingBox;
    plSimdMat4f m_GlobalToLocalBoxTransform;
  };

  plDynamicArray<Bounds, plAlignedAllocatorWrapper> m_Bounds;

  struct OutputContext
  {
    plSharedPtr<const plProcGenInternal::PlacementOutput> m_pOutput;

    struct TileIndexAndAge
    {
      PLASMA_DECLARE_POD_TYPE();

      plUInt32 m_uiIndex;
      plUInt64 m_uiLastSeenFrame;
    };

    plHashTable<plUInt64, TileIndexAndAge> m_TileIndices;

    plSharedPtr<plProcGenInternal::FindPlacementTilesTask> m_pUpdateTilesTask;
  };

  plDynamicArray<OutputContext> m_OutputContexts;
};
