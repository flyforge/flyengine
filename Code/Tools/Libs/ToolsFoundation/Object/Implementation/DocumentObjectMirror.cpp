#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plObjectChange, plNoBase, 1, plRTTIDefaultAllocator<plObjectChange>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Change", m_Change),
    PLASMA_MEMBER_PROPERTY("Root", m_Root),
    PLASMA_ARRAY_MEMBER_PROPERTY("Steps", m_Steps),
    PLASMA_MEMBER_PROPERTY("Graph", m_GraphData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plObjectChange::plObjectChange(const plObjectChange&)
{
  PLASMA_REPORT_FAILURE("Not supported!");
}

void plObjectChange::GetGraph(plAbstractObjectGraph& ref_graph) const
{
  ref_graph.Clear();

  plRawMemoryStreamReader reader(m_GraphData);
  plAbstractGraphBinarySerializer::Read(reader, &ref_graph);
}

void plObjectChange::SetGraph(plAbstractObjectGraph& ref_graph)
{
  plContiguousMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);
  plAbstractGraphBinarySerializer::Write(writer, &ref_graph);

  m_GraphData = {storage.GetData(), storage.GetStorageSize32()};
}

plObjectChange::plObjectChange(plObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void plObjectChange::operator=(plObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void plObjectChange::operator=(plObjectChange& rhs)
{
  PLASMA_REPORT_FAILURE("Not supported!");
}


plDocumentObjectMirror::plDocumentObjectMirror()
{
  m_pContext = nullptr;
  m_pManager = nullptr;
}

plDocumentObjectMirror::~plDocumentObjectMirror()
{
  PLASMA_ASSERT_DEV(m_pManager == nullptr && m_pContext == nullptr, "Need to call DeInit before d-tor!");
}

void plDocumentObjectMirror::InitSender(const plDocumentObjectManager* pManager)
{
  m_pManager = pManager;
  m_pManager->m_StructureEvents.AddEventHandler(plMakeDelegate(&plDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plDocumentObjectMirror::TreePropertyEventHandler, this));
}

void plDocumentObjectMirror::InitReceiver(plRttiConverterContext* pContext)
{
  m_pContext = pContext;
}

void plDocumentObjectMirror::DeInit()
{
  if (m_pManager)
  {
    m_pManager->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plDocumentObjectMirror::TreeStructureEventHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plDocumentObjectMirror::TreePropertyEventHandler, this));
    m_pManager = nullptr;
  }

  if (m_pContext)
  {
    m_pContext = nullptr;
  }
}

void plDocumentObjectMirror::SetFilterFunction(FilterFunction filter)
{
  m_Filter = filter;
}

void plDocumentObjectMirror::SendDocument()
{
  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    if (IsDiscardedByFilter(pRoot, pChild->GetParentProperty()))
      continue;

    plObjectChange change;
    change.m_Change.m_Operation = plObjectChangeType::NodeAdded;
    change.m_Change.m_Value = pChild->GetGuid();

    plAbstractObjectGraph graph;
    plDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
    objectConverter.AddObjectToGraph(pChild, "Object");
    change.SetGraph(graph);

    ApplyOp(change);
  }
}

void plDocumentObjectMirror::Clear()
{
  if (m_pManager)
  {
    const auto* pRoot = m_pManager->GetRootObject();
    for (auto* pChild : pRoot->GetChildren())
    {
      plObjectChange change;
      change.m_Change.m_Operation = plObjectChangeType::NodeRemoved;
      change.m_Change.m_Value = pChild->GetGuid();

      /*plAbstractObjectGraph graph;
      plDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      plAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
      change.SetGraph(graph);*/

      ApplyOp(change);
    }
  }

  if (m_pContext)
  {
    m_pContext->Clear();
  }
}

void plDocumentObjectMirror::TreeStructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent && IsDiscardedByFilter(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent && IsDiscardedByFilter(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty))
      {
        if (e.m_pNewParent == nullptr || e.m_pNewParent == m_pManager->GetRootObject())
        {
          // Object is now a root object, nothing to do to attach it to its new parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == plPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }
        plObjectChange change;
        CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

        change.m_Change.m_Operation = plObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = e.getInsertIndex();
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
      // case plDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      plObjectChange change;
      CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

      change.m_Change.m_Operation = plObjectChangeType::NodeAdded;
      change.m_Change.m_Index = e.getInsertIndex();
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      plAbstractObjectGraph graph;
      plDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      objectConverter.AddObjectToGraph(e.m_pObject, "Object");
      change.SetGraph(graph);

      ApplyOp(change);
    }
    break;
    case plDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsHeapAllocated(e.m_pPreviousParent, e.m_sParentProperty))
      {
        PLASMA_ASSERT_DEBUG(IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty), "Old and new parent must have the same heap allocation state!");
        if (e.m_pPreviousParent == nullptr || e.m_pPreviousParent == m_pManager->GetRootObject())
        {
          // Object is currently a root object, nothing to do to detach it from its parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == plPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }

        plObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        // Do not delete heap object, just remove it from its owner.
        change.m_Change.m_Operation = plObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      else
      {
        plObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        change.m_Change.m_Operation = plObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
    }
      // case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case plDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      plObjectChange change;
      CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

      change.m_Change.m_Operation = plObjectChangeType::NodeRemoved;
      change.m_Change.m_Index = e.m_OldPropertyIndex;
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ApplyOp(change);
    }
    break;

    default:
      break;
  }
}

void plDocumentObjectMirror::TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (IsDiscardedByFilter(e.m_pObject, e.m_sProperty))
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectPropertyEvent::Type::PropertySet:
    {
      plObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = plObjectChangeType::PropertySet;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case plDocumentObjectPropertyEvent::Type::PropertyInserted:
    {
      plObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = plObjectChangeType::PropertyInserted;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case plDocumentObjectPropertyEvent::Type::PropertyRemoved:
    {
      plObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = plObjectChangeType::PropertyRemoved;
      change.m_Change.m_Index = e.m_OldIndex;
      change.m_Change.m_Value = e.m_OldValue;
      ApplyOp(change);
    }
    break;
    case plDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      plUInt32 uiOldIndex = e.m_OldIndex.ConvertTo<plUInt32>();
      plUInt32 uiNewIndex = e.m_NewIndex.ConvertTo<plUInt32>();
      // NewValue can be invalid if an invalid variant in a variant array is moved
      // PLASMA_ASSERT_DEBUG(e.m_NewValue.IsValid(), "Value must be valid");

      {
        plObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = plObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = uiOldIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }

      {
        plObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = plObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = uiNewIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      return;
    }
    break;
  }
}

void* plDocumentObjectMirror::GetNativeObjectPointer(const plDocumentObject* pObject)
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

const void* plDocumentObjectMirror::GetNativeObjectPointer(const plDocumentObject* pObject) const
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

bool plDocumentObjectMirror::IsRootObject(const plDocumentObject* pParent)
{
  return (pParent == nullptr || pParent == m_pManager->GetRootObject());
}

bool plDocumentObjectMirror::IsHeapAllocated(const plDocumentObject* pParent, const char* szParentProperty)
{
  if (pParent == nullptr || pParent == m_pManager->GetRootObject())
    return true;

  const plRTTI* pRtti = pParent->GetTypeAccessor().GetType();

  auto* pProp = pRtti->FindPropertyByName(szParentProperty);
  return pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner);
}


bool plDocumentObjectMirror::IsDiscardedByFilter(const plDocumentObject* pObject, const char* szProperty) const
{
  if (m_Filter.IsValid())
  {
    return !m_Filter(pObject, szProperty);
  }
  return false;
}

void plDocumentObjectMirror::CreatePath(plObjectChange& out_change, const plDocumentObject* pRoot, const char* szProperty)
{
  if (pRoot && pRoot->GetDocumentObjectManager()->GetRootObject() != pRoot)
  {
    plHybridArray<const plDocumentObject*, 8> path;
    out_change.m_Root = FindRootOpObject(pRoot, path);
    FlattenSteps(path, out_change.m_Steps);
  }

  out_change.m_Change.m_sProperty = szProperty;
}

plUuid plDocumentObjectMirror::FindRootOpObject(const plDocumentObject* pParent, plHybridArray<const plDocumentObject*, 8>& path)
{
  path.PushBack(pParent);

  if (!pParent->IsOnHeap())
  {
    return FindRootOpObject(pParent->GetParent(), path);
  }
  else
  {
    return pParent->GetGuid();
  }
}

void plDocumentObjectMirror::FlattenSteps(const plArrayPtr<const plDocumentObject* const> path, plHybridArray<plPropertyPathStep, 2>& out_steps)
{
  plUInt32 uiCount = path.GetCount();
  PLASMA_ASSERT_DEV(uiCount > 0, "Path must not be empty!");
  PLASMA_ASSERT_DEV(path[uiCount - 1]->IsOnHeap(), "Root of steps must be on heap!");

  // Only root object? Then there is no path from it.
  if (uiCount == 1)
    return;

  for (plInt32 i = (plInt32)uiCount - 2; i >= 0; --i)
  {
    const plDocumentObject* pObject = path[i];
    out_steps.PushBack(plPropertyPathStep({pObject->GetParentProperty(), pObject->GetPropertyIndex()}));
  }
}

void plDocumentObjectMirror::ApplyOp(plObjectChange& change)
{
  plRttiConverterObject object;
  if (change.m_Root.IsValid())
  {
    object = m_pContext->GetObjectByGUID(change.m_Root);
    if (!object.m_pObject)
      return;
    // PLASMA_ASSERT_DEV(object.m_pObject != nullptr, "Root object does not exist in mirrored native object!");
  }

  plPropertyPath propPath;
  if (propPath.InitializeFromPath(object.m_pType, change.m_Steps).Failed())
  {
    plLog::Error("Failed to init property path on object of type '{0}'.", object.m_pType->GetTypeName());
    return;
  }
  propPath.WriteToLeafObject(
            object.m_pObject, *object.m_pType, [this, &change](void* pLeaf, const plRTTI& type) { ApplyOp(plRttiConverterObject(&type, pLeaf), change); })
    .IgnoreResult();
}

void plDocumentObjectMirror::ApplyOp(plRttiConverterObject object, const plObjectChange& change)
{
  const plAbstractProperty* pProp = nullptr;

  if (object.m_pType != nullptr)
  {
    pProp = object.m_pType->FindPropertyByName(change.m_Change.m_sProperty);
    if (pProp == nullptr)
    {
      plLog::Error("Property '{0}' not found, can't apply mirror op!", change.m_Change.m_sProperty);
      return;
    }
  }

  switch (change.m_Change.m_Operation)
  {
    case plObjectChangeType::NodeAdded:
    {
      plAbstractObjectGraph graph;
      change.GetGraph(graph);
      plRttiConverterReader reader(&graph, m_pContext);
      const plAbstractObjectNode* pNode = graph.GetNodeByName("Object");
      const plRTTI* pType = m_pContext->FindTypeByName(pNode->GetType());
      void* pValue = reader.CreateObjectFromNode(pNode);
      if (!pValue)
      {
        // Can't create object, exiting.
        return;
      }

      if (!change.m_Root.IsValid())
      {
        // Create without parent (root element)
        return;
      }

      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const plAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
        }
        else
        {
          pSpecificProp->SetValuePtr(object.m_pObject, pValue);
        }
      }
      else if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const plAbstractArrayProperty*>(pProp);
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<plUInt32>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<plUInt32>(), pValue);
        }
      }
      else if (pProp->GetCategory() == plPropertyCategory::Set)
      {
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<const plAbstractSetProperty*>(pProp);
        plReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, plVariant(pValue, pType));
      }
      else if (pProp->GetCategory() == plPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const plAbstractMapProperty*>(pProp);
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<plString>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<plString>(), pValue);
        }
      }

      if (!pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(pNode->GetGuid());
      }
    }
    break;
    case plObjectChangeType::NodeRemoved:
    {
      if (!change.m_Root.IsValid())
      {
        // Delete root object
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<plUuid>());
        return;
      }

      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const plAbstractMemberProperty*>(pProp);
        if (!pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
        {
          plLog::Error("Property '{0}' not a pointer, can't remove object!", change.m_Change.m_sProperty);
          return;
        }

        void* pValue = nullptr;
        pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const plAbstractArrayProperty*>(pProp);
        plReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<plUInt32>());
      }
      else if (pProp->GetCategory() == plPropertyCategory::Set)
      {
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<const plAbstractSetProperty*>(pProp);
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<plUuid>());
        plReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, plVariant(valueObject.m_pObject, valueObject.m_pType));
      }
      else if (pProp->GetCategory() == plPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const plAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<plString>());
      }

      if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<plUuid>());
      }
    }
    break;
    case plObjectChangeType::PropertySet:
    {
      if (pProp->GetCategory() == plPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const plAbstractMemberProperty*>(pProp);
        plReflectionUtils::SetMemberPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const plAbstractArrayProperty*>(pProp);
        plReflectionUtils::SetArrayPropertyValue(
          pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<plUInt32>(), change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<const plAbstractSetProperty*>(pProp);
        plReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const plAbstractMapProperty*>(pProp);
        plReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<plString>(), change.m_Change.m_Value);
      }
    }
    break;
    case plObjectChangeType::PropertyInserted:
    {
      plVariant value = change.m_Change.m_Value;
      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<plUuid>());
        value = plTypedPointer(valueObject.m_pObject, valueObject.m_pType);
      }

      if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const plAbstractArrayProperty*>(pProp);
        plReflectionUtils::InsertArrayPropertyValue(pSpecificProp, object.m_pObject, value, change.m_Change.m_Index.ConvertTo<plUInt32>());
      }
      else if (pProp->GetCategory() == plPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<const plAbstractSetProperty*>(pProp);
        plReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const plAbstractMapProperty*>(pProp);
        plReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<plString>(), value);
      }
    }
    break;
    case plObjectChangeType::PropertyRemoved:
    {
      if (pProp->GetCategory() == plPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const plAbstractArrayProperty*>(pProp);
        plReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<plUInt32>());
      }
      else if (pProp->GetCategory() == plPropertyCategory::Set)
      {
        plVariant value = change.m_Change.m_Value;
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<plUuid>());
          value = plTypedPointer(valueObject.m_pObject, valueObject.m_pType);
        }

        auto pSpecificProp = static_cast<const plAbstractSetProperty*>(pProp);
        plReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == plPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const plAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<plString>());
      }
    }
    break;
  }
}