#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/HashedString.h>

class plWorld;

class PLASMA_CORE_DLL plWorldModule : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plWorldModule, plReflectedClass);

protected:
  plWorldModule(plWorld* pWorld);
  virtual ~plWorldModule();

public:
  /// \brief Returns the corresponding world to this module.
  plWorld* GetWorld();

  /// \brief Returns the corresponding world to this module.
  const plWorld* GetWorld() const;

  /// \brief Same as GetWorld()->GetIndex(). Needed to break circular include dependencies.
  plUInt32 GetWorldIndex() const;

protected:
  friend class plWorld;
  friend class plInternal::WorldData;
  friend class plMemoryUtils;

  struct UpdateContext
  {
    plUInt32 m_uiFirstComponentIndex = 0;
    plUInt32 m_uiComponentCount = 0;
  };

  /// \brief Update function delegate.
  typedef plDelegate<void(const UpdateContext&)> UpdateFunction;

  /// \brief Description of an update function that can be registered at the world.
  struct UpdateFunctionDesc
  {
    struct Phase
    {
      typedef plUInt8 StorageType;

      enum Enum
      {
        PreAsync,
        Async,
        PostAsync,
        PostTransform,
        COUNT,

        Default = PreAsync
      };
    };

    UpdateFunctionDesc(const UpdateFunction& function, plStringView szFunctionName)
      : m_Function(function)
    {
      m_sFunctionName.Assign(szFunctionName);
    }

    UpdateFunction m_Function;                    ///< Delegate to the actual update function.
    plHashedString m_sFunctionName;               ///< Name of the function. Use the PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC macro to create a description
                                                  ///< with the correct name.
    plHybridArray<plHashedString, 4> m_DependsOn; ///< Array of other functions on which this function depends on. This function will be
                                                  ///< called after all its dependencies have been called.
    plEnum<Phase> m_Phase;                        ///< The update phase in which this update function should be called. See plWorld for a description on the
                                                  ///< different phases.
    bool m_bOnlyUpdateWhenSimulating = false;     ///< The update function is only called when the world simulation is enabled.
    plUInt16 m_uiGranularity = 0;                 ///< The granularity in which batch updates should happen during the asynchronous phase. Has to be 0 for
                                                  ///< synchronous functions.
    float m_fPriority = 0.0f;                     ///< Higher priority (higher number) means that this function is called earlier than a function with lower priority.
  };

  /// \brief Registers the given update function at the world.
  void RegisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief De-registers the given update function from the world. Note that only the m_Function and the m_Phase of the description have to
  /// be valid for de-registration.
  void DeregisterUpdateFunction(const UpdateFunctionDesc& desc);

  /// \brief Returns the allocator used by the world.
  plAllocatorBase* GetAllocator();

  /// \brief Returns the block allocator used by the world.
  plInternal::WorldLargeBlockAllocator* GetBlockAllocator();

  /// \brief Returns whether the world simulation is enabled.
  bool GetWorldSimulationEnabled() const;

protected:
  /// \brief This method is called after the constructor. A derived type can override this method to do initialization work. Typically this
  /// is the method where updates function are registered.
  virtual void Initialize() {}

  /// \brief This method is called before the destructor. A derived type can override this method to do deinitialization work.
  virtual void Deinitialize() {}

  /// \brief This method is called at the start of the next world update when the world is simulated. This method will be called after the
  /// initialization method.
  virtual void OnSimulationStarted() {}

  /// \brief Called by plWorld::Clear(). Can be used to clear cached data when a world is completely cleared of objects (but not deleted).
  virtual void WorldClear() {}

  plWorld* m_pWorld;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to get component type ids and create new instances of world modules from rtti.
class PLASMA_CORE_DLL plWorldModuleFactory
{
public:
  static plWorldModuleFactory* GetInstance();

  template <typename ModuleType, typename RTTIType>
  plWorldModuleTypeId RegisterWorldModule();

  /// \brief Returns the module type id to the given rtti module/component type.
  plWorldModuleTypeId GetTypeId(const plRTTI* pRtti);

  /// \brief Creates a new instance of the world module with the given type id and world.
  plWorldModule* CreateWorldModule(plUInt16 typeId, plWorld* pWorld);

  /// \brief Register explicit a mapping of a world module interface to a specific implementation.
  ///
  /// This is necessary if there are multiple implementations of the same interface.
  /// If there is only one implementation for an interface this implementation is registered automatically.
  void RegisterInterfaceImplementation(plStringView sInterfaceName, plStringView sImplementationName);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, WorldModuleFactory);

  typedef plWorldModule* (*CreatorFunc)(plAllocatorBase*, plWorld*);

  plWorldModuleFactory();
  plWorldModuleTypeId RegisterWorldModule(const plRTTI* pRtti, CreatorFunc creatorFunc);

  static void PluginEventHandler(const plPluginEvent& EventData);
  void FillBaseTypeIds();
  void ClearUnloadedTypeToIDs();
  void AdjustBaseTypeId(const plRTTI* pParentRtti, const plRTTI* pRtti, plUInt16 uiParentTypeId);

  plHashTable<const plRTTI*, plWorldModuleTypeId> m_TypeToId;

  struct CreatorFuncContext
  {
    PLASMA_DECLARE_POD_TYPE();

    CreatorFunc m_Func;
    const plRTTI* m_pRtti;
  };

  plDynamicArray<CreatorFuncContext> m_CreatorFuncs;

  plHashTable<plString, plString> m_InterfaceImplementations;
};

/// \brief Add this macro to the declaration of your module type.
#define PLASMA_DECLARE_WORLD_MODULE()                                           \
public:                                                                     \
  static PLASMA_ALWAYS_INLINE plWorldModuleTypeId TypeId() { return s_TypeId; } \
                                                                            \
private:                                                                    \
  static plWorldModuleTypeId s_TypeId;

/// \brief Implements the given module type. Add this macro to a cpp outside of the type declaration.
#define PLASMA_IMPLEMENT_WORLD_MODULE(moduleType) \
  plWorldModuleTypeId moduleType::s_TypeId = plWorldModuleFactory::GetInstance()->RegisterWorldModule<moduleType, moduleType>();

/// \brief Helper macro to create an update function description with proper name
#define PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(func, instance) plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&func, instance), #func)

#include <Core/World/Implementation/WorldModule_inl.h>
