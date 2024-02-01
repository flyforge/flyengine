#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plExposedParameter, plNoBase, 2, plRTTIDefaultAllocator<plExposedParameter>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("Type", m_sType),
    PL_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    PL_ARRAY_MEMBER_PROPERTY("Attributes", m_Attributes)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

plExposedParameter::plExposedParameter()
= default;

plExposedParameter::~plExposedParameter()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
  }
}

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedParameters, 3, plRTTIDefaultAllocator<plExposedParameters>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExposedParameters::plExposedParameters() = default;

plExposedParameters::~plExposedParameters()
{
  for (auto pAttr : m_Parameters)
  {
    plGetStaticRTTI<plExposedParameter>()->GetAllocator()->Deallocate(pAttr);
  }
}

const plExposedParameter* plExposedParameters::Find(const char* szParamName) const
{
  const plExposedParameter* const* pParam =
    std::find_if(cbegin(m_Parameters), cend(m_Parameters), [szParamName](const plExposedParameter* pParam) { return pParam->m_sName == szParamName; });
  return pParam != cend(m_Parameters) ? *pParam : nullptr;
}
