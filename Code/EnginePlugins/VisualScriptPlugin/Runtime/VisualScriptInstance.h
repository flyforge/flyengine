#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptInstance : public plScriptInstance
{
public:
  plVisualScriptInstance(plReflectedClass& inout_owner, plWorld* pWorld, const plSharedPtr<plVisualScriptDataStorage>& pConstantDataStorage, const plSharedPtr<const plVisualScriptDataDescription>& pInstanceDataDesc, const plSharedPtr<plVisualScriptInstanceDataMapping>& pInstanceDataMapping);

  virtual void ApplyParameters(const plArrayMap<plHashedString, plVariant>& parameters) override;

  plVisualScriptDataStorage* GetConstantDataStorage() { return m_pConstantDataStorage.Borrow(); }
  plVisualScriptDataStorage* GetInstanceDataStorage() { return m_pInstanceDataStorage.Borrow(); }

private:
  plSharedPtr<plVisualScriptDataStorage> m_pConstantDataStorage;
  plUniquePtr<plVisualScriptDataStorage> m_pInstanceDataStorage;
  plSharedPtr<plVisualScriptInstanceDataMapping> m_pInstanceDataMapping;
};
