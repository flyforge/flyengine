#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_IMPLEMENT_SINGLETON(plPropertyMetaState);

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyMetaState)

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plPropertyMetaState);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (plPropertyMetaState::GetSingleton())
    {
      auto ptr = plPropertyMetaState::GetSingleton();
      PLASMA_DEFAULT_DELETE(ptr);
    }
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plPropertyMetaState::plPropertyMetaState()
  : m_SingletonRegistrar(this)
{
}

void plPropertyMetaState::GetTypePropertiesState(const plDocumentObject* pObject, plMap<plString, plPropertyUiState>& out_propertyStates)
{
  plPropertyMetaStateEvent eventData;
  eventData.m_pPropertyStates = &out_propertyStates;
  eventData.m_pObject = pObject;

  m_Events.Broadcast(eventData);
}

void plPropertyMetaState::GetTypePropertiesState(const plHybridArray<plPropertySelection, 8>& items, plMap<plString, plPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp.Clear();
    GetTypePropertiesState(sel.m_pObject, m_Temp);

    for (auto it = m_Temp.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility = plMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}

void plPropertyMetaState::GetContainerElementsState(const plDocumentObject* pObject, const char* szProperty, plHashTable<plVariant, plPropertyUiState>& out_propertyStates)
{
  plContainerElementMetaStateEvent eventData;
  eventData.m_pContainerElementStates = &out_propertyStates;
  eventData.m_pObject = pObject;
  eventData.m_szProperty = szProperty;

  m_ContainerEvents.Broadcast(eventData);
}

void plPropertyMetaState::GetContainerElementsState(const plHybridArray<plPropertySelection, 8>& items, const char* szProperty, plHashTable<plVariant, plPropertyUiState>& out_propertyStates)
{
  for (const auto& sel : items)
  {
    m_Temp2.Clear();
    GetContainerElementsState(sel.m_pObject, szProperty, m_Temp2);

    for (auto it = m_Temp2.GetIterator(); it.IsValid(); ++it)
    {
      auto& curState = out_propertyStates[it.Key()];

      curState.m_Visibility = plMath::Max(curState.m_Visibility, it.Value().m_Visibility);
      curState.m_sNewLabelText = it.Value().m_sNewLabelText;
    }
  }
}
