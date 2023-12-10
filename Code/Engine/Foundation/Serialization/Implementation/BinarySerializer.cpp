#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>

enum plBinarySerializerVersion : plUInt32
{
  InvalidVersion = 0,
  Version1,
  // << insert new versions here >>

  ENUM_COUNT,
  CurrentVersion = ENUM_COUNT - 1 // automatically the highest version number
};

static void WriteGraph(const plAbstractObjectGraph* pGraph, plStreamWriter& inout_stream)
{
  const auto& Nodes = pGraph->GetAllNodes();

  plUInt32 uiNodes = Nodes.GetCount();
  inout_stream << uiNodes;
  for (auto itNode = Nodes.GetIterator(); itNode.IsValid(); ++itNode)
  {
    const auto& node = *itNode.Value();
    inout_stream << node.GetGuid();
    inout_stream << node.GetType();
    inout_stream << node.GetTypeVersion();
    inout_stream << node.GetNodeName();

    const plHybridArray<plAbstractObjectNode::Property, 16>& properties = node.GetProperties();
    plUInt32 uiProps = properties.GetCount();
    inout_stream << uiProps;
    for (const plAbstractObjectNode::Property& prop : properties)
    {
      inout_stream << prop.m_sPropertyName;
      inout_stream << prop.m_Value;
    }
  }
}

void plAbstractGraphBinarySerializer::Write(plStreamWriter& inout_stream, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph)
{
  plUInt32 uiVersion = plBinarySerializerVersion::CurrentVersion;
  inout_stream << uiVersion;

  WriteGraph(pGraph, inout_stream);
  if (pTypesGraph)
  {
    WriteGraph(pTypesGraph, inout_stream);
  }
}

static void ReadGraph(plStreamReader& inout_stream, plAbstractObjectGraph* pGraph)
{
  plUInt32 uiNodes = 0;
  inout_stream >> uiNodes;
  for (plUInt32 uiNodeIdx = 0; uiNodeIdx < uiNodes; uiNodeIdx++)
  {
    plUuid guid;
    plUInt32 uiTypeVersion;
    plStringBuilder sType;
    plStringBuilder sNodeName;
    inout_stream >> guid;
    inout_stream >> sType;
    inout_stream >> uiTypeVersion;
    inout_stream >> sNodeName;
    plAbstractObjectNode* pNode = pGraph->AddNode(guid, sType, uiTypeVersion, sNodeName);
    plUInt32 uiProps = 0;
    inout_stream >> uiProps;
    for (plUInt32 propIdx = 0; propIdx < uiProps; ++propIdx)
    {
      plStringBuilder sPropName;
      plVariant value;
      inout_stream >> sPropName;
      inout_stream >> value;
      pNode->AddProperty(sPropName, value);
    }
  }
}

void plAbstractGraphBinarySerializer::Read(
  plStreamReader& inout_stream, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph, bool bApplyPatches)
{
  plUInt32 uiVersion = 0;
  inout_stream >> uiVersion;
  if (uiVersion != plBinarySerializerVersion::CurrentVersion)
  {
   // PLASMA_REPORT_FAILURE(
   //   "Binary serializer version {0} does not match expected version {1}, re-export file.", uiVersion, plBinarySerializerVersion::CurrentVersion);
    return;
  }
  ReadGraph(inout_stream, pGraph);
  if (pTypesGraph)
  {
    ReadGraph(inout_stream, pTypesGraph);
  }

  if (bApplyPatches)
  {
    if (pTypesGraph)
      plGraphVersioning::GetSingleton()->PatchGraph(pTypesGraph);
    plGraphVersioning::GetSingleton()->PatchGraph(pGraph, pTypesGraph);
  }
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_BinarySerializer);
