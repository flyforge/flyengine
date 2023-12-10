#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/LongOps/LongOpsAdapter.h>

PLASMA_IMPLEMENT_SINGLETON(plLongOpsAdapter);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, LongOpsAdapter)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager",
    "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plLongOpsAdapter);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (plLongOpsAdapter::GetSingleton())
    {
      auto ptr = plLongOpsAdapter::GetSingleton();
      PLASMA_DEFAULT_DELETE(ptr);
    }
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plLongOpsAdapter::plLongOpsAdapter()
  : m_SingletonRegistrar(this)
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plLongOpsAdapter::DocumentManagerEventHandler, this));
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
}

plLongOpsAdapter::~plLongOpsAdapter()
{
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plLongOpsAdapter::DocumentManagerEventHandler, this));
}

void plLongOpsAdapter::DocumentManagerEventHandler(const plDocumentManager::Event& e)
{
  if (e.m_Type == plDocumentManager::Event::Type::DocumentOpened)
  {
    const plRTTI* pRttiScene = plRTTI::FindTypeByName("plSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      CheckAllTypes();

      e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plLongOpsAdapter::StructureEventHandler, this));

      ObjectAdded(e.m_pDocument->GetObjectManager()->GetRootObject());
    }
  }

  if (e.m_Type == plDocumentManager::Event::Type::DocumentClosing)
  {
    const plRTTI* pRttiScene = plRTTI::FindTypeByName("plSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      plLongOpControllerManager::GetSingleton()->CancelAndRemoveAllOpsForDocument(e.m_pDocument->GetGuid());

      e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plLongOpsAdapter::StructureEventHandler, this));
    }
  }
}

void plLongOpsAdapter::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == plDocumentObjectStructureEvent::Type::AfterObjectAdded)
  {
    ObjectAdded(e.m_pObject);
  }

  if (e.m_EventType == plDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    ObjectRemoved(e.m_pObject);
  }
}

void plLongOpsAdapter::PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e)
{
  const bool bExists = m_TypesWithLongOps.Contains(e.m_pChangedType);

  if (bExists && e.m_Type == plPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    m_TypesWithLongOps.Remove(e.m_pChangedType);
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to remove
  }

  if (!bExists && e.m_Type == plPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetAttributeByType<plLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(e.m_pChangedType);
      // if this ever becomes relevant:
      // iterate over all open documents and figure out which long ops to add
    }
  }

  if (e.m_Type == plPhantomRttiManagerEvent::Type::TypeChanged)
  {
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to add or remove
  }
}

void plLongOpsAdapter::CheckAllTypes()
{
  plRTTI::ForEachType(
    [&](const plRTTI* pRtti) {
      if (pRtti->GetAttributeByType<plLongOpAttribute>() != nullptr)
      {
        m_TypesWithLongOps.Insert(pRtti);
      }
    });
}

void plLongOpsAdapter::ObjectAdded(const plDocumentObject* pObject)
{
  const plRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<plComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const plPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = plDynamicCast<const plLongOpAttribute*>(pAttr))
          {
            plLongOpControllerManager::GetSingleton()->RegisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }

    return;
  }

  if (pRtti->IsDerivedFrom<plGameObject>() || pObject->GetParent() == nullptr /*document root object*/)
  {
    for (const plDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectAdded(pChild);
    }
  }
}

void plLongOpsAdapter::ObjectRemoved(const plDocumentObject* pObject)
{
  const plRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<plComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const plPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = plDynamicCast<const plLongOpAttribute*>(pAttr))
          {
            plLongOpControllerManager::GetSingleton()->UnregisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
  }
  else if (pRtti->IsDerivedFrom<plGameObject>())
  {
    for (const plDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectRemoved(pChild);
    }
  }
}
