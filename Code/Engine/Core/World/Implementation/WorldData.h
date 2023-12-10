#pragma once

#include <Foundation/Communication/MessageQueue.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/Clock.h>

#include <Core/World/GameObject.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Types/SharedPtr.h>

namespace plInternal
{
  class PLASMA_CORE_DLL WorldData
  {
  private:
    friend class ::plWorld;
    friend class ::plComponentManagerBase;

    WorldData(plWorldDesc& desc);
    ~WorldData();

    void Clear();

    plHashedString m_sName;
    mutable plProxyAllocator m_Allocator;
    plLocalAllocatorWrapper m_AllocatorWrapper;
    plInternal::WorldLargeBlockAllocator m_BlockAllocator;
    plDoubleBufferedStackAllocator m_StackAllocator;

    enum
    {
      GAME_OBJECTS_PER_BLOCK = plDataBlock<plGameObject, plInternal::DEFAULT_BLOCK_SIZE>::CAPACITY,
      TRANSFORMATION_DATA_PER_BLOCK = plDataBlock<plGameObject::TransformationData, plInternal::DEFAULT_BLOCK_SIZE>::CAPACITY
    };

    // object storage
    typedef plBlockStorage<plGameObject, plInternal::DEFAULT_BLOCK_SIZE, plBlockStorageType::Compact> ObjectStorage;
    plIdTable<plGameObjectId, plGameObject*, plLocalAllocatorWrapper> m_Objects;
    ObjectStorage m_ObjectStorage;

    plSet<plGameObject*, plCompareHelper<plGameObject*>, plLocalAllocatorWrapper> m_DeadObjects;
    plEvent<const plGameObject*> m_ObjectDeletionEvent;

  public:
    class PLASMA_CORE_DLL ConstObjectIterator
    {
    public:
      const plGameObject& operator*() const;
      const plGameObject* operator->() const;

      operator const plGameObject*() const;

      /// \brief Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// \brief Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// \brief Shorthand for 'Next'
      void operator++();

    private:
      friend class ::plWorld;

      ConstObjectIterator(ObjectStorage::ConstIterator iterator);

      ObjectStorage::ConstIterator m_Iterator;
    };

    class PLASMA_CORE_DLL ObjectIterator
    {
    public:
      plGameObject& operator*();
      plGameObject* operator->();

      operator plGameObject*();

      /// \brief Advances the iterator to the next object. The iterator will not be valid anymore, if the last object is reached.
      void Next();

      /// \brief Checks whether this iterator points to a valid object.
      bool IsValid() const;

      /// \brief Shorthand for 'Next'
      void operator++();

    private:
      friend class ::plWorld;

      ObjectIterator(ObjectStorage::Iterator iterator);

      ObjectStorage::Iterator m_Iterator;
    };

  private:
    // hierarchy structures
    struct Hierarchy
    {
      typedef plDataBlock<plGameObject::TransformationData, plInternal::DEFAULT_BLOCK_SIZE> DataBlock;
      typedef plDynamicArray<DataBlock> DataBlockArray;

      plHybridArray<DataBlockArray*, 8, plLocalAllocatorWrapper> m_Data;
    };

    struct HierarchyType
    {
      enum Enum
      {
        Static,
        Dynamic,
        COUNT
      };
    };

    Hierarchy m_Hierarchies[HierarchyType::COUNT];

    static HierarchyType::Enum GetHierarchyType(bool bDynamic);

    plGameObject::TransformationData* CreateTransformationData(bool bDynamic, plUInt32 uiHierarchyLevel);

    void DeleteTransformationData(bool bDynamic, plUInt32 uiHierarchyLevel, plGameObject::TransformationData* pData);

    template <typename VISITOR>
    static plVisitorExecution::Enum TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);
    template <typename VISITOR>
    plVisitorExecution::Enum TraverseHierarchyLevelMultiThreaded(Hierarchy::DataBlockArray& blocks, void* pUserData = nullptr);

    typedef plDelegate<plVisitorExecution::Enum(plGameObject*)> VisitorFunc;
    void TraverseBreadthFirst(VisitorFunc& func);
    void TraverseDepthFirst(VisitorFunc& func);
    static plVisitorExecution::Enum TraverseObjectDepthFirst(plGameObject* pObject, VisitorFunc& func);

    static void UpdateGlobalTransform(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter);
    static void UpdateGlobalTransformWithParent(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter);

    static void UpdateGlobalTransformAndSpatialData(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter, plSpatialSystem& spatialSystem);
    static void UpdateGlobalTransformWithParentAndSpatialData(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter, plSpatialSystem& spatialSystem);

    void UpdateGlobalTransforms();

    // game object lookups
    plHashTable<plUInt64, plGameObjectId, plHashHelper<plUInt64>, plLocalAllocatorWrapper> m_GlobalKeyToIdTable;
    plHashTable<plUInt64, plHashedString, plHashHelper<plUInt64>, plLocalAllocatorWrapper> m_IdToGlobalKeyTable;

    // modules
    plDynamicArray<plWorldModule*, plLocalAllocatorWrapper> m_Modules;
    plDynamicArray<plWorldModule*, plLocalAllocatorWrapper> m_ModulesToStartSimulation;

    // component management
    plSet<plComponent*, plCompareHelper<plComponent*>, plLocalAllocatorWrapper> m_DeadComponents;

    struct InitBatch
    {
      InitBatch(plAllocatorBase* pAllocator, plStringView szName, bool bMustFinishWithinOneFrame);

      plHashedString m_sName;
      bool m_bMustFinishWithinOneFrame = true;
      bool m_bIsReady = false;

      plUInt32 m_uiNextComponentToInitialize = 0;
      plUInt32 m_uiNextComponentToStartSimulation = 0;
      plDynamicArray<plComponentHandle> m_ComponentsToInitialize;
      plDynamicArray<plComponentHandle> m_ComponentsToStartSimulation;
    };

    plTime m_MaxInitializationTimePerFrame;
    plIdTable<plComponentInitBatchId, plUniquePtr<InitBatch>, plLocalAllocatorWrapper> m_InitBatches;
    InitBatch* m_pDefaultInitBatch = nullptr;
    InitBatch* m_pCurrentInitBatch = nullptr;

    struct RegisteredUpdateFunction
    {
      plWorldModule::UpdateFunction m_Function;
      plHashedString m_sFunctionName;
      float m_fPriority;
      plUInt16 m_uiGranularity;
      bool m_bOnlyUpdateWhenSimulating;

      void FillFromDesc(const plWorldModule::UpdateFunctionDesc& desc);
      bool operator<(const RegisteredUpdateFunction& other) const;
    };

    struct UpdateTask final : public plTask
    {
      virtual void Execute() override;

      plWorldModule::UpdateFunction m_Function;
      plUInt32 m_uiStartIndex;
      plUInt32 m_uiCount;
    };

    plDynamicArray<RegisteredUpdateFunction, plLocalAllocatorWrapper> m_UpdateFunctions[plWorldModule::UpdateFunctionDesc::Phase::COUNT];
    plDynamicArray<plWorldModule::UpdateFunctionDesc, plLocalAllocatorWrapper> m_UpdateFunctionsToRegister;

    plDynamicArray<plSharedPtr<UpdateTask>, plLocalAllocatorWrapper> m_UpdateTasks;

    plUniquePtr<plSpatialSystem> m_pSpatialSystem;
    plSharedPtr<plCoordinateSystemProvider> m_pCoordinateSystemProvider;
    plUniquePtr<plTimeStepSmoothing> m_pTimeStepSmoothing;

    plClock m_Clock;
    plRandom m_Random;

    struct QueuedMsgMetaData
    {
      PLASMA_DECLARE_POD_TYPE();

      PLASMA_ALWAYS_INLINE QueuedMsgMetaData()
        : m_uiReceiverData(0)
      {
      }

      union
      {
        struct
        {
          plUInt64 m_uiReceiverObjectOrComponent : 62;
          plUInt64 m_uiReceiverIsComponent : 1;
          plUInt64 m_uiRecursive : 1;
        };

        plUInt64 m_uiReceiverData;
      };

      plTime m_Due;
    };

    typedef plMessageQueue<QueuedMsgMetaData, plLocalAllocatorWrapper> MessageQueue;
    mutable MessageQueue m_MessageQueues[plObjectMsgQueueType::COUNT];
    mutable MessageQueue m_TimedMessageQueues[plObjectMsgQueueType::COUNT];
    plObjectMsgQueueType::Enum m_ProcessingMessageQueue = plObjectMsgQueueType::COUNT;


    plThreadID m_WriteThreadID;
    plInt32 m_iWriteCounter;
    mutable plAtomicInteger32 m_iReadCounter;

    plUInt32 m_uiUpdateCounter = 0;
    bool m_bSimulateWorld;
    bool m_bReportErrorWhenStaticObjectMoves;

    /// \brief Maps some data (given as void*) to an plGameObjectHandle. Only available in special situations (e.g. editor use cases).
    plDelegate<plGameObjectHandle(const void*, plComponentHandle, plStringView)> m_GameObjectReferenceResolver;

  public:
    class ReadMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::plInternal::WorldData;

      ReadMarker(const WorldData& data);
      const WorldData& m_Data;
    };

    class WriteMarker
    {
    public:
      void Lock();
      void Unlock();

    private:
      friend class ::plInternal::WorldData;

      WriteMarker(WorldData& data);
      WorldData& m_Data;
    };

  private:
    mutable ReadMarker m_ReadMarker;
    WriteMarker m_WriteMarker;

    void* m_pUserData;
  };
} // namespace plInternal

#include <Core/World/Implementation/WorldData_inl.h>
