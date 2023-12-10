#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

plHashTable<plUInt32, plTypeScriptBinding::PropertyBinding> plTypeScriptBinding::s_BoundProperties;

static int __CPP_ComponentProperty_get(duk_context* pDuk);
static int __CPP_ComponentProperty_set(duk_context* pDuk);

plResult plTypeScriptBinding::Init_PropertyBinding()
{
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_get", __CPP_ComponentProperty_get, 2);
  m_Duk.RegisterGlobalFunction("__CPP_ComponentProperty_set", __CPP_ComponentProperty_set, 3);

  return PLASMA_SUCCESS;
}

plUInt32 plTypeScriptBinding::ComputePropertyBindingHash(const plRTTI* pType, const plAbstractMemberProperty* pMember)
{
  plStringBuilder sFuncName;

  sFuncName.Set(pType->GetTypeName(), "::", pMember->GetPropertyName());

  return plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sFuncName.GetData()));
}

void plTypeScriptBinding::SetupRttiPropertyBindings()
{
  if (!s_BoundProperties.IsEmpty())
    return;

  plRTTI::ForEachDerivedType<plComponent>(
    [&](const plRTTI* pRtti) {
      for (const plAbstractProperty* pProp : pRtti->GetProperties())
      {
        if (pProp->GetCategory() != plPropertyCategory::Member)
          continue;

        const plUInt32 uiHash = ComputePropertyBindingHash(pRtti, static_cast<const plAbstractMemberProperty*>(pProp));
        PLASMA_ASSERT_DEV(!s_BoundProperties.Contains(uiHash), "Hash collision for bound property name!");

        s_BoundProperties[uiHash].m_pMember = static_cast<const plAbstractMemberProperty*>(pProp);
      }
    });
}

void plTypeScriptBinding::GeneratePropertiesCode(plStringBuilder& out_Code, const plRTTI* pRtti)
{
  plStringBuilder sProp;

  for (const plAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    auto pMember = static_cast<const plAbstractMemberProperty*>(pProp);
    const plRTTI* pPropType = pProp->GetSpecificType();

    const plUInt32 uiHash = ComputePropertyBindingHash(pRtti, pMember);

    plStringBuilder sTypeName;

    sTypeName = TsType(pPropType);
    if (sTypeName.IsEmpty())
      continue;

    sProp.Format("  get {0}(): {1} { return __CPP_ComponentProperty_get(this, {2}); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
    out_Code.Append(sProp.GetView());

    sProp.Format("  set {0}(value: {1}) { __CPP_ComponentProperty_set(this, {2}, value); }\n", pMember->GetPropertyName(), sTypeName, uiHash);
    out_Code.Append(sProp.GetView());
  }
}

const plTypeScriptBinding::PropertyBinding* plTypeScriptBinding::FindPropertyBinding(plUInt32 uiHash)
{
  const PropertyBinding* pBinding = nullptr;
  s_BoundProperties.TryGetValue(uiHash, pBinding);
  return pBinding;
}

int __CPP_ComponentProperty_get(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  const plUInt32 uiHash = duk.GetUIntValue(1);

  const plTypeScriptBinding::PropertyBinding* pBinding = plTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    plLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const plVariant value = plReflectionUtils::GetMemberPropertyValue(pBinding->m_pMember, pComponent);
  plTypeScriptBinding::PushVariant(duk, value);
  return duk.ReturnCustom();
}

int __CPP_ComponentProperty_set(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  const plUInt32 uiHash = duk.GetUIntValue(1);

  const plTypeScriptBinding::PropertyBinding* pBinding = plTypeScriptBinding::FindPropertyBinding(uiHash);

  if (pBinding == nullptr)
  {
    plLog::Error("Bound property with hash {} not found.", uiHash);
    return duk.ReturnVoid();
  }

  const plVariant value = plTypeScriptBinding::GetVariant(duk, 2, pBinding->m_pMember->GetSpecificType());

  plReflectionUtils::SetMemberPropertyValue(pBinding->m_pMember, pComponent, value);

  return duk.ReturnVoid();
}
