#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

namespace
{
  plSerializedBlock* FindBlock(plHybridArray<plSerializedBlock, 3>& ref_blocks, plStringView sName)
  {
    for (auto& block : ref_blocks)
    {
      if (block.m_Name == sName)
      {
        return &block;
      }
    }
    return nullptr;
  }

  plSerializedBlock* FindHeaderBlock(plHybridArray<plSerializedBlock, 3>& ref_blocks, plInt32& out_iVersion)
  {
    plStringBuilder sHeaderName = "HeaderV";
    out_iVersion = 0;
    for (auto& block : ref_blocks)
    {
      if (block.m_Name.StartsWith(sHeaderName))
      {
        plResult res = plConversionUtils::StringToInt(block.m_Name.GetData() + sHeaderName.GetElementCount(), out_iVersion);
        if (res.Failed())
        {
          plLog::Error("Failed to parse version from header name '{0}'", block.m_Name);
        }
        return &block;
      }
    }
    return nullptr;
  }

  plSerializedBlock* GetOrCreateBlock(plHybridArray<plSerializedBlock, 3>& ref_blocks, plStringView sName)
  {
    plSerializedBlock* pBlock = FindBlock(ref_blocks, sName);
    if (!pBlock)
    {
      pBlock = &ref_blocks.ExpandAndGetRef();
      pBlock->m_Name = sName;
    }
    if (!pBlock->m_Graph)
    {
      pBlock->m_Graph = PLASMA_DEFAULT_NEW(plAbstractObjectGraph);
    }
    return pBlock;
  }
} // namespace

static void WriteGraph(plOpenDdlWriter& ref_writer, const plAbstractObjectGraph* pGraph, const char* szName)
{
  plMap<plStringView, const plVariant*> SortedProperties;

  ref_writer.BeginObject(szName);

  const auto& Nodes = pGraph->GetAllNodes();
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();

    ref_writer.BeginObject("o");

    {

      plOpenDdlUtils::StoreUuid(ref_writer, node.GetGuid(), "id");
      plOpenDdlUtils::StoreString(ref_writer, node.GetType(), "t");
      plOpenDdlUtils::StoreUInt32(ref_writer, node.GetTypeVersion(), "v");

      if (!node.GetNodeName().IsEmpty())
        plOpenDdlUtils::StoreString(ref_writer, node.GetNodeName(), "n");

      ref_writer.BeginObject("p");
      {
        for (const auto& prop : node.GetProperties())
          SortedProperties[prop.m_sPropertyName] = &prop.m_Value;

        for (auto it = SortedProperties.GetIterator(); it.IsValid(); ++it)
        {
          plOpenDdlUtils::StoreVariant(ref_writer, *it.Value(), it.Key());
        }

        SortedProperties.Clear();
      }
      ref_writer.EndObject();
    }
    ref_writer.EndObject();
  }

  ref_writer.EndObject();
}

void plAbstractGraphDdlSerializer::Write(plStreamWriter& inout_stream, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph,
  bool bCompactMmode, plOpenDdlWriter::TypeStringMode typeMode)
{
  plOpenDdlWriter writer;
  writer.SetOutputStream(&inout_stream);
  writer.SetCompactMode(bCompactMmode);
  writer.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != plOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  Write(writer, pGraph, pTypesGraph);
}


void plAbstractGraphDdlSerializer::Write(
  plOpenDdlWriter& ref_writer, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph /*= nullptr*/)
{
  WriteGraph(ref_writer, pGraph, "Objects");
  if (pTypesGraph)
  {
    WriteGraph(ref_writer, pTypesGraph, "Types");
  }
}

static void ReadGraph(plAbstractObjectGraph* pGraph, const plOpenDdlReaderElement* pRoot)
{
  plStringBuilder tmp, tmp2;
  plVariant varTmp;

  for (const plOpenDdlReaderElement* pObject = pRoot->GetFirstChild(); pObject != nullptr; pObject = pObject->GetSibling())
  {
    const plOpenDdlReaderElement* pGuid = pObject->FindChildOfType(plOpenDdlPrimitiveType::Custom, "id");
    const plOpenDdlReaderElement* pType = pObject->FindChildOfType(plOpenDdlPrimitiveType::String, "t");
    const plOpenDdlReaderElement* pTypeVersion = pObject->FindChildOfType(plOpenDdlPrimitiveType::UInt32, "v");
    const plOpenDdlReaderElement* pName = pObject->FindChildOfType(plOpenDdlPrimitiveType::String, "n");
    const plOpenDdlReaderElement* pProps = pObject->FindChildOfType("p");

    if (pGuid == nullptr || pType == nullptr || pProps == nullptr)
    {
      PLASMA_REPORT_FAILURE("Object contains invalid elements");
      continue;
    }

    plUuid guid;
    if (plOpenDdlUtils::ConvertToUuid(pGuid, guid).Failed())
    {
      PLASMA_REPORT_FAILURE("Object has an invalid guid");
      continue;
    }

    tmp = pType->GetPrimitivesString()[0];

    if (pName)
      tmp2 = pName->GetPrimitivesString()[0];
    else
      tmp2.Clear();

    plUInt32 uiTypeVersion = 0;
    if (pTypeVersion)
    {
      uiTypeVersion = pTypeVersion->GetPrimitivesUInt32()[0];
    }

    auto* pNode = pGraph->AddNode(guid, tmp, uiTypeVersion, tmp2);

    for (const plOpenDdlReaderElement* pProp = pProps->GetFirstChild(); pProp != nullptr; pProp = pProp->GetSibling())
    {
      if (!pProp->HasName())
        continue;

      if (plOpenDdlUtils::ConvertToVariant(pProp, varTmp).Failed())
        continue;

      pNode->AddProperty(pProp->GetName(), varTmp);
    }
  }
}

plResult plAbstractGraphDdlSerializer::Read(
  plStreamReader& inout_stream, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  plOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse DDL graph");
    return PLASMA_FAILURE;
  }

  return Read(reader.GetRootElement(), pGraph, pTypesGraph, bApplyPatches);
}


plResult plAbstractGraphDdlSerializer::Read(const plOpenDdlReaderElement* pRootElement, plAbstractObjectGraph* pGraph,
  plAbstractObjectGraph* pTypesGraph /*= nullptr*/, bool bApplyPatches /*= true*/)
{
  const plOpenDdlReaderElement* pObjects = pRootElement->FindChildOfType("Objects");
  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    plLog::Error("DDL graph does not contain an 'Objects' root object");
    return PLASMA_FAILURE;
  }

  plAbstractObjectGraph* pTempTypesGraph = pTypesGraph;
  if (pTempTypesGraph == nullptr)
  {
    pTempTypesGraph = PLASMA_DEFAULT_NEW(plAbstractObjectGraph);
  }
  const plOpenDdlReaderElement* pTypes = pRootElement->FindChildOfType("Types");
  if (pTypes != nullptr)
  {
    ReadGraph(pTempTypesGraph, pTypes);
  }

  if (bApplyPatches)
  {
    if (pTempTypesGraph)
      plGraphVersioning::GetSingleton()->PatchGraph(pTempTypesGraph);
    plGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTempTypesGraph);
  }

  if (pTypesGraph == nullptr)
    PLASMA_DEFAULT_DELETE(pTempTypesGraph);

  return PLASMA_SUCCESS;
}

plResult plAbstractGraphDdlSerializer::ReadBlocks(plStreamReader& stream, plHybridArray<plSerializedBlock, 3>& blocks)
{
  plOpenDdlReader reader;
  if (reader.ParseDocument(stream, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse DDL graph");
    return PLASMA_FAILURE;
  }

  const plOpenDdlReaderElement* pRoot = reader.GetRootElement();
  for (const plOpenDdlReaderElement* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    plSerializedBlock* pBlock = GetOrCreateBlock(blocks, pChild->GetCustomType());
    ReadGraph(pBlock->m_Graph.Borrow(), pChild);
  }
  return PLASMA_SUCCESS;
}

#define PLASMA_DOCUMENT_VERSION 2

void plAbstractGraphDdlSerializer::WriteDocument(plStreamWriter& inout_stream, const plAbstractObjectGraph* pHeader, const plAbstractObjectGraph* pGraph,
  const plAbstractObjectGraph* pTypes, bool bCompactMode, plOpenDdlWriter::TypeStringMode typeMode)
{
  plOpenDdlWriter writer;
  writer.SetOutputStream(&inout_stream);
  writer.SetCompactMode(bCompactMode);
  writer.SetFloatPrecisionMode(plOpenDdlWriter::FloatPrecisionMode::Exact);
  writer.SetPrimitiveTypeStringMode(typeMode);

  if (typeMode != plOpenDdlWriter::TypeStringMode::Compliant)
    writer.SetIndentation(-1);

  plStringBuilder sHeaderVersion;
  sHeaderVersion.Format("HeaderV{0}", (int)PLASMA_DOCUMENT_VERSION);
  WriteGraph(writer, pHeader, sHeaderVersion);
  WriteGraph(writer, pGraph, "Objects");
  WriteGraph(writer, pTypes, "Types");
}

plResult plAbstractGraphDdlSerializer::ReadDocument(plStreamReader& inout_stream, plUniquePtr<plAbstractObjectGraph>& ref_pHeader,
  plUniquePtr<plAbstractObjectGraph>& ref_pGraph, plUniquePtr<plAbstractObjectGraph>& ref_pTypes, bool bApplyPatches)
{
  plHybridArray<plSerializedBlock, 3> blocks;
  if (ReadBlocks(inout_stream, blocks).Failed())
  {
    return PLASMA_FAILURE;
  }

  plInt32 iVersion = 2;
  plSerializedBlock* pHB = FindHeaderBlock(blocks, iVersion);
  plSerializedBlock* pOB = FindBlock(blocks, "Objects");
  plSerializedBlock* pTB = FindBlock(blocks, "Types");
  if (!pOB)
  {
    plLog::Error("No 'Objects' block in document");
    return PLASMA_FAILURE;
  }
  if (!pTB && !pHB)
  {
    iVersion = 0;
  }
  else if (!pHB)
  {
    iVersion = 1;
  }
  if (iVersion < 2)
  {
    // Move header into its own graph.
    plStringBuilder sHeaderVersion;
    sHeaderVersion.Format("HeaderV{0}", iVersion);
    pHB = GetOrCreateBlock(blocks, sHeaderVersion);
    plAbstractObjectGraph& graph = *pOB->m_Graph.Borrow();
    if (auto* pHeaderNode = graph.GetNodeByName("Header"))
    {
      plAbstractObjectGraph& headerGraph = *pHB->m_Graph.Borrow();
      /*auto* pNewHeaderNode =*/headerGraph.CopyNodeIntoGraph(pHeaderNode);
      // pNewHeaderNode->AddProperty("DocVersion", iVersion);
      graph.RemoveNode(pHeaderNode->GetGuid());
    }
  }

  if (bApplyPatches && pTB)
  {
    plGraphVersioning::GetSingleton()->PatchGraph(pTB->m_Graph.Borrow());
    plGraphVersioning::GetSingleton()->PatchGraph(pHB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
    plGraphVersioning::GetSingleton()->PatchGraph(pOB->m_Graph.Borrow(), pTB->m_Graph.Borrow());
  }

  ref_pHeader = std::move(pHB->m_Graph);
  ref_pGraph = std::move(pOB->m_Graph);
  if (pTB)
  {
    ref_pTypes = std::move(pTB->m_Graph);
  }

  return PLASMA_SUCCESS;
}

// This is a handcrafted DDL reader that ignores everything that is not an 'AssetInfo' object
// The purpose is to speed up reading asset information by skipping everything else
//
// Version 0 and 1:
// The reader 'knows' the file format details and uses them.
// Top-level (ie. depth 0) there is an "Objects" object -> we need to enter that
// Inside that (depth 1) there is the "AssetInfo" object -> need to enter that as well
// All objects inside that must be stored
// Once the AssetInfo object is left everything else can be skipped
//
// Version 2:
// The very first top level object is "Header" and only that is read and parsing is stopped afterwards.
class HeaderReader : public plOpenDdlReader
{
public:
  HeaderReader() = default;

  bool m_bHasHeader = false;
  plInt32 m_iDepth = 0;

  virtual void OnBeginObject(plStringView sType, plStringView sName, bool bGlobalName) override
  {
    //////////////////////////////////////////////////////////////////////////
    // New document format has header block.
    if (m_iDepth == 0 && sType.StartsWith("HeaderV"))
    {
      m_bHasHeader = true;
    }
    if (m_bHasHeader)
    {
      ++m_iDepth;
      plOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Old header is stored in the object block.
    // not yet entered the "Objects" group
    if (m_iDepth == 0 && sType == "Objects")
    {
      ++m_iDepth;

      plOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // not yet entered the "AssetInfo" group, but inside "Objects"
    if (m_iDepth == 1 && sType == "AssetInfo")
    {
      ++m_iDepth;

      plOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // inside "AssetInfo"
    if (m_iDepth > 1)
    {
      ++m_iDepth;
      plOpenDdlReader::OnBeginObject(sType, sName, bGlobalName);
      return;
    }

    // ignore everything else
    SkipRestOfObject();
  }


  virtual void OnEndObject() override
  {
    --m_iDepth;
    if (m_bHasHeader)
    {
      if (m_iDepth == 0)
      {
        m_iDepth = -1;
        StopParsing();
      }
    }
    else
    {
      if (m_iDepth <= 1)
      {
        // we were inside "AssetInfo" or "Objects" and returned from it, so now skip the rest
        m_iDepth = -1;
        StopParsing();
      }
    }
    plOpenDdlReader::OnEndObject();
  }
};

plResult plAbstractGraphDdlSerializer::ReadHeader(plStreamReader& inout_stream, plAbstractObjectGraph* pGraph)
{
  HeaderReader reader;
  if (reader.ParseDocument(inout_stream, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    PLASMA_REPORT_FAILURE("Failed to parse DDL graph");
    return PLASMA_FAILURE;
  }

  const plOpenDdlReaderElement* pObjects = nullptr;
  if (reader.m_bHasHeader)
  {
    pObjects = reader.GetRootElement()->GetFirstChild();
  }
  else
  {
    pObjects = reader.GetRootElement()->FindChildOfType("Objects");
  }

  if (pObjects != nullptr)
  {
    ReadGraph(pGraph, pObjects);
  }
  else
  {
    PLASMA_REPORT_FAILURE("DDL graph does not contain an 'Objects' root object");
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_DdlSerializer);
