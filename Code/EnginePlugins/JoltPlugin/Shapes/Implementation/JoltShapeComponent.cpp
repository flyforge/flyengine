#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plJoltShapeComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Shapes"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plJoltShapeComponent::plJoltShapeComponent() = default;
plJoltShapeComponent::~plJoltShapeComponent() = default;

void plJoltShapeComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeComponent::OnDeactivated()
{
  if (m_uiUserDataIndex != plInvalidIndex)
  {
    plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

const plJoltUserData* plJoltShapeComponent::GetUserData()
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  if (m_uiUserDataIndex != plInvalidIndex)
  {
    return &pModule->GetUserData(m_uiUserDataIndex);
  }
  else
  {
    plJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    return pUserData;
  }
}

plUInt32 plJoltShapeComponent::GetUserDataIndex()
{
  GetUserData();
  return m_uiUserDataIndex;
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeComponent);

