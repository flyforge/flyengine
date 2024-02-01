#pragma once

#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Id.h>

#include <Core/CoreDLL.h>

#ifndef PL_WORLD_INDEX_BITS
#  define PL_WORLD_INDEX_BITS 8
#endif

#define PL_MAX_WORLDS (1 << PL_WORLD_INDEX_BITS)

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

  using WorldLargeBlockAllocator = plLargeBlockAllocator<DEFAULT_BLOCK_SIZE>;
} // namespace plInternal

class plGameObject;
struct plGameObjectDesc;

class plComponentManagerBase;
class plComponent;

struct plMsgDeleteGameObject;

/// \brief Internal game object id used by plGameObjectHandle.
struct plGameObjectId
{
  using StorageType = plUInt64;

  PL_DECLARE_ID_TYPE(plGameObjectId, 32, 8);

  static_assert(PL_WORLD_INDEX_BITS > 0 && PL_WORLD_INDEX_BITS <= 24);

  PL_FORCE_INLINE plGameObjectId(StorageType instanceIndex, plUInt8 uiGeneration, plUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<plUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : PL_WORLD_INDEX_BITS;
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
  PL_DECLARE_HANDLE_TYPE(plGameObjectHandle, plGameObjectId);

  friend class plWorld;
  friend class plGameObject;
};

/// \brief HashHelper implementation so game object handles can be used as key in a hash table.
template <>
struct plHashHelper<plGameObjectHandle>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(plGameObjectHandle value) { return plHashHelper<plUInt64>::Hash(value.GetInternalID().m_Data); }

  PL_ALWAYS_INLINE static bool Equal(plGameObjectHandle a, plGameObjectHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for game object handles.
PL_CORE_DLL void operator<<(plStreamWriter& inout_stream, const plGameObjectHandle& hValue);
PL_CORE_DLL void operator>>(plStreamReader& inout_stream, plGameObjectHandle& ref_hValue);

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plGameObjectHandle);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plGameObjectHandle);
#define PL_COMPONENT_TYPE_INDEX_BITS (24 - PL_WORLD_INDEX_BITS)
#define PL_MAX_COMPONENT_TYPES (1 << PL_COMPONENT_TYPE_INDEX_BITS)

/// \brief Internal component id used by plComponentHandle.
struct plComponentId
{
  using StorageType = plUInt64;

  PL_DECLARE_ID_TYPE(plComponentId, 32, 8);

  static_assert(PL_COMPONENT_TYPE_INDEX_BITS > 0 && PL_COMPONENT_TYPE_INDEX_BITS <= 16);

  PL_ALWAYS_INLINE plComponentId(StorageType instanceIndex, plUInt8 uiGeneration, plUInt16 uiTypeId = 0, plUInt8 uiWorldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = static_cast<plUInt32>(instanceIndex);
    m_Generation = uiGeneration;
    m_TypeId = uiTypeId;
    m_WorldIndex = uiWorldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 32;
      StorageType m_Generation : 8;
      StorageType m_WorldIndex : PL_WORLD_INDEX_BITS;
      StorageType m_TypeId : PL_COMPONENT_TYPE_INDEX_BITS;
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
  PL_DECLARE_HANDLE_TYPE(plComponentHandle, plComponentId);

  friend class plWorld;
  friend class plComponentManagerBase;
  friend class plComponent;
};

/// \brief HashHelper implementation so component handles can be used as key in a hashtable.
template <>
struct plHashHelper<plComponentHandle>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(plComponentHandle value)
  {
    plComponentId id = value.GetInternalID();
    plUInt64 data = *reinterpret_cast<plUInt64*>(&id);
    return plHashHelper<plUInt64>::Hash(data);
  }

  PL_ALWAYS_INLINE static bool Equal(plComponentHandle a, plComponentHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for component handles.
PL_CORE_DLL void operator<<(plStreamWriter& inout_stream, const plComponentHandle& hValue);
PL_CORE_DLL void operator>>(plStreamReader& inout_stream, plComponentHandle& ref_hValue);

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plComponentHandle);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plComponentHandle);

/// \brief Internal flags of game objects or components.
struct plObjectFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    Dynamic = PL_BIT(0),                 ///< Usually detected automatically. A dynamic object will not cache render data across frames.
    ForceDynamic = PL_BIT(1),            ///< Set by the user to enforce the 'Dynamic' mode. Necessary when user code (or scripts) should change
                                         ///< objects, and the automatic detection cannot know that.
    ActiveFlag = PL_BIT(2),              ///< The object/component has the 'active flag' set
    ActiveState = PL_BIT(3),             ///< The object/component and all its parents have the active flag
    Initialized = PL_BIT(4),             ///< The object/component has been initialized
    Initializing = PL_BIT(5),            ///< The object/component is currently initializing. Used to prevent recursions during initialization.
    SimulationStarted = PL_BIT(6),       ///< OnSimulationStarted() has been called on the component
    SimulationStarting = PL_BIT(7),      ///< Used to prevent recursion during OnSimulationStarted()
    UnhandledMessageHandler = PL_BIT(8), ///< For components, when a message is not handled, a virtual function is called

    ChildChangesNotifications = PL_BIT(9),            ///< The object should send a notification message when children are added or removed.
    ComponentChangesNotifications = PL_BIT(10),       ///< The object should send a notification message when components are added or removed.
    StaticTransformChangesNotifications = PL_BIT(11), ///< The object should send a notification message if it is static and its transform changes.
    ParentChangesNotifications = PL_BIT(12),          ///< The object should send a notification message when the parent is changes.

    CreatedByPrefab = PL_BIT(13), ///< Such flagged objects and components are ignored during scene export (see plWorldWriter) and will be removed when a prefab needs to be re-instantiated.

    UserFlag0 = PL_BIT(24),
    UserFlag1 = PL_BIT(25),
    UserFlag2 = PL_BIT(26),
    UserFlag3 = PL_BIT(27),
    UserFlag4 = PL_BIT(28),
    UserFlag5 = PL_BIT(29),
    UserFlag6 = PL_BIT(30),
    UserFlag7 = PL_BIT(31),

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

    StorageType CreatedByPrefab : 1; //< 13

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

PL_DECLARE_FLAGS_OPERATORS(plObjectFlags);

/// \brief Specifies the mode of an object. This enum is only used in the editor.
///
/// \sa plObjectFlags
struct plObjectMode
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    Automatic,
    ForceDynamic,

    Default = Automatic
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plObjectMode);

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
struct PL_CORE_DLL plOnComponentFinishedAction
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.

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
  static void HandleDeleteObjectMsg(plMsgDeleteGameObject& ref_msg, plEnum<plOnComponentFinishedAction>& ref_action);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plOnComponentFinishedAction);

/// \brief Same as plOnComponentFinishedAction, but additionally includes 'Restart'
struct PL_CORE_DLL plOnComponentFinishedAction2
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,             ///< Nothing happens after the action is finished.
    DeleteComponent,  ///< The component deletes only itself, but its game object stays.
    DeleteGameObject, ///< When finished the component deletes its owner game object. If there are multiple objects with this mode, the component instead deletes itself, and only the last such component deletes the game object.
    Restart,          ///< When finished, restart from the beginning.

    Default = None
  };

  /// \brief See plOnComponentFinishedAction::HandleFinishedAction()
  static void HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction2::Enum action);

  /// \brief See plOnComponentFinishedAction::HandleDeleteObjectMsg()
  static void HandleDeleteObjectMsg(plMsgDeleteGameObject& ref_msg, plEnum<plOnComponentFinishedAction2>& ref_action);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plOnComponentFinishedAction2);

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

using plSpatialDataId = plGenericId<24, 8>;
class plSpatialDataHandle
{
  PL_DECLARE_HANDLE_TYPE(plSpatialDataHandle, plSpatialDataId);
};

#define PL_MAX_WORLD_MODULE_TYPES PL_MAX_COMPONENT_TYPES
using plWorldModuleTypeId = plUInt16;
static_assert(plMath::MaxValue<plWorldModuleTypeId>() >= PL_MAX_WORLD_MODULE_TYPES - 1);

using plComponentInitBatchId = plGenericId<24, 8>;
class plComponentInitBatchHandle
{
  PL_DECLARE_HANDLE_TYPE(plComponentInitBatchHandle, plComponentInitBatchId);
};
