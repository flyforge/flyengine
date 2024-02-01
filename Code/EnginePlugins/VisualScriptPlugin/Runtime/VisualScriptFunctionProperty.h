#pragma once

#include <VisualScriptPlugin/Runtime/VisualScript.h>

class PL_VISUALSCRIPTPLUGIN_DLL plVisualScriptFunctionProperty : public plScriptFunctionProperty
{
public:
  plVisualScriptFunctionProperty(plStringView sName, const plSharedPtr<const plVisualScriptGraphDescription>& pDesc);
  ~plVisualScriptFunctionProperty();

  virtual plFunctionType::Enum GetFunctionType() const override { return plFunctionType::Member; }
  virtual const plRTTI* GetReturnType() const override { return nullptr; }
  virtual plBitflags<plPropertyFlags> GetReturnFlags() const override { return plPropertyFlags::Void; }
  virtual plUInt32 GetArgumentCount() const override { return 0; }
  virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const override { return nullptr; }
  virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const override { return plPropertyFlags::Void; }

  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const override;

private:
  plSharedPtr<const plVisualScriptGraphDescription> m_pDesc;
  mutable plVisualScriptDataStorage m_LocalDataStorage;
};

class PL_VISUALSCRIPTPLUGIN_DLL plVisualScriptMessageHandler : public plScriptMessageHandler
{
public:
  plVisualScriptMessageHandler(const plScriptMessageDesc& desc, const plSharedPtr<const plVisualScriptGraphDescription>& pDesc);
  ~plVisualScriptMessageHandler();

  static void Dispatch(plAbstractMessageHandler* pSelf, void* pInstance, plMessage& ref_msg);

private:
  plSharedPtr<const plVisualScriptGraphDescription> m_pDesc;
  mutable plVisualScriptDataStorage m_LocalDataStorage;
};
