#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  void DumpAST(const plExpressionAST& ast, plStringView sAssetName, plStringView sOutputName)
  {
    plDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    plStringBuilder sFileName;
    sFileName.SetFormat(":appdata/{0}_{1}_AST.dgml", sAssetName, sOutputName);

    plDGMLGraphWriter dgmlGraphWriter;
    PL_IGNORE_UNUSED(dgmlGraphWriter);
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      plLog::Info("AST was dumped to: {0}", sFileName);
    }
    else
    {
      plLog::Error("Failed to dump AST to: {0}", sFileName);
    }
  }

  static const char* s_szSphereAssetId = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";     // Base/Prefabs/Sphere.plPrefab
  static const char* s_szBWGradientAssetId = "{ 3834b7d0-5a3f-140d-31d8-3a2bf48b09bd }"; // Base/Textures/BlackWhiteGradient.plColorGradientAsset

} // namespace

////////////////////////////////////////////////////////////////

struct DocObjAndOutput
{
  PL_DECLARE_POD_TYPE();

  const plDocumentObject* m_pObject;
  const char* m_szOutputName;
};

template <>
struct plHashHelper<DocObjAndOutput>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const DocObjAndOutput& value)
  {
    const plUInt32 hashA = plHashHelper<const void*>::Hash(value.m_pObject);
    const plUInt32 hashB = plHashHelper<const void*>::Hash(value.m_szOutputName);
    return plHashingUtils::CombineHashValues32(hashA, hashB);
  }

  PL_ALWAYS_INLINE static bool Equal(const DocObjAndOutput& a, const DocObjAndOutput& b)
  {
    return a.m_pObject == b.m_pObject && a.m_szOutputName == b.m_szOutputName;
  }
};

struct plProcGenGraphAssetDocument::GenerateContext
{
  GenerateContext(const plDocumentObjectManager* pManager)
    : m_ObjectWriter(&m_AbstractObjectGraph, pManager)
    , m_RttiConverter(&m_AbstractObjectGraph, &m_RttiConverterContext)
  {
  }

  plAbstractObjectGraph m_AbstractObjectGraph;
  plDocumentObjectConverterWriter m_ObjectWriter;
  plRttiConverterContext m_RttiConverterContext;
  plRttiConverterReader m_RttiConverter;
  plHashTable<const plDocumentObject*, plUniquePtr<plProcGenNodeBase>> m_DocObjToProcGenNodeTable;
  plHashTable<DocObjAndOutput, plExpressionAST::Node*> m_DocObjAndOutputToASTNodeTable;
  plProcGenNodeBase::GraphContext m_GraphContext;
};

////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenGraphAssetDocument, 7, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plProcGenGraphAssetDocument::plProcGenGraphAssetDocument(plStringView sDocumentPath)
  : plAssetDocument(sDocumentPath, PL_DEFAULT_NEW(plProcGenNodeManager), plAssetDocEngineConnection::None)
{
}

void plProcGenGraphAssetDocument::SetDebugPin(const plPin* pDebugPin)
{
  m_pDebugPin = pDebugPin;

  if (m_pDebugPin != nullptr)
  {
    CreateDebugNode();
  }

  plDocumentObjectPropertyEvent e;
  e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_sProperty = "DebugPin";

  GetObjectManager()->m_PropertyEvents.Broadcast(e);
}

plStatus plProcGenGraphAssetDocument::WriteAsset(plStreamWriter& inout_stream, const plPlatformProfile* pAssetProfile, bool bAllowDebug) const
{
  GenerateContext context(GetObjectManager());

  plDynamicArray<const plDocumentObject*> placementNodes;
  plDynamicArray<const plDocumentObject*> vertexColorNodes;
  GetAllOutputNodes(placementNodes, vertexColorNodes);

  const bool bDebug = bAllowDebug && (m_pDebugPin != nullptr);

  plStringDeduplicationWriteContext stringDedupContext(inout_stream);

  plChunkStreamWriter chunk(stringDedupContext.Begin());
  chunk.BeginStream(1);

  plExpressionCompiler compiler;

  auto WriteByteCode = [&](const plDocumentObject* pOutputNode)->plStatus {
    context.m_GraphContext.m_VolumeTagSetIndices.Clear();

    if (pOutputNode->GetType()->IsDerivedFrom<plProcGen_PlacementOutput>())
    {
      context.m_GraphContext.m_OutputType = plProcGenNodeBase::GraphContext::Placement;
    }
    else if (pOutputNode->GetType()->IsDerivedFrom<plProcGen_VertexColorOutput>())
    {
      context.m_GraphContext.m_OutputType = plProcGenNodeBase::GraphContext::Color;
    }
    else
    {
      PL_ASSERT_NOT_IMPLEMENTED;
      return plStatus("Unknown output type");
    }

    plExpressionAST ast;
    GenerateExpressionAST(pOutputNode, "", context, ast);
    context.m_DocObjAndOutputToASTNodeTable.Clear();

    if (false)
    {
      plStringBuilder sDocumentPath = GetDocumentPath();
      plStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
      plStringView sOutputName = pOutputNode->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();

      DumpAST(ast, sAssetName, sOutputName);
    }

    plExpressionByteCode byteCode;
    if (compiler.Compile(ast, byteCode).Failed())
    {
      return plStatus("Compilation failed");
    }

    PL_SUCCEED_OR_RETURN(byteCode.Save(chunk));

    return plStatus(PL_SUCCESS);
  };

  {
    chunk.BeginChunk("PlacementOutputs", 7);

    if (!bDebug)
    {
      chunk << placementNodes.GetCount();

      for (auto pPlacementNode : placementNodes)
      {
        PL_SUCCEED_OR_RETURN(WriteByteCode(pPlacementNode));

        auto pPGNode = context.m_DocObjToProcGenNodeTable.GetValue(pPlacementNode);
        auto pPlacementOutput = plStaticCast<plProcGen_PlacementOutput*>(pPGNode->Borrow());

        pPlacementOutput->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
        pPlacementOutput->Save(chunk);
      }
    }
    else
    {
      plUInt32 uiNumNodes = 1;
      chunk << uiNumNodes;

      context.m_GraphContext.m_VolumeTagSetIndices.Clear();
      context.m_GraphContext.m_OutputType = plProcGenNodeBase::GraphContext::Placement;

      plExpressionAST ast;
      GenerateDebugExpressionAST(context, ast);
      context.m_DocObjAndOutputToASTNodeTable.Clear();

      plExpressionByteCode byteCode;
      if (compiler.Compile(ast, byteCode).Failed())
      {
        return plStatus("Debug Compilation failed");
      }

      PL_SUCCEED_OR_RETURN(byteCode.Save(chunk));

      m_pDebugNode->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
      m_pDebugNode->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexColorOutputs", 2);

    chunk << vertexColorNodes.GetCount();

    for (auto pVertexColorNode : vertexColorNodes)
    {
      PL_SUCCEED_OR_RETURN(WriteByteCode(pVertexColorNode));

      auto pPGNode = context.m_DocObjToProcGenNodeTable.GetValue(pVertexColorNode);
      auto pVertexColorOutput = plStaticCast<plProcGen_VertexColorOutput*>(pPGNode->Borrow());

      pVertexColorOutput->m_VolumeTagSetIndices = context.m_GraphContext.m_VolumeTagSetIndices;
      pVertexColorOutput->Save(chunk);
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SharedData", 1);

    context.m_GraphContext.m_SharedData.Save(chunk);

    chunk.EndChunk();
  }

  chunk.EndStream();
  PL_SUCCEED_OR_RETURN(stringDedupContext.End());

  return plStatus(PL_SUCCESS);
}

void plProcGenGraphAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (m_pDebugPin == nullptr)
  {
    plDynamicArray<const plDocumentObject*> placementNodes;
    plDynamicArray<const plDocumentObject*> vertexColorNodes;
    GetAllOutputNodes(placementNodes, vertexColorNodes);

    for (auto pPlacementNode : placementNodes)
    {
      auto& typeAccessor = pPlacementNode->GetTypeAccessor();

      plUInt32 uiNumObjects = typeAccessor.GetCount("Objects");
      for (plUInt32 i = 0; i < uiNumObjects; ++i)
      {
        plVariant prefab = typeAccessor.GetValue("Objects", i);
        if (prefab.IsA<plString>())
        {
          pInfo->m_PackageDependencies.Insert(prefab.Get<plString>());
          pInfo->m_ThumbnailDependencies.Insert(prefab.Get<plString>());
        }
      }

      plVariant colorGradient = typeAccessor.GetValue("ColorGradient");
      if (colorGradient.IsA<plString>())
      {
        pInfo->m_PackageDependencies.Insert(colorGradient.Get<plString>());
        pInfo->m_ThumbnailDependencies.Insert(colorGradient.Get<plString>());
      }
    }
  }
  else
  {
    pInfo->m_PackageDependencies.Insert(s_szSphereAssetId);
    pInfo->m_PackageDependencies.Insert(s_szBWGradientAssetId);

    pInfo->m_ThumbnailDependencies.Insert(s_szSphereAssetId);
    pInfo->m_ThumbnailDependencies.Insert(s_szBWGradientAssetId);
  }
}

plTransformStatus plProcGenGraphAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  PL_ASSERT_DEV(sOutputTag.IsEmpty(), "Additional output '{0}' not implemented!", sOutputTag);

  return WriteAsset(stream, pAssetProfile, false);
}

void plProcGenGraphAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/plEditor.ProcGenGraph");
}

bool plProcGenGraphAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/plEditor.ProcGenGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plProcGenGraphAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void plProcGenGraphAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void plProcGenGraphAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void plProcGenGraphAssetDocument::GetAllOutputNodes(plDynamicArray<const plDocumentObject*>& placementNodes, plDynamicArray<const plDocumentObject*>& vertexColorNodes) const
{
  const plRTTI* pPlacementOutputRtti = plGetStaticRTTI<plProcGen_PlacementOutput>();
  const plRTTI* pVertexColorOutputRtti = plGetStaticRTTI<plProcGen_VertexColorOutput>();

  placementNodes.Clear();
  vertexColorNodes.Clear();

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const plDocumentObject* pObject : children)
  {
    if (pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>())
    {
      const plRTTI* pRtti = pObject->GetTypeAccessor().GetType();
      if (pRtti->IsDerivedFrom(pPlacementOutputRtti))
      {
        placementNodes.PushBack(pObject);
      }
      else if (pRtti->IsDerivedFrom(pVertexColorOutputRtti))
      {
        vertexColorNodes.PushBack(pObject);
      }
    }
  }
}

void plProcGenGraphAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

plExpressionAST::Node* plProcGenGraphAssetDocument::GenerateExpressionAST(const plDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, plExpressionAST& out_Ast) const
{
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());

  auto inputPins = pManager->GetInputPins(outputNode);

  plHybridArray<plExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(inputPins.GetCount());

  for (plUInt32 i = 0; i < inputPins.GetCount(); ++i)
  {
    auto connections = pManager->GetConnections(*inputPins[i]);
    PL_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const plPin& pinSource = connections[0]->GetSourcePin();

    DocObjAndOutput key = {pinSource.GetParent(), pinSource.GetName()};
    plExpressionAST::Node* astNode;
    if (!context.m_DocObjAndOutputToASTNodeTable.TryGetValue(key, astNode))
    {
      // recursively generate all dependent code
      astNode = GenerateExpressionAST(pinSource.GetParent(), pinSource.GetName(), context, out_Ast);

      context.m_DocObjAndOutputToASTNodeTable.Insert(key, astNode);
    }

    inputAstNodes[i] = astNode;
  }

  plProcGenNodeBase* cachedPGNode = nullptr;
  if (auto pCachedPGNode = context.m_DocObjToProcGenNodeTable.GetValue(outputNode))
  {
    cachedPGNode = pCachedPGNode->Borrow();
  }
  else
  {
    plAbstractObjectNode* pAbstractNode = context.m_ObjectWriter.AddObjectToGraph(outputNode);
    auto newPGNode = context.m_RttiConverter.CreateObjectFromNode(pAbstractNode).Cast<plProcGenNodeBase>();
    cachedPGNode = newPGNode;

    context.m_DocObjToProcGenNodeTable.Insert(outputNode, newPGNode);
  }

  return cachedPGNode->GenerateExpressionASTNode(plTempHashedString(szOutputName), inputAstNodes, out_Ast, context.m_GraphContext);
}

plExpressionAST::Node* plProcGenGraphAssetDocument::GenerateDebugExpressionAST(GenerateContext& context, plExpressionAST& out_Ast) const
{
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  PL_ASSERT_DEV(m_pDebugPin != nullptr, "");

  const plPin* pPinSource = m_pDebugPin;
  if (pPinSource->GetType() == plPin::Type::Input)
  {
    auto connections = pManager->GetConnections(*pPinSource);
    PL_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      return nullptr;

    pPinSource = &connections[0]->GetSourcePin();
    PL_ASSERT_DEBUG(pPinSource != nullptr, "Invalid connection");
  }

  plHybridArray<plExpressionAST::Node*, 8> inputAstNodes;
  inputAstNodes.SetCount(4); // placement output node has 4 inputs

  // Recursively generate all dependent code and pretend it is connected to the color index input of the debug placement output node.
  inputAstNodes[2] = GenerateExpressionAST(pPinSource->GetParent(), pPinSource->GetName(), context, out_Ast);

  return m_pDebugNode->GenerateExpressionASTNode("", inputAstNodes, out_Ast, context.m_GraphContext);
}

void plProcGenGraphAssetDocument::DumpSelectedOutput(bool bAst, bool bDisassembly) const
{
  const plDocumentObject* pSelectedNode = nullptr;

  auto selection = GetSelectionManager()->GetSelection();
  if (!selection.IsEmpty())
  {
    pSelectedNode = selection[0];
    if (!pSelectedNode->GetType()->IsDerivedFrom<plProcGenOutput>())
    {
      pSelectedNode = nullptr;
    }
  }

  if (pSelectedNode == nullptr)
  {
    plLog::Error("No valid output node selected.");
    return;
  }

  GenerateContext context(GetObjectManager());
  if (pSelectedNode->GetType()->IsDerivedFrom<plProcGen_PlacementOutput>())
  {
    context.m_GraphContext.m_OutputType = plProcGenNodeBase::GraphContext::Placement;
  }
  else if (pSelectedNode->GetType()->IsDerivedFrom<plProcGen_VertexColorOutput>())
  {
    context.m_GraphContext.m_OutputType = plProcGenNodeBase::GraphContext::Color;
  }
  else
  {
    PL_ASSERT_NOT_IMPLEMENTED;
    return;
  }

  plExpressionAST ast;
  GenerateExpressionAST(pSelectedNode, "", context, ast);

  plStringBuilder sDocumentPath = GetDocumentPath();
  plStringView sAssetName = sDocumentPath.GetFileNameAndExtension();
  plStringView sOutputName = pSelectedNode->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();

  if (bAst)
  {
    DumpAST(ast, sAssetName, sOutputName);
  }

  plExpressionByteCode byteCode;
  plExpressionCompiler compiler;
  if (compiler.Compile(ast, byteCode).Failed())
  {
    plLog::Error("Compiling expression failed");
    return;
  }

  if (bAst)
  {
    plStringBuilder sOutputName2 = sOutputName;
    sOutputName2.Append("_Opt");

    DumpAST(ast, sAssetName, sOutputName2);
  }

  if (bDisassembly)
  {
    plStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    plStringBuilder sFileName;
    sFileName.SetFormat(":appdata/{0}_{1}_ByteCode.txt", sAssetName, sOutputName);

    plFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      plLog::Info("Disassembly was dumped to: {0}", sFileName);
    }
    else
    {
      plLog::Error("Failed to dump Disassembly to: {0}", sFileName);
    }
  }
}

void plProcGenGraphAssetDocument::CreateDebugNode()
{
  if (m_pDebugNode != nullptr)
    return;

  m_pDebugNode = PL_DEFAULT_NEW(plProcGen_PlacementOutput);
  m_pDebugNode->m_sName = "Debug";
  m_pDebugNode->m_ObjectsToPlace.PushBack(s_szSphereAssetId);
  m_pDebugNode->m_sColorGradient = s_szBWGradientAssetId;
  m_pDebugNode->m_PlacementPattern = plProcPlacementPattern::RegularGrid;
}
