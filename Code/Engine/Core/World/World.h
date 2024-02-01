#pragma once

#include <Core/World/Implementation/WorldData.h>

struct plEventMessage;
class plEventMessageHandlerComponent;

/// \brief A world encapsulates a scene graph of game objects and various component managers and their components.
///
/// There can be multiple worlds active at a time, but only 64 at most. The world manages all object storage and might move objects around
/// in memory. Thus it is not allowed to store pointers to objects. They should be referenced by handles.\n The world has a multi-phase
/// update mechanism which is divided in the following phases:\n
/// * Pre-async phase: The corresponding component manager update functions are called synchronously in the order of their dependencies.
/// * Async phase: The update functions are called in batches asynchronously on multiple threads. There is absolutely no guarantee in which
/// order the functions are called.
///   Thus it is not allowed to access any data other than the components own data during that phase.
/// * Post-async phase: Another synchronous phase like the pre-async phase.
/// * Actual deletion of dead objects and components are done now.
/// * Transform update: The global transformation of dynamic objects is updated.
/// * Post-transform phase: Another synchronous phase like the pre-async phase after the transformation has been updated.
class PL_CORE_DLL plWorld final
{
public:
  /// \brief Creates a new world with the given name.
  plWorld(plWorldDesc& ref_desc);
  ~plWorld();

  /// \brief Deletes all game objects in a world
  void Clear();

  /// \brief Returns the name of this world.
  plStringView GetName() const;

  /// \brief Returns the index of this world.
  plUInt32 GetIndex() const;

  /// \name Object Functions
  ///@{

  /// \brief Create a new game object from the given description and returns a handle to it.
  plGameObjectHandle CreateObject(const plGameObjectDesc& desc);

  /// \brief Create a new game object from the given description, writes a pointer to it to out_pObject and returns a handle to it.
  plGameObjectHandle CreateObject(const plGameObjectDesc& desc, plGameObject*& out_pObject);

  /// \brief Deletes the given object, its children and all components.
  /// \note This function deletes the object immediately! It is unsafe to use this during a game update loop, as other objects
  /// may rely on this object staying valid for the rest of the frame.
  /// Use DeleteObjectDelayed() instead for safe removal at the end of the frame.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectNow(const plGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents = true);

  /// \brief Deletes the given object at the beginning of the next world update. The object and its components and children stay completely
  /// valid until then.
  ///
  /// If bAlsoDeleteEmptyParents is set, any ancestor object that has no other children and no components, will also get deleted.
  void DeleteObjectDelayed(const plGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents = true);

  /// \brief Returns the event that is triggered before an object is deleted. This can be used for external systems to cleanup data
  /// which is associated with the deleted object.
  const plEvent<const plGameObject*>& GetObjectDeletionEvent() const;

  /// \brief Returns whether the given handle corresponds to a valid object.
  bool IsValidObject(const plGameObjectHandle& hObject) const;

  /// \brief Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const plGameObjectHandle& hObject, plGameObject*& out_pObject);

  /// \brief Returns whether an object with the given handle exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObject(const plGameObjectHandle& hObject, const plGameObject*& out_pObject) const;

  /// \brief Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, plGameObject*& out_pObject);

  /// \brief Returns whether an object with the given global key exists and if so writes out the corresponding pointer to out_pObject.
  [[nodiscard]] bool TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, const plGameObject*& out_pObject) const;


  /// \brief Returns the total number of objects in this world.
  plUInt32 GetObjectCount() const;

  /// \brief Returns an iterator over all objects in this world in no specific order.
  plInternal::WorldData::ObjectIterator GetObjects();

  /// \brief Returns an iterator over all objects in this world in no specific order.
  plInternal::WorldData::ConstObjectIterator GetObjects() const;

  /// \brief Defines a visitor function that is called for every game-object when using the traverse method.
  /// The function takes a pointer to the game object as argument and returns a bool which indicates whether to continue (true) or abort
  /// (false) traversal.
  using VisitorFunc = plInternal::WorldData::VisitorFunc;

  enum TraversalMethod
  {
    BreadthFirst,
    DepthFirst
  };

  /// \brief Traverses the game object tree starting at the top level objects and then recursively all children. The given callback function
  /// is called for every object.
  void Traverse(VisitorFunc visitorFunc, TraversalMethod method = DepthFirst);

  ///@}
  /// \name Module Functions
  ///@{

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  template <typename ModuleType>
  ModuleType* GetOrCreateModule();

  /// \brief Creates an instance of the given module type or derived type or returns a pointer to an already existing instance.
  plWorldModule* GetOrCreateModule(const plRTTI* pRtti);

  /// \brief Deletes the module of the given type or derived types.
  template <typename ModuleType>
  void DeleteModule();

  /// \brief Deletes the module of the given type or derived types.
  void DeleteModule(const plRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  ModuleType* GetModule();

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModule() const;

  /// \brief Returns the instance to the given module type or derived types.
  template <typename ModuleType>
  const ModuleType* GetModuleReadOnly() const;

  /// \brief Returns the instance to the given module type or derived types.
  plWorldModule* GetModule(const plRTTI* pRtti);

  /// \brief Returns the instance to the given module type or derived types.
  const plWorldModule* GetModule(const plRTTI* pRtti) const;

  ///@}
  /// \name Component Functions
  ///@{

  /// \brief Creates an instance of the given component manager type or returns a pointer to an already existing instance.
  template <typename ManagerType>
  ManagerType* GetOrCreateComponentManager();

  /// \brief Returns the component manager that handles the given rtti component type.
  plComponentManagerBase* GetOrCreateManagerForComponentType(const plRTTI* pComponentRtti);

  /// \brief Deletes the component manager of the given type and all its components.
  template <typename ManagerType>
  void DeleteComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  ManagerType* GetComponentManager();

  /// \brief Returns the instance to the given component manager type.
  template <typename ManagerType>
  const ManagerType* GetComponentManager() const;

  /// \brief Returns the component manager that handles the given rtti component type.
  plComponentManagerBase* GetManagerForComponentType(const plRTTI* pComponentRtti);

  /// \brief Returns the component manager that handles the given rtti component type.
  const plComponentManagerBase* GetManagerForComponentType(const plRTTI* pComponentRtti) const;

  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const plComponentHandle& hComponent) const;

  /// \brief Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const plComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// \brief Returns whether a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  template <typename ComponentType>
  [[nodiscard]] bool TryGetComponent(const plComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// \brief Creates a new component init batch.
  /// It is ensured that the Initialize function is called for all components in a batch before the OnSimulationStarted is called.
  /// If bMustFinishWithinOneFrame is set to false the processing of an init batch can be distributed over multiple frames if
  /// m_MaxComponentInitializationTimePerFrame in the world desc is set to a reasonable value.
  plComponentInitBatchHandle CreateComponentInitBatch(plStringView sBatchName, bool bMustFinishWithinOneFrame = true);

  /// \brief Deletes a component init batch. It must be completely processed before it can be deleted.
  void DeleteComponentInitBatch(const plComponentInitBatchHandle& hBatch);

  /// \brief All components that are created between an BeginAddingComponentsToInitBatch/EndAddingComponentsToInitBatch scope are added to the
  /// given init batch.
  void BeginAddingComponentsToInitBatch(const plComponentInitBatchHandle& hBatch);

  /// \brief End adding components to the given batch. Components created after this call are added to the default init batch.
  void EndAddingComponentsToInitBatch(const plComponentInitBatchHandle& hBatch);

  /// \brief After all components have been added to the init batch call submit to start processing the batch.
  void SubmitComponentInitBatch(const plComponentInitBatchHandle& hBatch);

  /// \brief Returns whether the init batch has been completely processed and all corresponding components are initialized
  /// and their OnSimulationStarted function was called.
  bool IsComponentInitBatchCompleted(const plComponentInitBatchHandle& hBatch, double* pCompletionFactor = nullptr);

  /// \brief Cancel the init batch if it is still active. This might leave outstanding components in an inconsistent state,
  /// so this function has be used with care.
  void CancelComponentInitBatch(const plComponentInitBatchHandle& hBatch);

  ///@}
  /// \name Message Functions
  ///@{

  /// \brief Sends a message to all components of the receiverObject.
  void SendMessage(const plGameObjectHandle& hReceiverObject, plMessage& ref_msg);

  /// \brief Sends a message to all components of the receiverObject and all its children.
  void SendMessageRecursive(const plGameObjectHandle& hReceiverObject, plMessage& ref_msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverObject after the given delay in the corresponding phase.
  void PostMessage(const plGameObjectHandle& hReceiverObject, const plMessage& msg, plTime delay,
    plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;

  /// \brief Queues the message for the given phase. The message is send to the receiverObject and all its children after the given delay in
  /// the corresponding phase.
  void PostMessageRecursive(const plGameObjectHandle& hReceiverObject, const plMessage& msg, plTime delay,
    plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;

  /// \brief Sends a message to the component.
  void SendMessage(const plComponentHandle& hReceiverComponent, plMessage& ref_msg);

  /// \brief Queues the message for the given phase. The message is send to the receiverComponent after the given delay in the corresponding phase.
  void PostMessage(const plComponentHandle& hReceiverComponent, const plMessage& msg, plTime delay,
    plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;

  /// \brief Finds the closest (parent) object, starting at pSearchObject, which has an plComponent that handles the given message and returns all
  /// matching components owned by that object. If a plEventMessageHandlerComponent is found the search is stopped even if it doesn't handle the given message.
  ///
  /// If no such parent object exists, it searches for all plEventMessageHandlerComponent instances that are set to 'handle global events'
  /// that handle messages of the given type.
  void FindEventMsgHandlers(const plMessage& msg, plGameObject* pSearchObject, plDynamicArray<plComponent*>& out_components);

  /// \copydoc plWorld::FindEventMsgHandlers()
  void FindEventMsgHandlers(const plMessage& msg, const plGameObject* pSearchObject, plDynamicArray<const plComponent*>& out_components) const;

  ///@}

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  void SetWorldSimulationEnabled(bool bEnable);

  /// \brief If enabled, the full simulation should be executed, otherwise only the rendering related updates should be done
  bool GetWorldSimulationEnabled() const;

  /// \brief Updates the world by calling the various update methods on the component managers and also updates the transformation data of
  /// the game objects. See plWorld for a detailed description of the update phases.
  void Update();

  /// \brief Returns a task implementation that calls Update on this world.
  const plSharedPtr<plTask>& GetUpdateTask();

  /// \brief Returns the number of update calls. Can be used to determine whether an operation has already been done during a frame.
  plUInt32 GetUpdateCounter() const;

  /// \brief Returns the spatial system that is associated with this world.
  plSpatialSystem* GetSpatialSystem();

  /// \brief Returns the spatial system that is associated with this world.
  const plSpatialSystem* GetSpatialSystem() const;


  /// \brief Returns the coordinate system for the given position.
  /// By default this always returns a coordinate system with forward = +X, right = +Y and up = +Z.
  /// This can be customized by setting a different coordinate system provider.
  void GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_coordinateSystem) const;

  /// \brief Sets the coordinate system provider that should be used in this world.
  void SetCoordinateSystemProvider(const plSharedPtr<plCoordinateSystemProvider>& pProvider);

  /// \brief Returns the coordinate system provider that is associated with this world.
  plCoordinateSystemProvider& GetCoordinateSystemProvider();

  /// \brief Returns the coordinate system provider that is associated with this world.
  const plCoordinateSystemProvider& GetCoordinateSystemProvider() const;


  /// \brief Returns the clock that is used for all updates in this game world
  plClock& GetClock();

  /// \brief Returns the clock that is used for all updates in this game world
  const plClock& GetClock() const;

  /// \brief Accesses the default random number generator.
  /// If more control is desired, individual components should use their own RNG.
  plRandom& GetRandomNumberGenerator();


  /// \brief Returns the allocator used by this world.
  plAllocator* GetAllocator();

  /// \brief Returns the block allocator used by this world.
  plInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns the stack allocator used by this world.
  plDoubleBufferedLinearAllocator* GetStackAllocator();

  /// \brief Mark the world for reading by using PL_LOCK(world.GetReadMarker()). Multiple threads can read simultaneously if none is
  /// writing.
  plInternal::WorldData::ReadMarker& GetReadMarker() const;

  /// \brief Mark the world for writing by using PL_LOCK(world.GetWriteMarker()). Only one thread can write at a time.
  plInternal::WorldData::WriteMarker& GetWriteMarker();

  /// \brief Allows re-setting the maximum time that is spent on component initialization per frame, which is first configured on construction.
  void SetMaxInitializationTimePerFrame(plTime maxInitTime);

  /// \brief Associates the given user data with the world. The user is responsible for the life time of user data.
  void SetUserData(void* pUserData);

  /// \brief Returns the associated user data.
  void* GetUserData() const;

  using ReferenceResolver = plDelegate<plGameObjectHandle(const void*, plComponentHandle hThis, plStringView sProperty)>;

  /// \brief If set, this delegate can be used to map some data (GUID or string) to an plGameObjectHandle.
  ///
  /// Currently only used in editor settings, to create a runtime handle from a unique editor reference.
  void SetGameObjectReferenceResolver(const ReferenceResolver& resolver);

  /// \sa SetGameObjectReferenceResolver()
  const ReferenceResolver& GetGameObjectReferenceResolver() const;

  using ResourceReloadContext = plInternal::WorldData::ResourceReloadContext;
  using ResourceReloadFunc = plInternal::WorldData::ResourceReloadFunc;

  /// \brief Add a function that is called when the given resource has been reloaded.
  void AddResourceReloadFunction(plTypelessResourceHandle hResource, plComponentHandle hComponent, void* pUserData, ResourceReloadFunc function);
  void RemoveResourceReloadFunction(plTypelessResourceHandle hResource, plComponentHandle hComponent, void* pUserData);

  /// \name Helper methods to query plWorld limits
  ///@{
  static constexpr plUInt64 GetMaxNumGameObjects();
  static constexpr plUInt64 GetMaxNumHierarchyLevels();
  static constexpr plUInt64 GetMaxNumComponentsPerType();
  static constexpr plUInt64 GetMaxNumWorldModules();
  static constexpr plUInt64 GetMaxNumComponentTypes();
  static constexpr plUInt64 GetMaxNumWorlds();
  ///@}

public:
  /// \brief Returns the number of active worlds.
  static plUInt32 GetWorldCount();

  /// \brief Returns the world with the given index.
  static plWorld* GetWorld(plUInt32 uiIndex);

  /// \brief Returns the world for the given game object handle.
  static plWorld* GetWorld(const plGameObjectHandle& hObject);

  /// \brief Returns the world for the given component handle.
  static plWorld* GetWorld(const plComponentHandle& hComponent);

private:
  friend class plGameObject;
  friend class plWorldModule;
  friend class plComponentManagerBase;
  friend class plComponent;
  PL_ALLOW_PRIVATE_PROPERTIES(plWorld);

  plGameObject* Reflection_TryGetObjectWithGlobalKey(plTempHashedString sGlobalKey);
  plClock* Reflection_GetClock();

  void CheckForReadAccess() const;
  void CheckForWriteAccess() const;

  plGameObject* GetObjectUnchecked(plUInt32 uiIndex) const;

  void SetParent(plGameObject* pObject, plGameObject* pNewParent,
    plGameObject::TransformPreservation preserve = plGameObject::TransformPreservation::PreserveGlobal);
  void LinkToParent(plGameObject* pObject);
  void UnlinkFromParent(plGameObject* pObject);

  void SetObjectGlobalKey(plGameObject* pObject, const plHashedString& sGlobalKey);
  plStringView GetObjectGlobalKey(const plGameObject* pObject) const;

  void PostMessage(const plGameObjectHandle& receiverObject, const plMessage& msg, plObjectMsgQueueType::Enum queueType, plTime delay, bool bRecursive) const;
  void ProcessQueuedMessage(const plInternal::WorldData::MessageQueue::Entry& entry);
  void ProcessQueuedMessages(plObjectMsgQueueType::Enum queueType);

  template <typename World, typename GameObject, typename Component>
  static void FindEventMsgHandlers(World& world, const plMessage& msg, GameObject pSearchObject, plDynamicArray<Component>& out_components);

  void RegisterUpdateFunction(const plWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunction(const plWorldModule::UpdateFunctionDesc& desc);
  void DeregisterUpdateFunctions(plWorldModule* pModule);

  /// \brief Used by component managers to queue a new component for initialization during the next update
  void AddComponentToInitialize(plComponentHandle hComponent);

  void UpdateFromThread();
  void UpdateSynchronous(const plArrayPtr<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions);
  void UpdateAsynchronous();

  // returns if the batch was completely initialized
  bool ProcessInitializationBatch(plInternal::WorldData::InitBatch& batch, plTime endTime);
  void ProcessComponentsToInitialize();
  void ProcessUpdateFunctionsToRegister();
  plResult RegisterUpdateFunctionInternal(const plWorldModule::UpdateFunctionDesc& desc);

  void DeleteDeadObjects();
  void DeleteDeadComponents();

  void PatchHierarchyData(plGameObject* pObject, plGameObject::TransformPreservation preserve);
  void RecreateHierarchyData(plGameObject* pObject, bool bWasDynamic);

  void ProcessResourceReloadFunctions();

  bool ReportErrorWhenStaticObjectMoves() const;

  float GetInvDeltaSeconds() const;

  plSharedPtr<plTask> m_pUpdateTask;

  plInternal::WorldData m_Data;

  using QueuedMsgMetaData = plInternal::WorldData::QueuedMsgMetaData;

  plUInt32 m_uiIndex;
  static plStaticArray<plWorld*, PL_MAX_WORLDS> s_Worlds;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plWorld);

#include <Core/World/Implementation/World_inl.h>
