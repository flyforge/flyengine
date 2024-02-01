#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/VariantAdapter.h>


template <class R, class... Args>
class plTypedFunctionProperty : public plAbstractFunctionProperty
{
public:
  plTypedFunctionProperty(const char* szPropertyName)
    : plAbstractFunctionProperty(szPropertyName)
  {
  }

  virtual const plRTTI* GetReturnType() const override { return plGetStaticRTTI<typename plCleanType<R>::RttiType>(); }
  virtual plBitflags<plPropertyFlags> GetReturnFlags() const override { return plPropertyFlags::GetParameterFlags<R>(); }

  virtual plUInt32 GetArgumentCount() const override { return sizeof...(Args); }

  template <std::size_t... I>
  const plRTTI* GetParameterTypeImpl(plUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static const plRTTI* params[] = {plGetStaticRTTI<typename plCleanType<typename getArgument<I, Args...>::Type>::RttiType>()..., nullptr};
    return params[uiParamIndex];
  }

  virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const override
  {
    return GetParameterTypeImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }

  template <std::size_t... I>
  plBitflags<plPropertyFlags> GetParameterFlagsImpl(plUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static plBitflags<plPropertyFlags> params[] = {
      plPropertyFlags::GetParameterFlags<typename getArgument<I, Args...>::Type>()..., plPropertyFlags::Void};
    return params[uiParamIndex];
  }

  virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const override
  {
    return GetParameterFlagsImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <typename FUNC>
class plFunctionProperty
{
};

template <class CLASS, class R, class... Args>
class plFunctionProperty<R (CLASS::*)(Args...)> : public plTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (CLASS::*)(Args...);

  plFunctionProperty(const char* szPropertyName, TargetFunction func)
    : plTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual plFunctionType::Enum GetFunctionType() const override
  {
    return plFunctionType::Member;
  }

  template <std::size_t... I>
  PL_FORCE_INLINE void ExecuteImpl(void* pInstance, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = static_cast<CLASS*>(pInstance);
    if constexpr (std::is_same<R, void>::value)
    {
      (pTargetInstance->*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
      out_returnValue = plVariant();
    }
    else
    {
      plVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
      returnWrapper = (pTargetInstance->*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    }
  }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override
  {
    ExecuteImpl(pInstance, out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};

template <class CLASS, class R, class... Args>
class plFunctionProperty<R (CLASS::*)(Args...) const> : public plTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (CLASS::*)(Args...) const;

  plFunctionProperty(const char* szPropertyName, TargetFunction func)
    : plTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
    this->AddFlags(plPropertyFlags::Const);
  }

  virtual plFunctionType::Enum GetFunctionType() const override
  {
    return plFunctionType::Member;
  }

  template <std::size_t... I>
  PL_FORCE_INLINE void ExecuteImpl(const void* pInstance, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    const CLASS* pTargetInstance = static_cast<const CLASS*>(pInstance);
    if constexpr (std::is_same<R, void>::value)
    {
      (pTargetInstance->*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
      out_returnValue = plVariant();
    }
    else
    {
      plVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
      returnWrapper = (pTargetInstance->*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    }
  }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override
  {
    ExecuteImpl(pInstance, out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};

template <class R, class... Args>
class plFunctionProperty<R (*)(Args...)> : public plTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (*)(Args...);

  plFunctionProperty(const char* szPropertyName, TargetFunction func)
    : plTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual plFunctionType::Enum GetFunctionType() const override { return plFunctionType::StaticMember; }

  template <std::size_t... I>
  void ExecuteImpl(plTraitInt<1>, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    (*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    out_returnValue = plVariant();
  }

  template <std::size_t... I>
  void ExecuteImpl(plTraitInt<0>, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    plVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
    returnWrapper = (*m_Function)(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override
  {
    ExecuteImpl(plTraitInt<std::is_same<R, void>::value>(), out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};


template <class CLASS, class... Args>
class plConstructorFunctionProperty : public plTypedFunctionProperty<CLASS*, Args...>
{
public:
  plConstructorFunctionProperty()
    : plTypedFunctionProperty<CLASS*, Args...>("Constructor")
  {
  }

  virtual plFunctionType::Enum GetFunctionType() const override { return plFunctionType::Constructor; }

  template <std::size_t... I>
  void ExecuteImpl(plTraitInt<1>, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    out_returnValue = CLASS(plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // returnValue = CLASS(static_cast<typename getArgument<I, Args...>::Type>(plVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
  }

  template <std::size_t... I>
  void ExecuteImpl(plTraitInt<0>, plVariant& out_returnValue, plArrayPtr<plVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pInstance = PL_DEFAULT_NEW(CLASS, plVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // CLASS* pInstance = PL_DEFAULT_NEW(CLASS, static_cast<typename getArgument<I, Args...>::Type>(plVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
    out_returnValue = pInstance;
  }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override
  {
    ExecuteImpl(plTraitInt<plIsStandardType<CLASS>::value>(), out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};
