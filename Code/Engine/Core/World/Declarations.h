#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

#ifndef PLASMA_WORLD_INDEX_BITS
#define PLASMA_WORLD_INDEX_BITS 8
#endif

#define PLASMA_MAX_WORLDS (1 << PLASMA_WORLD_INDEX_BITS)

class plWorld;
class plSpatialSystem;
class plCoordinateSystemProvider;

namespace plInternal
{
  class WorldData;

  enum
  {
    DEFAULT_BLOCK_SIZE = 1024 * 16
  };

  typedef plLargeBlockAllocator<DEFAULT_BLOCK_SIZE> WorldLargeBlockAllocator;
} // namespace plInternal

class plGameObject;
struct plGameObjectDesc;

class plComponentManagerBase;
class plComponent;

struct plMsgDeleteGameObject;

/// \brief Internal game object id used by plGameObjectHandle.
struct plGameObjectId
{
  typedef plUInt64 StorageType;

  PLASMA_DECLARE_ID_TYPE(plGameObjectId, 32, 8);

  static_assert(PLASMA_WORLD_INDEX_BITS > 0 && PLASMA_WORLD_INDEX_BITS <= 24);

  PLASMA_FORCE_INLINE plGameObjectId(StorageType instanceIndex, plUInt8 generation, plUInt8 worldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<plUInt32>(instanceIndex);
    m_Generation = generation;
    m_WorldIndex = worldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : PLASMA_WORLD_INDEX_BITS;
    };
  };
};

/// \brief A handle to a game object.
///
/// Never store a direct pointer to a game object. Always store a handle instead. A pointer to a game object can
/// be received by calling plWorld::TryGetObject with the handle.
/// Note that the object might have been deleted so always check the return value of TryGetObject.
struct plGameObjectHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGameObjectHandle, plGameObjectId);

  friend class plWorld;
  friend class plGameObject;
};

/// \brief HashHelper implementation so game object handles can be used as key in a hash table.
template <>
struct plHashHelper<plGameObjectHandle>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plGameObjectHandle value) { return plHashHelper<plUInt64>::Hash(value.GetInternalID().m_Data); }

  PLASMA_ALWAYS_INLINE static bool Equal(plGameObjectHandle a, plGameObjectHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for game object handles.
PLASMA_CORE_DLL void operator<<(plStreamWriter& Stream, const plGameObjectHandle& Value);
PLASMA_CORE_DLL void operator>>(plStreamReader& Stream, plGameObjectHandle& Value);

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plGameObjectHandle);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plGameObjectHandle);
#define PLASMA_COMPONENT_TYPE_INDEX_BITS (24 - PLASMA_WORLD_INDEX_BITS)
#define PLASMA_MAX_COMPONENT_TYPES (1 << PLASMA_COMPONENT_TYPE_INDEX_BITS)

/// \brief Internal component id used by plComponentHandle.
struct plComponentId
{
  typedef plUInt64 StorageType;

  PLASMA_DECLARE_ID_TYPE(plComponentId, 32, 8);

  static_assert(PLASMA_COMPONENT_TYPE_INDEX_BITS > 0 && PLASMA_COMPONENT_TYPE_INDEX_BITS <= 16);

  PLASMA_ALWAYS_INLINE plComponentId(StorageType instanceIndex, plUInt8 generation, plUInt16 typeId = 0, plUInt8 worldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<plUInt32>(instanceIndex);
    m_Generation = generation;
    m_TypeId = typeId;
    m_WorldIndex = worldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : PLASMA_WORLD_INDEX_BITS;
      StorageType m_TypeId : PLASMA_COMPONENT_TYPE_INDEX_BITS;
    };
  };
};

/// \brief A handle to a component.
///
/// Never store a direct pointer to a component. Always store a handle instead. A pointer to a component can
/// be received by calling plWorld::TryGetComponent or TryGetComponent on the corresponding component manager.
/// Note that the component might have been deleted so always check the return value of TryGetComponent.
struct plComponentHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plComponentHandle, plComponentId);

  friend class plWorld;
  friend class plComponentManagerBase;
  friend class plComponent;
};

/// \brief HashHelper implementation so component handles can be used as key in a hashtable.
template <>
struct plHashHelper<plComponentHandle>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plComponentHandle value)
  {
    plComponentId id = value.GetInternalID();
    plUInt64 data = *reinterpret_cast<plUInt64*>(&id);
    return plHashHelper<plUInt64>::Hash(data);
  }

  PLASMA_ALWAYS_INLINE static bool Equal(plComponentHandle a, plComponentHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for component handles.
PLASMA_CORE_DLL void operator<<(plStreamWriter& Stream, const plComponentHandle& Value);
PLASMA_CORE_DLL void operator>>(plStreamReader& Stream, plComponentHandle& Value);

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plComponentHandle);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plComponentHandle);

/// \brief Internal flags of game objects or components.
struct plObjectFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    Dynamic = PLASMA_BIT(0),                 ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = PLASMA_BIT(1),            ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                         ///< objects, and the automatic detection cannot know that.
    ActiveFlag = PLASMA_BIT(2),              ///< The object/component has the 'active flag' set
    ActiveState = PLASMA_BIT(3),             ///< The object/component and all its parents have the active flag
    Initialized = PLASMA_BIT(4),             ///< The object/component has been initialized
    Initializing = PLASMA_BIT(5),            ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted = PLASMA_BIT(6),       ///< OnSimulationStarted() has been called on the component
    SimulationStarting = PLASMA_BIT(7),      ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = PLASMA_BIT(8), ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications = PLASMA_BIT(9),            ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications = PLASMA_BIT(10),       ///< The object should send a notification message when components are added or removed.
    StaticTransformChangesNotifications = PLASMA_BIT(11), ///< The object should send a notification message if it is static and its transform changes.
    ParentChangesNotifications = PLASMA_BIT(12), ///< The object should send a notification message when the parent is changes.
    CreatedByPrefab = PLASMA_BIT(13),                         ///< Such flagged objects and components are ignored during scene export (see plWorldWriter) and will be removed when a prefab needs to be re-instantiated.

    UserFlag0 = PLASMA_BIT(24),
    UserFlag1 = PLASMA_BIT(25),
    UserFlag2 = PLASMA_BIT(26),
    UserFlag3 = PLASMA_BIT(27),
    UserFlag4 = PLASMA_BIT(28),
    UserFlag5 = PLASMA_BIT(29),
    UserFlag6 = PLASMA_BIT(30),
    UserFlag7 = PLASMA_BIT(31),

    Default = None
  };

  struct Bits
  {
    StorageType Dynamic : 1;                             //< 0
    StorageType ForceDynamic : 1;                        //< 1
    StorageType ActiveFlag : 1;                          //< 2
    StorageType ActiveState : 1;                         //< 3
    StorageType Initialized : 1;                         //< 4
    StorageType Initializing : 1;                        //< 5
    StorageType SimulationStarted : 1;                   //< 6
    StorageType SimulationStarting : 1;                  //< 7
    StorageType UnhandledMessageHandler : 1;             //< 8
    StorageType ChildChangesNotifications : 1;           //< 9
    StorageType ComponentChangesNotifications : 1;       //< 10
    StorageType StaticTransformChangesNotifications : 1; //< 11
    StorageType ParentChangesNotifications : 1;          //< 12
    StorageType CreatedByPrefab : 1;                     //< 13

    StorageType Padding : 10; // 14 - 23

    StorageType UserFlag0 : 1; //< 24
    StorageType UserFlag1 : 1; //< 25
    StorageType UserFlag2 : 1; //< 26
    StorageType UserFlag3 : 1; //< 27
    StorageType UserFlag4 : 1; //< 28
    StorageType UserFlag5 : 1; //< 29
    StorageType UserFlag6 : 1; //< 30
    StorageType UserFlag7 : 1; //< 31
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plObjectFlags);

/// \brief Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa plObjectFlags
struct plObjectMode
{
  typedef plUInt8 StorageType;

  enum Enum : plUInt8
  {
    Automatic,
    ForceDynamic,

    Default = Automatic
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plObjectMode);

/// \brief Specifies the mode of a component. Dynamic components may change an object's transform, static components must not.
///
/// \sa plObjectFlags
struct plComponentMode
{
  enum Enum
  {
    Static,
    Dynamic
  };
};

/// \brief Specifies at which phase the queued message should be processed.
struct plObjectMsgQueueType
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    PostAsync,        ///< Process the message in the PostAsync phase.
    PostTransform,    ///< Process the message in the PostTransform phase.
    NextFrame,        ///< Process the message in the PreAsync phase of the next frame.
    AfterInitialized, ///< Process the message after new components have been initialized.
    COUNT
  };
};

/// \brief Certain components may delete themselves or their owner when they are finished with their main purpose
struct PLASMA_CORE_DLL plOnComponentFinishedAction
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    None,
    DeleteComponent,
    DeleteGameObject,

    Default = None
  };

  /// \brief Call this when a component is 'finished' with its work.
  ///
  /// Pass in the desired action (usually configured by the user) and the 'this' pointer of the component.
  /// The helper function will delete this component and maybe also attempt to delete the entire object.
  /// For that it will coordinate with other components, and delay the object deletion, if necessary,
  /// until the last component has finished it's work.
  static void HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction::Enum action);

  /// \brief Call this function in a message handler for plMsgDeleteGameObject messages.
  ///
  /// This is needed to coordinate object deletion across multiple components that use the
  /// plOnComponentFinishedAction mechanism.
  /// Depending on the state of this component, the function will either execute the object deletion,
  /// or delay it, until its own work is done.
  static void HandleDeleteObjectMsg(plMsgDeleteGameObject& msg, plEnum<plOnComponentFinishedAction>& action);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plOnComponentFinishedAction);

/// \brief Same as plOnComponentFinishedAction, but additionally includes 'Restart'
struct PLASMA_CORE_DLL plOnComponentFinishedAction2
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    None,
    DeleteComponent,
    DeleteGameObject,
    Restart,

    Default = None
  };

  /// \brief See plOnComponentFinishedAction::HandleFinishedAction()
  static void HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction2::Enum action);

  /// \brief See plOnComponentFinishedAction::HandleDeleteObjectMsg()
  static void HandleDeleteObjectMsg(plMsgDeleteGameObject& msg, plEnum<plOnComponentFinishedAction2>& action);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plOnComponentFinishedAction2);

/// \brief Used as return value of visitor functions to define whether calling function should stop or continue visiting.
struct plVisitorExecution
{
  enum Enum
  {
    Continue, ///< Continue regular iteration
    Skip,     ///< In a depth-first iteration mode this will skip the entire sub-tree below the current object
    Stop      ///< Stop the entire iteration
  };
};

typedef plGenericId<24, 8> plSpatialDataId;
class plSpatialDataHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plSpatialDataHandle, plSpatialDataId);
};

#define PLASMA_MAX_WORLD_MODULE_TYPES PLASMA_MAX_COMPONENT_TYPES
typedef plUInt16 plWorldModuleTypeId;
static_assert(plMath::MaxValue<plWorldModuleTypeId>() >= PLASMA_MAX_WORLD_MODULE_TYPES - 1);

typedef plGenericId<24, 8> plComponentInitBatchId;
class plComponentInitBatchHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plComponentInitBatchHandle, plComponentInitBatchId);
};
