#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAddObjectCommand, 1, plRTTIDefaultAllocator<plAddObjectCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Type", GetType, SetType),
    PLASMA_MEMBER_PROPERTY("ParentGuid", m_Parent),
    PLASMA_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
    PLASMA_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPasteObjectsCommand, 1, plRTTIDefaultAllocator<plPasteObjectsCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ParentGuid", m_Parent),
    PLASMA_MEMBER_PROPERTY("TextGraph", m_sGraphTextFormat),
    PLASMA_MEMBER_PROPERTY("Mime", m_sMimeType),
    PLASMA_MEMBER_PROPERTY("AllowPickedPosition", m_bAllowPickedPosition),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plInstantiatePrefabCommand, 1, plRTTIDefaultAllocator<plInstantiatePrefabCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ParentGuid", m_Parent),
    PLASMA_MEMBER_PROPERTY("CreateFromPrefab", m_CreateFromPrefab),
    PLASMA_MEMBER_PROPERTY("BaseGraph", m_sBasePrefabGraph),
    PLASMA_MEMBER_PROPERTY("ObjectGraph", m_sObjectGraph),
    PLASMA_MEMBER_PROPERTY("RemapGuid", m_RemapGuid),
    PLASMA_MEMBER_PROPERTY("CreatedObjects", m_CreatedRootObject),
    PLASMA_MEMBER_PROPERTY("AllowPickedPos", m_bAllowPickedPosition),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plUnlinkPrefabCommand, 1, plRTTIDefaultAllocator<plUnlinkPrefabCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Object", m_Object),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRemoveObjectCommand, 1, plRTTIDefaultAllocator<plRemoveObjectCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMoveObjectCommand, 1, plRTTIDefaultAllocator<plMoveObjectCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("NewParentGuid", m_NewParent),
    PLASMA_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSetObjectPropertyCommand, 1, plRTTIDefaultAllocator<plSetObjectPropertyCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("NewValue", m_NewValue),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
    PLASMA_MEMBER_PROPERTY("Property", m_sProperty),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plResizeAndSetObjectPropertyCommand, 1, plRTTIDefaultAllocator<plResizeAndSetObjectPropertyCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("NewValue", m_NewValue),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
    PLASMA_MEMBER_PROPERTY("Property", m_sProperty),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plInsertObjectPropertyCommand, 1, plRTTIDefaultAllocator<plInsertObjectPropertyCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("NewValue", m_NewValue),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
    PLASMA_MEMBER_PROPERTY("Property", m_sProperty),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRemoveObjectPropertyCommand, 1, plRTTIDefaultAllocator<plRemoveObjectPropertyCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("Index", m_Index),
    PLASMA_MEMBER_PROPERTY("Property", m_sProperty),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMoveObjectPropertyCommand, 1, plRTTIDefaultAllocator<plMoveObjectPropertyCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("OldIndex", m_OldIndex),
    PLASMA_MEMBER_PROPERTY("NewIndex", m_NewIndex),
    PLASMA_MEMBER_PROPERTY("Property", m_sProperty),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plAddObjectCommand
////////////////////////////////////////////////////////////////////////

plAddObjectCommand::plAddObjectCommand()
  : m_pType(nullptr)
  , m_pObject(nullptr)
{
}

plStringView plAddObjectCommand::GetType() const
{
if (m_pType == nullptr)
  return {};

return m_pType->GetTypeName();
}

void plAddObjectCommand::SetType(plStringView sType)
{
  m_pType = plRTTI::FindTypeByName(sType);
}

plStatus plAddObjectCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (!m_NewObjectGuid.IsValid())
      m_NewObjectGuid.CreateNewUuid();
  }

  plDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return plStatus("Add Object: The given parent does not exist!");
  }

  PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index));

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plAddObjectCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  plDocument* pDocument = GetDocument();
  PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return plStatus(PLASMA_SUCCESS);
}

void plAddObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// plPasteObjectsCommand
////////////////////////////////////////////////////////////////////////

plPasteObjectsCommand::plPasteObjectsCommand() {}

plStatus plPasteObjectsCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  plDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return plStatus("Paste Objects: The given parent does not exist!");
  }

  if (!bRedo)
  {
    plAbstractObjectGraph graph;

    {
      // Deserialize
      plRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
      PLASMA_SUCCEED_OR_RETURN(plAbstractGraphDdlSerializer::Read(memoryReader, &graph));
    }

    // Remap
    plUuid seed;
    seed.CreateNewUuid();
    graph.ReMapNodeGuids(seed);

    plDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateOnly);

    plHybridArray<plAbstractObjectNode*, 16> RootNodes;
    auto& nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (pNode->GetNodeName() == "root")
      {
        RootNodes.PushBack(pNode);
      }
    }

    RootNodes.Sort([](const plAbstractObjectNode* a, const plAbstractObjectNode* b) {
      auto* pOrderA = a->FindProperty("__Order");
      auto* pOrderB = b->FindProperty("__Order");
      if (pOrderA && pOrderB && pOrderA->m_Value.CanConvertTo<plUInt32>() && pOrderB->m_Value.CanConvertTo<plUInt32>())
      {
        return pOrderA->m_Value.ConvertTo<plUInt32>() < pOrderB->m_Value.ConvertTo<plUInt32>();
      }
      return a < b;
    });

    plHybridArray<plDocument::PasteInfo, 16> ToBePasted;
    for (plAbstractObjectNode* pNode : RootNodes)
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = pParent;
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, m_sMimeType))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_PastedObjects.IsEmpty())
      return plStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plPasteObjectsCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  plDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return plStatus(PLASMA_SUCCESS);
}

void plPasteObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}

////////////////////////////////////////////////////////////////////////
// plInstantiatePrefabCommand
////////////////////////////////////////////////////////////////////////

plInstantiatePrefabCommand::plInstantiatePrefabCommand()
{
  m_bAllowPickedPosition = true;
}

plStatus plInstantiatePrefabCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  plDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return plStatus("Instantiate Prefab: The given parent does not exist!");
  }

  if (!bRedo)
  {
    // TODO: this is hard-coded, it only works for scene documents !
    const plRTTI* pRootObjectType = plRTTI::FindTypeByName("plGameObject");
    const char* szParentProperty = "Children";

    plDocumentObject* pRootObject = nullptr;
    plHybridArray<plDocument::PasteInfo, 16> ToBePasted;
    plAbstractObjectGraph graph;

    // create root object
    {
      PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(pRootObjectType, pParent, szParentProperty, m_Index));

      // use the same GUID for the root object ID as the remap GUID, this way the object ID is deterministic and reproducible
      m_CreatedRootObject = m_RemapGuid;

      pRootObject = pDocument->GetObjectManager()->CreateObject(pRootObjectType, m_CreatedRootObject);

      auto& ref = ToBePasted.ExpandAndGetRef();
      ref.m_pObject = pRootObject;
      ref.m_pParent = pParent;
      ref.m_Index = m_Index;
    }

    // update meta data
    // this is read when Paste is executed, to determine a good node name
    {
      // if prefabs are not allowed in this document, just create this as a regular object, with no link to the prefab template
      if (pDocument->ArePrefabsAllowed())
      {
        auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_CreatedRootObject);
        pMeta->m_CreateFromPrefab = m_CreateFromPrefab;
        pMeta->m_PrefabSeedGuid = m_RemapGuid;
        pMeta->m_sBasePrefab = m_sBasePrefabGraph;
        pDocument->m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::PrefabFlag);
      }
      else
      {
        pDocument->ShowDocumentStatus("Nested prefabs are not allowed. Instantiated object will not be linked to prefab template.");
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, "application/PlasmaEditor.plAbstractGraph"))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }

      ToBePasted.Clear();
    }

    if (m_PastedObjects.IsEmpty())
      return plStatus("Paste Objects: nothing was pasted!");

    if (!m_sObjectGraph.IsEmpty())
      plPrefabUtils::LoadGraph(graph, m_sObjectGraph);
    else
      plPrefabUtils::LoadGraph(graph, m_sBasePrefabGraph);

    graph.ReMapNodeGuids(m_RemapGuid);

    // a prefab can have multiple top level nodes
    plHybridArray<plAbstractObjectNode*, 4> rootNodes;
    plPrefabUtils::GetRootNodes(graph, rootNodes);

    for (auto* pPrefabRoot : rootNodes)
    {
      plDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateOnly);

      if (auto* pNewObject = reader.CreateObjectFromNode(pPrefabRoot))
      {
        reader.ApplyPropertiesToObject(pPrefabRoot, pNewObject);

        // attach all prefab nodes to the main group node
        pDocument->GetObjectManager()->AddObject(pNewObject, pRootObject, szParentProperty, -1);
      }
    }
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plInstantiatePrefabCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  plDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return plStatus(PLASMA_SUCCESS);
}

void plInstantiatePrefabCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////
// plUnlinkPrefabCommand
//////////////////////////////////////////////////////////////////////////

plStatus plUnlinkPrefabCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  plDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return plStatus("Unlink Prefab: The given object does not exist!");

  // store previous values
  if (!bRedo)
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginReadMetaData(m_Object);
    m_OldCreateFromPrefab = pMeta->m_CreateFromPrefab;
    m_OldRemapGuid = pMeta->m_PrefabSeedGuid;
    m_sOldGraphTextFormat = pMeta->m_sBasePrefab;
    pDocument->m_DocumentObjectMetaData->EndReadMetaData();
  }

  // unlink
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = plUuid();
    pMeta->m_PrefabSeedGuid = plUuid();
    pMeta->m_sBasePrefab.Clear();
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::PrefabFlag);
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plUnlinkPrefabCommand::UndoInternal(bool bFireEvents)
{
  plDocument* pDocument = GetDocument();
  plDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return plStatus("Unlink Prefab: The given object does not exist!");

  // restore link
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = m_OldCreateFromPrefab;
    pMeta->m_PrefabSeedGuid = m_OldRemapGuid;
    pMeta->m_sBasePrefab = m_sOldGraphTextFormat;
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(plDocumentObjectMetaData::PrefabFlag);
  }

  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

plRemoveObjectCommand::plRemoveObjectCommand()
  : m_pParent(nullptr)
  , m_pObject(nullptr)
{
}

plStatus plRemoveObjectCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Remove Object: The given object does not exist!");
    }
    else
      return plStatus("Remove Object: The given object does not exist!");

    PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

    m_pParent = const_cast<plDocumentObject*>(m_pObject->GetParent());
    m_sParentProperty = m_pObject->GetParentProperty();
    const plIReflectedTypeAccessor& accessor = m_pObject->GetParent()->GetTypeAccessor();
    m_Index = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plRemoveObjectCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  plDocument* pDocument = GetDocument();
  PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index));

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index);
  return plStatus(PLASMA_SUCCESS);
}

void plRemoveObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasDone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// plMoveObjectCommand
////////////////////////////////////////////////////////////////////////

plMoveObjectCommand::plMoveObjectCommand()
{
  m_pObject = nullptr;
  m_pOldParent = nullptr;
  m_pNewParent = nullptr;
}

plStatus plMoveObjectCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Move Object: The given object does not exist!");
    }

    if (m_NewParent.IsValid())
    {
      m_pNewParent = pDocument->GetObjectManager()->GetObject(m_NewParent);
      if (m_pNewParent == nullptr)
        return plStatus("Move Object: The new parent does not exist!");
    }

    m_pOldParent = const_cast<plDocumentObject*>(m_pObject->GetParent());
    m_sOldParentProperty = m_pObject->GetParentProperty();
    const plIReflectedTypeAccessor& accessor = m_pOldParent->GetTypeAccessor();
    m_OldIndex = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());

    PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pNewParent, m_sParentProperty, m_Index));
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pNewParent, m_sParentProperty, m_Index);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plMoveObjectCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  plDocument* pDocument = GetDocument();

  plVariant FinalOldPosition = m_OldIndex;

  if (m_Index.CanConvertTo<plInt32>() && m_pOldParent == m_pNewParent)
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    plInt32 iNew = m_Index.ConvertTo<plInt32>();
    plInt32 iOld = m_OldIndex.ConvertTo<plInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }
  }

  PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition));

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition);

  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

plSetObjectPropertyCommand::plSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

plStatus plSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Set Property: The given object does not exist!");
    }
    else
      return plStatus("Set Property: The given object does not exist!");

    plIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    plStatus res;
    m_OldValue = accessor0.GetValue(m_sProperty, m_Index, &res);
    if (res.Failed())
      return res;
    const plAbstractProperty* pProp = accessor0.GetType()->FindPropertyByName(m_sProperty);
    if (pProp == nullptr)
      return plStatus(plFmt("Set Property: The property '{0}' does not exist", m_sProperty));

    if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
    {
      return plStatus(plFmt("Set Property: The property '{0}' is a PointerOwner, use plAddObjectCommand instead", m_sProperty));
    }

    if (pProp->GetAttributeByType<plTemporaryAttribute>())
    {
      // if we modify a 'temporary' property, ie. one that is not serialized,
      // don't mark the document as modified
      m_bModifiedDocument = false;
    }
  }

  return pDocument->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

plStatus plSetObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    plIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.SetValue(m_sProperty, m_OldValue, m_Index))
    {
      return plStatus(plFmt("Set Property: The property '{0}' does not exist", m_sProperty));
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// plSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

plResizeAndSetObjectPropertyCommand::plResizeAndSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

plStatus plResizeAndSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Set Property: The given object does not exist!");
    }
    else
      return plStatus("Set Property: The given object does not exist!");

    const plInt32 uiIndex = m_Index.ConvertTo<plInt32>();

    plIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    const plInt32 iCount = accessor0.GetCount(m_sProperty);

    for (plInt32 i = iCount; i <= uiIndex; ++i)
    {
      plInsertObjectPropertyCommand ins;
      ins.m_Object = m_Object;
      ins.m_sProperty = m_sProperty;
      ins.m_Index = i;
      ins.m_NewValue = plReflectionUtils::GetDefaultVariantFromType(m_NewValue.GetType());

      AddSubCommand(ins).IgnoreResult();
    }

    plSetObjectPropertyCommand set;
    set.m_sProperty = m_sProperty;
    set.m_Index = m_Index;
    set.m_NewValue = m_NewValue;
    set.m_Object = m_Object;

    AddSubCommand(set).IgnoreResult();
  }

  return plStatus(PLASMA_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// plInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

plInsertObjectPropertyCommand::plInsertObjectPropertyCommand()
{
  m_pObject = nullptr;
}

plStatus plInsertObjectPropertyCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Insert Property: The given object does not exist!");
    }
    else
      return plStatus("Insert Property: The given object does not exist!");

    if (m_Index.CanConvertTo<plInt32>() && m_Index.ConvertTo<plInt32>() == -1)
    {
      plIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
      m_Index = accessor.GetCount(m_sProperty.GetData());
    }
  }

  return pDocument->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

plStatus plInsertObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
  }
  else
  {
    plIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.RemoveValue(m_sProperty, m_Index))
    {
      return plStatus(plFmt("Insert Property: The property '{0}' does not exist", m_sProperty));
    }
  }

  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plRemoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

plRemoveObjectPropertyCommand::plRemoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

plStatus plRemoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return plStatus("Remove Property: The given object does not exist!");
      plStatus res;
      m_OldValue = m_pObject->GetTypeAccessor().GetValue(m_sProperty, m_Index, &res);
      if (res.Failed())
        return res;
    }
    else
      return plStatus("Remove Property: The given object does not exist!");
  }

  return pDocument->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
}

plStatus plRemoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    plIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.InsertValue(m_sProperty, m_Index, m_OldValue))
    {
      return plStatus(plFmt("Remove Property: Undo failed! The index '{0}' in property '{1}' does not exist", m_Index.ConvertTo<plString>(), m_sProperty));
    }
  }
  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plMoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

plMoveObjectPropertyCommand::plMoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

plStatus plMoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return plStatus("Move Property: The given object does not exist.");
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, m_OldIndex, m_NewIndex);
}

plStatus plMoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  plVariant FinalOldPosition = m_OldIndex;
  plVariant FinalNewPosition = m_NewIndex;

  if (m_OldIndex.CanConvertTo<plInt32>())
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    plInt32 iNew = m_NewIndex.ConvertTo<plInt32>();
    plInt32 iOld = m_OldIndex.ConvertTo<plInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }

    // The new position is relative to the original array, so we need to substract one to account for
    // the removal of the same element at the lower index.
    if (iNew > iOld)
    {
      FinalNewPosition = iNew - 1;
    }
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, FinalOldPosition, FinalNewPosition);
}
