#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class plWorld;

class PLASMA_CORE_DLL plScriptRTTI : public plRTTI, public plRefCountingImpl
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plScriptRTTI);

public:
  enum
  {
    NumInplaceFunctions = 7
  };

  using FunctionList = plSmallArray<plUniquePtr<plAbstractFunctionProperty>, NumInplaceFunctions>;
  using MessageHandlerList = plSmallArray<plUniquePtr<plAbstractMessageHandler>, NumInplaceFunctions>;

  plScriptRTTI(plStringView sName, const plRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~plScriptRTTI();

  const plAbstractFunctionProperty* GetFunctionByIndex(plUInt32 uiIndex) const;

private:
  plString m_sTypeNameStorage;
  FunctionList m_FunctionStorage;
  MessageHandlerList m_MessageHandlerStorage;
  plSmallArray<const plAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  plSmallArray<plAbstractMessageHandler*, NumInplaceFunctions> m_MessageHandlerRawPtrs;
};

class PLASMA_CORE_DLL plScriptFunctionProperty : public plAbstractFunctionProperty
{
public:
  plScriptFunctionProperty(plStringView sName);
  ~plScriptFunctionProperty();

private:
  plHashedString m_sPropertyNameStorage;
};

struct plScriptMessageDesc
{
  const plRTTI* m_pType = nullptr;
  plArrayPtr<const plAbstractProperty* const> m_Properties;
};

class PLASMA_CORE_DLL plScriptMessageHandler : public plAbstractMessageHandler
{
public:
  plScriptMessageHandler(const plScriptMessageDesc& desc);
  ~plScriptMessageHandler();

  void FillMessagePropertyValues(const plMessage& msg, plDynamicArray<plVariant>& out_propertyValues);

private:
  plArrayPtr<const plAbstractProperty* const> m_Properties;
};

class PLASMA_CORE_DLL plScriptInstance
{
public:
  plScriptInstance(plReflectedClass& inout_owner, plWorld* pWorld);
  virtual ~plScriptInstance() = default;

  plReflectedClass& GetOwner() { return m_Owner; }
  plWorld* GetWorld() { return m_pWorld; }

  virtual void ApplyParameters(const plArrayMap<plHashedString, plVariant>& parameters) = 0;

private:
  plReflectedClass& m_Owner;
  plWorld* m_pWorld = nullptr;
};
