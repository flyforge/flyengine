#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/ObjectSelection.h>

plObjectSelection::plObjectSelection()
{
  m_pWorld = nullptr;
}

void plObjectSelection::SetWorld(plWorld* pWorld)
{
  PL_ASSERT_DEV((m_pWorld == nullptr) || (m_pWorld == pWorld) || m_Objects.IsEmpty(), "You cannot change the world for this selection.");

  m_pWorld = pWorld;
}

void plObjectSelection::RemoveDeadObjects()
{
  PL_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  for (plUInt32 i = m_Objects.GetCount(); i > 0; --i)
  {
    plGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAtAndCopy(i - 1); // keep the order
    }
  }
}

void plObjectSelection::AddObject(plGameObjectHandle hObject, bool bDontAddTwice)
{
  PL_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  // only insert valid objects
  plGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  if (m_Objects.IndexOf(hObject) != plInvalidIndex)
    return;

  m_Objects.PushBack(hObject);
}

bool plObjectSelection::RemoveObject(plGameObjectHandle hObject)
{
  return m_Objects.RemoveAndCopy(hObject);
}

void plObjectSelection::ToggleSelection(plGameObjectHandle hObject)
{
  for (plUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAtAndCopy(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}


