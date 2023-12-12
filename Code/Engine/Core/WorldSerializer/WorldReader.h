#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

class plStringDeduplicationReadContext;
class plProgress;
class plProgressRange;

struct plPrefabInstantiationOptions
{
  plGameObjectHandle m_hParent;

  plDynamicArray<plGameObject*>* m_pCreatedRootObjectsOut = nullptr;
  plDynamicArray<plGameObject*>* m_pCreatedChildObjectsOut = nullptr;
  const plUInt16* m_pOverrideTeamID = nullptr;

  bool m_bForceDynamic = false;

  /// \brief If the prefab has a single root node with this non-empty name, rather than creating a new object, instead the m_hParent object is used.
  plTempHashedString m_ReplaceNamedRootWithParent;

  enum class RandomSeedMode
  {
    DeterministicFromParent,
    CompletelyRandom,
    FixedFromSerialization,
    CustomRootValue,
  };

  RandomSeedMode m_RandomSeedMode = RandomSeedMode::DeterministicFromParent;
  plUInt32 m_uiCustomRandomSeedRootValue = 0;

  plTime m_MaxStepTime = plTime::Zero();

  plProgress* m_pProgress = nullptr;
};

/// \brief Reads a world description from a stream. Allows to instantiate that world multiple times
///        in different locations and different plWorld's.
///
/// The reader will ignore unknown component types and skip them during instantiation.
class PLASMA_CORE_DLL plWorldReader
{
public:
  /// \brief A context object is returned from InstantiateWorld or InstantiatePrefab if a maxStepTime greater than zero is specified.
  ///
  /// Call the Step() function periodically to complete the instantiation.
  /// Each step will try to spend not more than the given maxStepTime.
  /// E.g. this is useful if the instantiation cost of large prefabs needs to be distributed over multiple frames.
  class InstantiationContextBase
  {
  public:
    enum class StepResult
    {
      Continue,          ///< The available time slice is used up. Call Step() again to continue the process.
      ContinueNextFrame, ///< The process has reached a point where you need to call plWorld::Update(). Otherwise no further progress can be made.
      Finished,          ///< The instantiation is finished and you can delete the context. Don't call 'Step()' on it again.
    };

    virtual ~InstantiationContextBase() {}

    /// \Brief Advance the instantiation by one step
    /// \return Whether the operation is finished or needs to be repeated.
    virtual StepResult Step() = 0;

    /// \Brief Cancel the instantiation. This might lead to inconsistent states and must be used with care.
    virtual void Cancel() = 0;
  };

  plWorldReader();
  ~plWorldReader();

  /// \brief Reads all information about the world from the given stream.
  ///
  /// Call this once to populate plWorldReader with information how to instantiate the world.
  /// Afterwards \a stream can be deleted.
  /// Call InstantiateWorld() or InstantiatePrefab() afterwards as often as you like
  /// to actually get an objects into an plWorld.
  /// By default, the method will warn if it skips bytes in the stream that are of unknown
  /// types. The warnings can be suppressed by setting warningOnUnkownSkip to false.
  plResult ReadWorldDescription(plStreamReader& stream, bool warningOnUnkownSkip = true);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// This is identical to calling InstantiatePrefab() with identity values, however, it is a bit
  /// more efficient, as unnecessary computations are skipped.
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The plProgress object
  /// has to be valid as long as the instantiation is in progress.
  plUniquePtr<InstantiationContextBase> InstantiateWorld(plWorld& world, const plUInt16* pOverrideTeamID = nullptr, plTime maxStepTime = plTime::Zero(), plProgress* pProgress = nullptr);

  /// \brief Creates one instance of the world that was previously read by ReadWorldDescription().
  ///
  /// \param rootTransform is an additional transform that is applied to all root objects.
  /// \param hParent allows to attach the newly created objects immediately to a parent
  /// \param out_CreatedRootObjects If this is valid, all pointers the to created root objects are stored in this array
  ///
  /// If pOverrideTeamID is not null, every instantiated game object will get it passed in as its new value.
  /// This can be used to identify that the object belongs to a specific player or team.
  ///
  /// If maxStepTime is not zero the function will return a valid ptr to an InstantiationContextBase.
  /// This context will only spend the given amount of time in its Step() function.
  /// The function has to be periodically called until it returns true to complete the instantiation.
  ///
  /// If pProgress is a valid pointer it is used to track the progress of the instantiation. The plProgress object
  /// has to be valid as long as the instantiation is in progress.
  plUniquePtr<InstantiationContextBase> InstantiatePrefab(plWorld& world, const plTransform& rootTransform, const plPrefabInstantiationOptions& options);

  /// \brief Gives access to the stream of data. Use this inside component deserialization functions to read data.
  plStreamReader& GetStream() const { return *m_pStream; }

  /// \brief Used during component deserialization to read a handle to a game object.
  plGameObjectHandle ReadGameObjectHandle();

  /// \brief Used during component deserialization to read a handle to a component.
  void ReadComponentHandle(plComponentHandle& out_hComponent);

  /// \brief Used during component deserialization to query the actual version number with which the
  /// given component type was written. The version number is given through the PLASMA_BEGIN_COMPONENT_TYPE
  /// macro. Whenever the serialization of a component changes, that number should be increased.
  plUInt32 GetComponentTypeVersion(const plRTTI* pRtti) const;

  /// \brief Clears all data.
  void ClearAndCompact();

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  plUInt64 GetHeapMemoryUsage() const;

  using FindComponentTypeCallback = plDelegate<const plRTTI*(plStringView sTypeName)>;

  /// \brief An optional callback to redirect the lookup of a component type name to an plRTTI type.
  ///
  /// If specified, this is used by ALL world readers. The intention is to use this either for logging purposes,
  /// or to implement a whitelist or blacklist for specific component types.
  /// E.g. if the callback returns nullptr, the component type is 'unknown' and skipped by the world reader.
  /// Thus one can remove unwanted component types.
  /// Theoretically one could also redirect an old (or renamed) component type to a new one,
  /// given that their deserialization code is compatible.
  static FindComponentTypeCallback s_FindComponentTypeCallback;

  plUInt32 GetRootObjectCount() const;
  plUInt32 GetChildObjectCount() const;

  static void SetMaxStepTime(InstantiationContextBase* context, plTime maxStepTime);
  static plTime GetMaxStepTime(InstantiationContextBase* context);

private:
  struct GameObjectToCreate
  {
    plGameObjectDesc m_Desc;
    plString m_sGlobalKey;
    plUInt32 m_uiParentHandleIdx;
  };

  void ReadGameObjectDesc(GameObjectToCreate& godesc);
  void ReadComponentTypeInfo(plUInt32 uiComponentTypeIdx);
  void ReadComponentDataToMemStream(bool warningOnUnknownSkip = true);
  void ClearHandles();
  plUniquePtr<InstantiationContextBase> Instantiate(plWorld& world, bool bUseTransform, const plTransform& rootTransform, const plPrefabInstantiationOptions& options);

  plStreamReader* m_pStream = nullptr;
  plWorld* m_pWorld = nullptr;

  plUInt8 m_uiVersion = 0;
  plDynamicArray<plGameObjectHandle> m_IndexToGameObjectHandle;

  plDynamicArray<GameObjectToCreate> m_RootObjectsToCreate;
  plDynamicArray<GameObjectToCreate> m_ChildObjectsToCreate;

  struct ComponentTypeInfo
  {
    const plRTTI* m_pRtti = nullptr;
    plDynamicArray<plComponentHandle> m_ComponentIndexToHandle;
    plUInt32 m_uiNumComponents = 0;
  };

  plDynamicArray<ComponentTypeInfo> m_ComponentTypes;
  plHashTable<const plRTTI*, plUInt32> m_ComponentTypeVersions;
  plDefaultMemoryStreamStorage m_ComponentCreationStream;
  plDefaultMemoryStreamStorage m_ComponentDataStream;
  plUInt64 m_uiTotalNumComponents = 0;

  plUniquePtr<plStringDeduplicationReadContext> m_pStringDedupReadContext;

  class InstantiationContext : public InstantiationContextBase
  {
  public:
    InstantiationContext(plWorldReader& worldReader, bool bUseTransform, const plTransform& rootTransform, const plPrefabInstantiationOptions& options);
    ~InstantiationContext();

    virtual StepResult Step() override;
    virtual void Cancel() override;

    template <bool UseTransform>
    bool CreateGameObjects(const plDynamicArray<GameObjectToCreate>& objects, plGameObjectHandle hParent, plDynamicArray<plGameObject*>* out_CreatedObjects, plTime endTime);

    bool CreateComponents(plTime endTime);
    bool DeserializeComponents(plTime endTime);
    bool AddComponentsToBatch(plTime endTime);

    void SetMaxStepTime(plTime stepTime);
    plTime GetMaxStepTime() const;

  private:
    void BeginNextProgressStep(plStringView sName);
    void SetSubProgressCompletion(double fCompletion);

    friend class plWorldReader;
    plWorldReader& m_WorldReader;

    bool m_bUseTransform = false;
    plTransform m_RootTransform;

    plPrefabInstantiationOptions m_Options;

    plComponentInitBatchHandle m_hComponentInitBatch;

    // Current state
    struct Phase
    {
      enum Enum
      {
        Invalid = -1,
        CreateRootObjects,
        CreateChildObjects,
        CreateComponents,
        DeserializeComponents,
        AddComponentsToBatch,
        InitComponents,

        Count
      };
    };

    Phase::Enum m_Phase = Phase::Invalid;
    plUInt32 m_uiCurrentIndex = 0; // object or component
    plUInt32 m_uiCurrentComponentTypeIndex = 0;
    plUInt64 m_uiCurrentNumComponentsProcessed = 0;
    plMemoryStreamReader m_CurrentReader;

    plUniquePtr<plProgressRange> m_pOverallProgressRange;
    plUniquePtr<plProgressRange> m_pSubProgressRange;
  };
};
