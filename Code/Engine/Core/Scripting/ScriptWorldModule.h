#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using plScriptClassResourceHandle = plTypedResourceHandle<class plScriptClassResource>;
class plScriptInstance;

class PLASMA_CORE_DLL plScriptWorldModule : public plWorldModule
{
  PLASMA_DECLARE_WORLD_MODULE();
  PLASMA_ADD_DYNAMIC_REFLECTION(plScriptWorldModule, plWorldModule);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plScriptWorldModule);

public:
  plScriptWorldModule(plWorld* pWorld);
  ~plScriptWorldModule();

  virtual void Initialize() override;
  virtual void WorldClear() override;

  void AddUpdateFunctionToSchedule(const plAbstractFunctionProperty* pFunction, void* pInstance, plTime updateInterval, bool bOnlyWhenSimulating);
  void RemoveUpdateFunctionToSchedule(const plAbstractFunctionProperty* pFunction, void* pInstance);

  /// \name Coroutine Functions
  ///@{

  /// \brief Creates a new coroutine of pCoroutineType with the given name. If the creationMode prevents creating a new coroutine,
  /// this function will return an invalid handle and a nullptr in out_pCoroutine if there is already a coroutine running
  /// with the same name on the given instance.
  plScriptCoroutineHandle CreateCoroutine(const plRTTI* pCoroutineType, plStringView sName, plScriptInstance& inout_instance, plScriptCoroutineCreationMode::Enum creationMode, plScriptCoroutine*& out_pCoroutine);

  /// \brief Starts the coroutine with the given arguments. This will call the Start() function and then UpdateAndSchedule() once on the coroutine object.
  void StartCoroutine(plScriptCoroutineHandle hCoroutine, plArrayPtr<plVariant> arguments);

  /// \brief Stops and deletes the coroutine. This will call the Stop() function and will delete the coroutine on next update of the script world module.
  void StopAndDeleteCoroutine(plScriptCoroutineHandle hCoroutine);

  /// \brief Stops and deletes all coroutines with the given name on pInstance.
  void StopAndDeleteCoroutine(plStringView sName, plScriptInstance* pInstance);

  /// \brief Stops and deletes all coroutines on pInstance.
  void StopAndDeleteAllCoroutines(plScriptInstance* pInstance);

  /// \brief Returns whether the coroutine has already finished or has been stopped.
  bool IsCoroutineFinished(plScriptCoroutineHandle hCoroutine) const;

  ///@}
  /// \name Script Reload Functions
  ///@{

  using ReloadFunction = plDelegate<void()>;
  void AddScriptReloadFunction(plScriptClassResourceHandle hScript, ReloadFunction function);
  void RemoveScriptReloadFunction(plScriptClassResourceHandle hScript, ReloadFunction function);

  ///@}

  struct FunctionContext
  {
    enum Flags : plUInt8
    {
      None,
      OnlyWhenSimulating
    };

    plPointerWithFlags<const plAbstractFunctionProperty, 1> m_pFunctionAndFlags;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunctionAndFlags == other.m_pFunctionAndFlags && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const plWorldModule::UpdateContext& context);
  void ReloadScripts(const plWorldModule::UpdateContext& context);
  void ResourceEventHandler(const plResourceEvent& e);

  plIntervalScheduler<FunctionContext> m_Scheduler;

  plIdTable<plScriptCoroutineId, plUniquePtr<plScriptCoroutine>> m_RunningScriptCoroutines;
  plHashTable<plScriptInstance*, plSmallArray<plScriptCoroutineHandle, 8>> m_InstanceToScriptCoroutines;
  plDynamicArray<plUniquePtr<plScriptCoroutine>> m_DeadScriptCoroutines;

  using ReloadFunctionList = plHybridArray<ReloadFunction, 8>;
  plHashTable<plScriptClassResourceHandle, ReloadFunctionList> m_ReloadFunctions;
  plHashSet<plScriptClassResourceHandle> m_NeedReload;
  ReloadFunctionList m_TempReloadFunctions;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct plHashHelper<plScriptWorldModule::FunctionContext>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plScriptWorldModule::FunctionContext& value)
  {
    plUInt32 hash = plHashHelper<const void*>::Hash(value.m_pFunctionAndFlags);
    hash = plHashingUtils::CombineHashValues32(hash, plHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const plScriptWorldModule::FunctionContext& a, const plScriptWorldModule::FunctionContext& b) { return a == b; }
};
