#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

plVisualScriptInstance::plVisualScriptInstance(plReflectedClass& inout_owner, plWorld* pWorld, const plSharedPtr<plVisualScriptDataStorage>& pConstantDataStorage, const plSharedPtr<const plVisualScriptDataDescription>& pInstanceDataDesc, const plSharedPtr<plVisualScriptInstanceDataMapping>& pInstanceDataMapping)
  : plScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
  , m_pInstanceDataMapping(pInstanceDataMapping)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_pInstanceDataStorage = PLASMA_DEFAULT_NEW(plVisualScriptDataStorage, pInstanceDataDesc);
    m_pInstanceDataStorage->AllocateStorage();

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_pInstanceDataStorage->SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void plVisualScriptInstance::ApplyParameters(const plArrayMap<plHashedString, plVariant>& parameters)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  for (auto it : parameters)
  {
    plVisualScriptInstanceData* pInstanceData = nullptr;
    if (m_pInstanceDataMapping->m_Content.TryGetValue(it.key, pInstanceData))
    {
      plResult conversionStatus = PLASMA_FAILURE;
      plVariantType::Enum targetType = plVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

      plVariant convertedValue = it.value.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        plLog::Error("Can't apply script parameter '{}' because the given value of type '{}' can't be converted the expected target type '{}'", it.key, it.value.GetType(), targetType);
        continue;
      }

      m_pInstanceDataStorage->SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
    }
  }
}
