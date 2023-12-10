#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

#define PREFAB_DEBUG false

plString ToBinary(const plUuid& guid)
{
  plStringBuilder s, sResult;

  plUInt8* pBytes = (plUInt8*)&guid;

  for (plUInt32 i = 0; i < sizeof(plUuid); ++i)
  {
    s.Format("{0}", plArgU((plUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    sResult.Append(s.GetData());
  }

  return sResult;
}

void plPrefabUtils::LoadGraph(plAbstractObjectGraph& out_graph, plStringView sGraph)
{
  plPrefabCache::GetSingleton()->LoadGraph(out_graph, plStringView(sGraph));
}


plAbstractObjectNode* plPrefabUtils::GetFirstRootNode(plAbstractObjectGraph& ref_graph)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<plVariantArray>())
        {
          const plVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<plVariantArray>();

          for (const plVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<plUuid>())
              continue;

            const plUuid& rootObjectGuid = childGuid.Get<plUuid>();

            return ref_graph.GetNode(rootObjectGuid);
          }
        }
      }
    }
  }
  return nullptr;
}

void plPrefabUtils::GetRootNodes(plAbstractObjectGraph& ref_graph, plHybridArray<plAbstractObjectNode*, 4>& out_nodes)
{
  auto& nodes = ref_graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "ObjectTree")
    {
      for (const auto& ObjectTreeProp : pNode->GetProperties())
      {
        if (ObjectTreeProp.m_sPropertyName == "Children" && ObjectTreeProp.m_Value.IsA<plVariantArray>())
        {
          const plVariantArray& RootChildren = ObjectTreeProp.m_Value.Get<plVariantArray>();

          for (const plVariant& childGuid : RootChildren)
          {
            if (!childGuid.IsA<plUuid>())
              continue;

            const plUuid& rootObjectGuid = childGuid.Get<plUuid>();

            out_nodes.PushBack(ref_graph.GetNode(rootObjectGuid));
          }

          return;
        }
      }

      return;
    }
  }
}

plUuid plPrefabUtils::GetPrefabRoot(const plDocumentObject* pObject, const plObjectMetaData<plUuid, plDocumentObjectMetaData>& documentObjectMetaData, plInt32* pDepth)
{
  auto pMeta = documentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
  plUuid source = pMeta->m_CreateFromPrefab;
  documentObjectMetaData.EndReadMetaData();

  if (source.IsValid())
  {
    return pObject->GetGuid();
  }

  if (pObject->GetParent() != nullptr)
  {
    if (pDepth)
      *pDepth += 1;
    return GetPrefabRoot(pObject->GetParent(), documentObjectMetaData);
  }
  return plUuid();
}


plVariant plPrefabUtils::GetDefaultValue(const plAbstractObjectGraph& graph, const plUuid& objectGuid, plStringView sProperty, plVariant index, bool* pValueFound)
{
  if (pValueFound)
    *pValueFound = false;

  const plAbstractObjectNode* pNode = graph.GetNode(objectGuid);
  if (!pNode)
    return plVariant();

  const plAbstractObjectNode::Property* pProp = pNode->FindProperty(sProperty);
  if (pProp)
  {
    const plVariant& value = pProp->m_Value;

    if (value.IsA<plVariantArray>() && index.CanConvertTo<plUInt32>())
    {
      plUInt32 uiIndex = index.ConvertTo<plUInt32>();
      const plVariantArray& valueArray = value.Get<plVariantArray>();
      if (uiIndex < valueArray.GetCount())
      {
        if (pValueFound)
          *pValueFound = true;
        return valueArray[uiIndex];
      }
      return plVariant();
    }
    else if (value.IsA<plVariantDictionary>() && index.CanConvertTo<plString>())
    {
      plString sKey = index.ConvertTo<plString>();
      const plVariantDictionary& valueDict = value.Get<plVariantDictionary>();
      auto it = valueDict.Find(sKey);
      if (it.IsValid())
      {
        if (pValueFound)
          *pValueFound = true;
        return it.Value();
      }
      return plVariant();
    }
    if (pValueFound)
      *pValueFound = true;
    return value;
  }

  return plVariant();
}

void plPrefabUtils::WriteDiff(const plDeque<plAbstractGraphDiffOperation>& mergedDiff, plStringBuilder& out_sText)
{
  for (const auto& diff : mergedDiff)
  {
    plStringBuilder Data = ToBinary(diff.m_Node);

    switch (diff.m_Operation)
    {
      case plAbstractGraphDiffOperation::Op::NodeAdded:
      {
        out_sText.AppendFormat("<add> - {{0}} ({1})\n", Data, diff.m_sProperty);
      }
      break;

      case plAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        out_sText.AppendFormat("<del> - {{0}}\n", Data);
      }
      break;

      case plAbstractGraphDiffOperation::Op::PropertyChanged:
        if (diff.m_Value.CanConvertTo<plString>())
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = {2}\n", Data, diff.m_sProperty, diff.m_Value.ConvertTo<plString>());
        else
          out_sText.AppendFormat("<set> - {{0}} - \"{1}\" = xxx\n", Data, diff.m_sProperty);
        break;
    }
  }
}

void plPrefabUtils::Merge(const plAbstractObjectGraph& baseGraph, const plAbstractObjectGraph& leftGraph, const plAbstractObjectGraph& rightGraph, plDeque<plAbstractGraphDiffOperation>& out_mergedDiff)
{
  // debug output
  if (PREFAB_DEBUG)
  {
    {
      plFileWriter file;
      file.Open("C:\\temp\\Prefab - base.txt").IgnoreResult();
      plAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, plOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      plFileWriter file;
      file.Open("C:\\temp\\Prefab - template.txt").IgnoreResult();
      plAbstractGraphDdlSerializer::Write(file, &leftGraph, nullptr, false, plOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }

    {
      plFileWriter file;
      file.Open("C:\\temp\\Prefab - instance.txt").IgnoreResult();
      plAbstractGraphDdlSerializer::Write(file, &rightGraph, nullptr, false, plOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }

  plDeque<plAbstractGraphDiffOperation> LeftToBase;
  leftGraph.CreateDiffWithBaseGraph(baseGraph, LeftToBase);
  plDeque<plAbstractGraphDiffOperation> RightToBase;
  rightGraph.CreateDiffWithBaseGraph(baseGraph, RightToBase);

  baseGraph.MergeDiffs(LeftToBase, RightToBase, out_mergedDiff);

  // debug output
  if (PREFAB_DEBUG)
  {
    plFileWriter file;
    file.Open("C:\\temp\\Prefab - diff.txt").IgnoreResult();

    plStringBuilder sDiff;
    sDiff.Append("######## Template To Base #######\n");
    plPrefabUtils::WriteDiff(LeftToBase, sDiff);
    sDiff.Append("\n\n######## Instance To Base #######\n");
    plPrefabUtils::WriteDiff(RightToBase, sDiff);
    sDiff.Append("\n\n######## Merged Diff #######\n");
    plPrefabUtils::WriteDiff(out_mergedDiff, sDiff);


    file.WriteBytes(sDiff.GetData(), sDiff.GetElementCount()).IgnoreResult();
  }
}

void plPrefabUtils::Merge(plStringView sBase, plStringView sLeft, plDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const plUuid& prefabSeed, plStringBuilder& out_sNewGraph)
{
  // prepare the original prefab as a graph
  plAbstractObjectGraph baseGraph;
  plPrefabUtils::LoadGraph(baseGraph, sBase);
  if (auto pHeader = baseGraph.GetNodeByName("Header"))
  {
    baseGraph.RemoveNode(pHeader->GetGuid());
  }

  {
    // read the new template as a graph
    plAbstractObjectGraph leftGraph;
    plPrefabUtils::LoadGraph(leftGraph, sLeft);
    if (auto pHeader = leftGraph.GetNodeByName("Header"))
    {
      leftGraph.RemoveNode(pHeader->GetGuid());
    }

    // prepare the current state as a graph
    plAbstractObjectGraph rightGraph;
    {
      plDocumentObjectConverterWriter writer(&rightGraph, pRight->GetDocumentObjectManager());

      plVariantArray children;
      if (bRightIsNotPartOfPrefab)
      {
        for (plDocumentObject* pChild : pRight->GetChildren())
        {
          writer.AddObjectToGraph(pChild);
          children.PushBack(pChild->GetGuid());
        }
      }
      else
      {
        writer.AddObjectToGraph(pRight);
        children.PushBack(pRight->GetGuid());
      }

      rightGraph.ReMapNodeGuids(prefabSeed, true);
      // just take the entire ObjectTree node as is TODO: this may cause a crash if the root object is replaced
      plAbstractObjectNode* pRightObjectTree = rightGraph.CopyNodeIntoGraph(leftGraph.GetNodeByName("ObjectTree"));
      // The root node should always have a property 'children' where all the root objects are attached to. We need to replace that property's value as the prefab instance graph can have less or more objects than the template.
      plAbstractObjectNode::Property* pChildrenProp = pRightObjectTree->FindProperty("Children");
      pChildrenProp->m_Value = children;
    }

    // Merge diffs relative to base
    plDeque<plAbstractGraphDiffOperation> mergedDiff;
    plPrefabUtils::Merge(baseGraph, leftGraph, rightGraph, mergedDiff);


    {
      // Apply merged diff to base.
      baseGraph.ApplyDiff(mergedDiff);

      plContiguousMemoryStreamStorage stor;
      plMemoryStreamWriter sw(&stor);

      plAbstractGraphDdlSerializer::Write(sw, &baseGraph, nullptr, true, plOpenDdlWriter::TypeStringMode::Shortest);

      out_sNewGraph.SetSubString_ElementCount((const char*)stor.GetData(), stor.GetStorageSize32());
    }

    // debug output
    if (PREFAB_DEBUG)
    {
      plFileWriter file;
      file.Open("C:\\temp\\Prefab - result.txt").IgnoreResult();
      plAbstractGraphDdlSerializer::Write(file, &baseGraph, nullptr, false, plOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);
    }
  }
}

plString plPrefabUtils::ReadDocumentAsString(plStringView sFile)
{
  plFileReader file;
  if (file.Open(sFile) == PLASMA_FAILURE)
  {
    plLog::Error("Failed to open document file '{0}'", sFile);
    return plString();
  }

  plStringBuilder sGraph;
  sGraph.ReadAll(file);

  return sGraph;
}
