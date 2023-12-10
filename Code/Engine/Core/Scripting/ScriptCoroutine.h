#pragma once

#include <Core/CoreDLL.h>

#include <Core/Scripting/ScriptRTTI.h>

class plScriptWorldModule;

using plScriptCoroutineId = plGenericId<20, 12>;

/// \brief A handle to a script coroutine which can be used to determine whether a coroutine is still running
/// even after the underlying coroutine object has already been deleted.
///
/// \sa plScriptWorldModule::CreateCoroutine, plScriptWorldModule::IsCoroutineFinished
struct plScriptCoroutineHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plScriptCoroutineHandle, plScriptCoroutineId);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plScriptCoroutineHandle);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plScriptCoroutineHandle);

/// \brief Base class of script coroutines.
///
/// A coroutine is a function that can be distributed over multiple frames and behaves similar to a mini state machine.
/// That is why coroutines are actually individual objects that keep track of their state rather than simple functions.
/// At first Start() is called with the arguments of the coroutine followed by one or multiple calls to Update().
/// The return value of the Update() function determines whether the Update() function should be called again next frame
/// or at latest after the specified delay. If the Update() function returns completed the Stop() function is called and the
/// coroutine object is destroyed.
/// The plScriptWorldModule is used to create and manage coroutine objects. The coroutine can then either be started and
/// scheduled automatically by calling plScriptWorldModule::StartCoroutine or the
/// Start/Stop/Update function is called manually if the coroutine is embedded as a subroutine in another coroutine.
class PLASMA_CORE_DLL plScriptCoroutine
{
public:
  plScriptCoroutine();
  virtual ~plScriptCoroutine();

  plScriptCoroutineHandle GetHandle() { return plScriptCoroutineHandle(m_Id); }

  plStringView GetName() const { return m_sName; }

  plScriptInstance* GetScriptInstance() { return m_pInstance; }
  const plScriptInstance* GetScriptInstance() const { return m_pInstance; }

  plScriptWorldModule* GetScriptWorldModule() { return m_pOwnerModule; }
  const plScriptWorldModule* GetScriptWorldModule() const { return m_pOwnerModule; }

  struct Result
  {
    struct State
    {
      using StorageType = plUInt8;

      enum Enum
      {
        Invalid,
        Running,
        Completed,
        Failed,

        Default = Invalid,
      };
    };

    static PLASMA_ALWAYS_INLINE Result Running(plTime maxDelay = plTime::MakeZero()) { return {State::Running, maxDelay}; }
    static PLASMA_ALWAYS_INLINE Result Completed() { return {State::Completed}; }
    static PLASMA_ALWAYS_INLINE Result Failed() { return {State::Failed}; }

    plEnum<State> m_State;
    plTime m_MaxDelay = plTime::MakeZero();
  };

  virtual void Start(plArrayPtr<plVariant> arguments) = 0;
  virtual void Stop() {}
  virtual Result Update(plTime deltaTimeSinceLastUpdate) = 0;

  void UpdateAndSchedule(plTime deltaTimeSinceLastUpdate = plTime::MakeZero());

private:
  friend class plScriptWorldModule;
  void Initialize(plScriptCoroutineId id, plStringView sName, plScriptInstance& inout_instance, plScriptWorldModule& inout_ownerModule);
  void Deinitialize();

  static const plAbstractFunctionProperty* GetUpdateFunctionProperty();

  plScriptCoroutineId m_Id;
  plHashedString m_sName;
  plScriptInstance* m_pInstance = nullptr;
  plScriptWorldModule* m_pOwnerModule = nullptr;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plScriptCoroutine);

/// \brief Base class of coroutines which are implemented in C++ to allow automatic unpacking of the arguments from variants
template <typename Derived, class... Args>
class plTypedScriptCoroutine : public plScriptCoroutine
{
private:
  template <std::size_t... I>
  PLASMA_ALWAYS_INLINE void StartImpl(plArrayPtr<plVariant> arguments, std::index_sequence<I...>)
  {
    static_cast<Derived*>(this)->Start(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Start(plArrayPtr<plVariant> arguments) override
  {
    StartImpl(arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};

/// \brief Mode that decides what should happen if a new coroutine is created while there is already another coroutine running with the same name
/// on a given instance.
///
/// \sa plScriptWorldModule::CreateCoroutine
struct plScriptCoroutineCreationMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    StopOther,     ///< Stop the other coroutine before creating a new one with the same name
    DontCreateNew, ///< Don't create a new coroutine if there is already one running with the same name
    AllowOverlap,  ///< Allow multiple overlapping coroutines with the same name

    Default = StopOther
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plScriptCoroutineCreationMode);

/// \brief A coroutine type that stores a custom allocator.
///
/// The custom allocator allows to pass more data to the created coroutine object than the default allocator.
/// E.g. this is used to pass the visual script graph to a visual script coroutine without the user needing to know
/// that the coroutine is actually implemented in visual script.
class PLASMA_CORE_DLL plScriptCoroutineRTTI : public plRTTI, public plRefCountingImpl
{
public:
  plScriptCoroutineRTTI(plStringView sName, plUniquePtr<plRTTIAllocator>&& pAllocator);
  ~plScriptCoroutineRTTI();

private:
  plString m_sTypeNameStorage;
  plUniquePtr<plRTTIAllocator> m_pAllocatorStorage;
};

/// \brief A function property that creates an instance of the given coroutine type and starts it immediately.
class PLASMA_CORE_DLL plScriptCoroutineFunctionProperty : public plScriptFunctionProperty
{
public:
  plScriptCoroutineFunctionProperty(plStringView sName, const plSharedPtr<plScriptCoroutineRTTI>& pType, plScriptCoroutineCreationMode::Enum creationMode);
  ~plScriptCoroutineFunctionProperty();

  virtual plFunctionType::Enum GetFunctionType() const override { return plFunctionType::Member; }
  virtual const plRTTI* GetReturnType() const override { return nullptr; }
  virtual plBitflags<plPropertyFlags> GetReturnFlags() const override { return plPropertyFlags::Void; }
  virtual plUInt32 GetArgumentCount() const override { return 0; }
  virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const override { return nullptr; }
  virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const override { return plPropertyFlags::Void; }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override;

protected:
  plSharedPtr<plScriptCoroutineRTTI> m_pType;
  plEnum<plScriptCoroutineCreationMode> m_CreationMode;
};

/// \brief A message handler that creates an instance of the given coroutine type and starts it immediately.
class PLASMA_CORE_DLL plScriptCoroutineMessageHandler : public plScriptMessageHandler
{
public:
  plScriptCoroutineMessageHandler(plStringView sName, const plScriptMessageDesc& desc, const plSharedPtr<plScriptCoroutineRTTI>& pType, plScriptCoroutineCreationMode::Enum creationMode);
  ~plScriptCoroutineMessageHandler();

  static void Dispatch(plAbstractMessageHandler* pSelf, void* pInstance, plMessage& ref_msg);

protected:
  plHashedString m_sName;
  plSharedPtr<plScriptCoroutineRTTI> m_pType;
  plEnum<plScriptCoroutineCreationMode> m_CreationMode;
};

/// \brief HashHelper implementation so coroutine handles can be used as key in a hash table. Also needed to store in a variant.
template <>
struct plHashHelper<plScriptCoroutineHandle>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plScriptCoroutineHandle value) { return plHashHelper<plUInt32>::Hash(value.GetInternalID().m_Data); }

  PLASMA_ALWAYS_INLINE static bool Equal(plScriptCoroutineHandle a, plScriptCoroutineHandle b) { return a == b; }
};

/// \brief Currently not implemented as it is not needed for coroutine handles.
PLASMA_ALWAYS_INLINE void operator<<(plStreamWriter& inout_stream, const plScriptCoroutineHandle& hValue) {}
PLASMA_ALWAYS_INLINE void operator>>(plStreamReader& inout_stream, plScriptCoroutineHandle& ref_hValue) {}
