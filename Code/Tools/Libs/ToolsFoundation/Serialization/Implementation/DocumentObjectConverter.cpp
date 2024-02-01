#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

plAbstractObjectNode* plDocumentObjectConverterWriter::AddObjectToGraph(const plDocumentObject* pObject, plStringView sNodeName)
{
  plAbstractObjectNode* pNode = AddSubObjectToGraph(pObject, sNodeName);

  while (!m_QueuedObjects.IsEmpty())
  {
    auto itCur = m_QueuedObjects.GetIterator();

    AddSubObjectToGraph(itCur.Key(), nullptr);

    m_QueuedObjects.Remove(itCur);
  }

  return pNode;
}

void plDocumentObjectConverterWriter::AddProperty(plAbstractObjectNode* pNode, const plAbstractProperty* pProp, const plDocumentObject* pObject)
{
  if (m_Filter.IsValid() && !m_Filter(pObject, pProp))
    return;

  const plRTTI* pPropType = pProp->GetSpecificType();
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
        {
          const plUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<plUuid>();

          pNode->AddProperty(pProp->GetPropertyName(), guid);
          if (guid.IsValid())
            m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
        else
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        {
          plStringBuilder sTemp;
          plReflectionUtils::EnumerationToString(
            pPropType, pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).ConvertTo<plInt64>(), sTemp);
          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
        {
          const plUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<plUuid>();
          PL_ASSERT_DEV(guid.IsValid(), "Embedded class cannot be null.");
          pNode->AddProperty(pProp->GetPropertyName(), guid);
          m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
      }
    }

    break;

    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      const plInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      PL_ASSERT_DEV(iCount >= 0, "Invalid array property size {0}", iCount);

      plVariantArray values;
      values.SetCount(iCount);

      for (plInt32 i = 0; i < iCount; ++i)
      {
        values[i] = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(values[i].Get<plUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case plPropertyCategory::Map:
    {
      const plInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      PL_ASSERT_DEV(iCount >= 0, "Invalid map property size {0}", iCount);

      plVariantDictionary values;
      values.Reserve(iCount);
      plHybridArray<plVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      for (const plVariant& key : keys)
      {
        plVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), key);
        values.Insert(key.Get<plString>(), value);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(value.Get<plUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case plPropertyCategory::Constant:
      // Nothing to do here.
      break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED
  }
}

void plDocumentObjectConverterWriter::AddProperties(plAbstractObjectNode* pNode, const plDocumentObject* pObject)
{
  plHybridArray<const plAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (const auto* pProp : properties)
  {
    AddProperty(pNode, pProp, pObject);
  }
}

plAbstractObjectNode* plDocumentObjectConverterWriter::AddSubObjectToGraph(const plDocumentObject* pObject, plStringView sNodeName)
{
  plAbstractObjectNode* pNode = m_pGraph->AddNode(pObject->GetGuid(), pObject->GetType()->GetTypeName(), pObject->GetType()->GetTypeVersion(), sNodeName);
  AddProperties(pNode, pObject);
  return pNode;
}

plDocumentObjectConverterReader::plDocumentObjectConverterReader(const plAbstractObjectGraph* pGraph, plDocumentObjectManager* pManager, Mode mode)
{
  m_pManager = pManager;
  m_pGraph = pGraph;
  m_Mode = mode;
  m_uiUnknownTypeInstances = 0;
}

plDocumentObject* plDocumentObjectConverterReader::CreateObjectFromNode(const plAbstractObjectNode* pNode)
{
  plDocumentObject* pObject = nullptr;
  const plRTTI* pType = plRTTI::FindTypeByName(pNode->GetType());
  if (pType)
  {
    pObject = m_pManager->CreateObject(pType, pNode->GetGuid());
  }
  else
  {
    if (!m_UnknownTypes.Contains(pNode->GetType()))
    {
      plLog::Error("Cannot create node of unknown type '{0}'.", pNode->GetType());
      m_UnknownTypes.Insert(pNode->GetType());
    }
    m_uiUnknownTypeInstances++;
  }
  return pObject;
}

void plDocumentObjectConverterReader::AddObject(plDocumentObject* pObject, plDocumentObject* pParent, plStringView sParentProperty, plVariant index)
{
  PL_ASSERT_DEV(pObject && pParent, "Need to have valid objects to add them to the document");
  if (m_Mode == plDocumentObjectConverterReader::Mode::CreateAndAddToDocument && pParent->GetDocumentObjectManager()->GetObject(pParent->GetGuid()))
  {
    m_pManager->AddObject(pObject, pParent, sParentProperty, index);
  }
  else
  {
    pParent->InsertSubObject(pObject, sParentProperty, index);
  }
}

void plDocumentObjectConverterReader::ApplyPropertiesToObject(const plAbstractObjectNode* pNode, plDocumentObject* pObject)
{
  // PL_ASSERT_DEV(pObject->GetChildren().GetCount() == 0, "Can only apply properties to empty objects!");
  plHybridArray<const plAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (auto* pProp : properties)
  {
    auto* pOtherProp = pNode->FindProperty(pProp->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, pProp, pOtherProp);
  }
}

void plDocumentObjectConverterReader::ApplyDiffToObject(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pObject, plDeque<plAbstractGraphDiffOperation>& ref_diff)
{
  plHybridArray<plAbstractGraphDiffOperation*, 4> change;

  for (auto& op : ref_diff)
  {
    if (op.m_Operation == plAbstractGraphDiffOperation::Op::PropertyChanged && pObject->GetGuid() == op.m_Node)
      change.PushBack(&op);
  }

  for (auto* op : change)
  {
    const plAbstractProperty* pProp = pObject->GetTypeAccessor().GetType()->FindPropertyByName(op->m_sProperty);
    if (!pProp)
      continue;

    ApplyDiff(pObjectAccessor, pObject, pProp, *op, ref_diff);
  }

  // Recurse into owned sub objects (old or new)
  for (const plDocumentObject* pSubObject : pObject->GetChildren())
  {
    ApplyDiffToObject(pObjectAccessor, pSubObject, ref_diff);
  }
}

void plDocumentObjectConverterReader::ApplyDiff(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plAbstractGraphDiffOperation& op, plDeque<plAbstractGraphDiffOperation>& diff)
{
  plStringBuilder sTemp;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  auto NeedsToBeDeleted = [&diff](const plUuid& guid) -> bool
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeRemoved && guid == op.m_Node)
        return true;
    }
    return false;
  };
  auto NeedsToBeCreated = [&diff](const plUuid& guid) -> plAbstractGraphDiffOperation*
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeAdded && guid == op.m_Node)
        return &op;
    }
    return nullptr;
  };

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags) || bIsValueType)
      {
        pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
      }
      else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
      {
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            const plUuid oldGuid = pObjectAccessor->Get<plUuid>(pObject, pProp);
            const plUuid newGuid = op.m_Value.Get<plUuid>();
            if (oldGuid.IsValid())
            {
              if (NeedsToBeDeleted(oldGuid))
              {
                pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).IgnoreResult();
              }
            }

            if (newGuid.IsValid())
            {
              if (plAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(newGuid))
              {
                pObjectAccessor->AddObject(pObject, pProp, plVariant(), plRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
              }

              const plDocumentObject* pChild = pObject->GetChild(newGuid);
              PL_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
            }
          }
          else
          {
            pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
          }
        }
        else
        {
          // Noting to do here, value cannot change
        }
      }
      break;
    }
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      const plVariantArray& values = op.m_Value.Get<plVariantArray>();
      plInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      if (bIsValueType || (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        for (plUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (i < (plUInt32)iCurrentCount)
            pObjectAccessor->SetValue(pObject, pProp, values[i], i).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, values[i], i).IgnoreResult();
        }
        for (plInt32 i = iCurrentCount - 1; i >= (plInt32)values.GetCount(); --i)
        {
          pObjectAccessor->RemoveValue(pObject, pProp, i).IgnoreResult();
        }
      }
      else // Class
      {
        const plInt32 iCurrentCount2 = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

        plHybridArray<plVariant, 16> currentValues;
        pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
        for (plInt32 i = iCurrentCount2 - 1; i >= 0; --i)
        {
          if (NeedsToBeDeleted(currentValues[i].Get<plUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<plUuid>())).IgnoreResult();
          }
        }

        for (plUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (plAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(values[i].Get<plUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, i, plRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(values[i].Get<plUuid>()), pObject, pProp, i).IgnoreResult();
          }
        }
      }
      break;
    }
    case plPropertyCategory::Map:
    {
      const plVariantDictionary& values = op.m_Value.Get<plVariantDictionary>();
      plHybridArray<plVariant, 16> keys;
      PL_VERIFY(pObjectAccessor->GetKeys(pObject, pProp, keys).Succeeded(), "Property is not a map, getting keys failed.");

      if (bIsValueType || (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        for (const plVariant& key : keys)
        {
          const plString& sKey = key.Get<plString>();
          if (!values.Contains(sKey))
          {
            PL_VERIFY(pObjectAccessor->RemoveValue(pObject, pProp, key).Succeeded(), "RemoveValue failed.");
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          plVariant variantKey(it.Key());
          if (keys.Contains(variantKey))
            pObjectAccessor->SetValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
        }
      }
      else // Class
      {
        for (const plVariant& key : keys)
        {
          plVariant value;
          PL_VERIFY(pObjectAccessor->GetValue(pObject, pProp, value, key).Succeeded(), "");
          if (NeedsToBeDeleted(value.Get<plUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(value.Get<plUuid>())).IgnoreResult();
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const plVariant& value = it.Value();
          plVariant variantKey(it.Key());
          if (plAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(value.Get<plUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, variantKey, plRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(value.Get<plUuid>()), pObject, pProp, variantKey).IgnoreResult();
          }
        }
      }
      break;
    }

    case plPropertyCategory::Function:
    case plPropertyCategory::Constant:
      break; // nothing to do
  }
}

void plDocumentObjectConverterReader::ApplyProperty(plDocumentObject* pObject, const plAbstractProperty* pProp, const plAbstractObjectNode::Property* pSource)
{
  plStringBuilder sTemp;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
        {
          const plUuid guid = pSource->m_Value.Get<plUuid>();
          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            PL_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            if (auto* pSubObject = CreateObjectFromNode(pSubNode))
            {
              ApplyPropertiesToObject(pSubNode, pSubObject);
              AddObject(pSubObject, pObject, pProp->GetPropertyName(), plVariant());
            }
          }
        }
        else
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags) || bIsValueType)
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
        else // plPropertyFlags::Class
        {
          const plUuid& nodeGuid = pSource->m_Value.Get<plUuid>();

          const plUuid subObjectGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<plUuid>();
          plDocumentObject* pEmbeddedClassObject = pObject->GetChild(subObjectGuid);
          PL_ASSERT_DEV(pEmbeddedClassObject != nullptr, "CreateObject should have created all embedded classes!");
          auto* pSubNode = m_pGraph->GetNode(nodeGuid);
          PL_ASSERT_DEV(pSubNode != nullptr, "invalid document");

          ApplyPropertiesToObject(pSubNode, pEmbeddedClassObject);
        }
      }
      break;
    }
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      const plVariantArray& array = pSource->m_Value.Get<plVariantArray>();
      const plInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      if (bIsValueType || (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        for (plUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (i < (plUInt32)iCurrentCount)
          {
            pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), array[i], i);
          }
          else
          {
            pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), i, array[i]);
          }
        }
        for (plInt32 i = iCurrentCount - 1; i >= (plInt32)array.GetCount(); i--)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), i);
        }
      }
      else
      {
        for (plUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const plUuid guid = array[i].Get<plUuid>();
          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            PL_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            if (i < (plUInt32)iCurrentCount)
            {
              // Overwrite existing object
              plUuid childGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i).ConvertTo<plUuid>();
              if (plDocumentObject* pSubObject = m_pManager->GetObject(childGuid))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
              }
            }
            else
            {
              if (plDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
                AddObject(pSubObject, pObject, pProp->GetPropertyName(), -1);
              }
            }
          }
        }
        for (plInt32 i = iCurrentCount - 1; i >= (plInt32)array.GetCount(); i--)
        {
          PL_REPORT_FAILURE("Not implemented");
        }
      }
      break;
    }
    case plPropertyCategory::Map:
    {
      const plVariantDictionary& values = pSource->m_Value.Get<plVariantDictionary>();
      plHybridArray<plVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      if (bIsValueType || (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        for (const plVariant& key : keys)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), key);
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), plVariant(it.Key()), it.Value());
        }
      }
      else
      {
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const plVariant& value = it.Value();
          const plUuid guid = value.Get<plUuid>();

          const plVariant variantKey(it.Key());

          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            PL_ASSERT_DEV(pSubNode != nullptr, "invalid document");
            if (plDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
            {
              ApplyPropertiesToObject(pSubNode, pSubObject);
              AddObject(pSubObject, pObject, pProp->GetPropertyName(), variantKey);
            }
          }
        }
      }
      break;
    }

    case plPropertyCategory::Function:
    case plPropertyCategory::Constant:
      break; // nothing to do
  }
}
