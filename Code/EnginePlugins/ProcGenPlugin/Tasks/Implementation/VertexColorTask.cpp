#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <typename T>
  PL_ALWAYS_INLINE plProcessingStream MakeStream(plArrayPtr<T> data, plUInt32 uiOffset, const plHashedString& sName, plProcessingStream::DataType dataType = plProcessingStream::DataType::Float)
  {
    return plProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
  }

  PL_ALWAYS_INLINE float Remap(plEnum<plProcVertexColorChannelMapping> channelMapping, const plColor& srcColor)
  {
    if (channelMapping >= plProcVertexColorChannelMapping::R && channelMapping <= plProcVertexColorChannelMapping::A)
    {
      return (&srcColor.r)[channelMapping];
    }
    else
    {
      return channelMapping == plProcVertexColorChannelMapping::White ? 1.0f : 0.0f;
    }
  }
} // namespace

using namespace plProcGenInternal;

VertexColorTask::VertexColorTask()
{
  m_VM.RegisterFunction(plProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(plProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const plWorld& world, const plMeshBufferResourceDescriptor& desc, const plTransform& transform, plArrayPtr<plSharedPtr<const VertexColorOutput>> outputs, plArrayPtr<plProcVertexColorMapping> outputMappings, plArrayPtr<plUInt32> outputVertexColors)
{
  PL_PROFILE_SCOPE("VertexColorPrepare");

  m_InputVertices.Clear();
  m_InputVertices.Reserve(desc.GetVertexCount());

  const plVertexDeclarationInfo& vdi = desc.GetVertexDeclaration();
  const plUInt8* pRawVertexData = desc.GetVertexBufferData().GetPtr();

  const float* pPositions = nullptr;
  const plUInt8* pNormals = nullptr;
  plGALResourceFormat::Enum normalFormat = plGALResourceFormat::Invalid;
  const plColorLinearUB* pColors = nullptr;

  for (plUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != plGALResourceFormat::RGBFloat)
      {
        plLog::Error("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const float*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Normal)
    {
      pNormals = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Color0)
    {
      if (vdi.m_VertexStreams[vs].m_Format != plGALResourceFormat::RGBAUByteNormalized)
      {
        plLog::Error("Unsupported CPU mesh vertex color format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other color formats are not supported
      }

      pColors = reinterpret_cast<const plColorLinearUB*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    plLog::Error("No position and normal stream found in CPU mesh");
    return;
  }

  plUInt8 dummySource[16] = {};
  plVec3 vNormal;
  if (plMeshBufferUtils::DecodeNormal(plMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    plLog::Error("Unsupported CPU mesh vertex normal format {0}", normalFormat);
    return;
  }

  plMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
  normalTransform.Invert(0.0f).IgnoreResult();
  normalTransform.Transpose();

  const plUInt32 uiElementStride = desc.GetVertexDataSize();

  // write out all vertices
  for (plUInt32 i = 0; i < desc.GetVertexCount(); ++i)
  {
    plMeshBufferUtils::DecodeNormal(plMakeArrayPtr(pNormals, sizeof(plVec3)), normalFormat, vNormal).IgnoreResult();

    auto& vert = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(plVec3(pPositions[0], pPositions[1], pPositions[2]));
    vert.m_vNormal = normalTransform.TransformDirection(vNormal).GetNormalized();
    vert.m_Color = pColors != nullptr ? plColor(*pColors) : plColor::MakeZero();
    vert.m_uiIndex = i;

    pPositions = plMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    pNormals = plMemoryUtils::AddByteOffset(pNormals, uiElementStride);
    pColors = pColors != nullptr ? plMemoryUtils::AddByteOffset(pColors, uiElementStride) : nullptr;
  }

  m_Outputs = outputs;
  m_OutputMappings = outputMappings;
  m_OutputVertexColors = outputVertexColors;

  //////////////////////////////////////////////////////////////////////////

  // TODO:
  // plBoundingBox box = mbDesc.GetBounds();
  plBoundingBox box = plBoundingBox::MakeFromMinMax(plVec3(-1000), plVec3(1000));
  box.TransformFromOrigin(transform.GetAsMat4());

  m_VolumeCollections.Clear();
  m_GlobalData.Clear();

  for (auto& pOutput : outputs)
  {
    if (pOutput != nullptr)
    {
      plProcGenInternal::ExtractVolumeCollections(world, box, *pOutput, m_VolumeCollections, m_GlobalData);
    }
  }

  const plUInt32 uiTransformHash = plHashingUtils::xxHash32(&transform, sizeof(plTransform));
  plProcGenInternal::SetInstanceSeed(uiTransformHash, m_GlobalData);
}

void VertexColorTask::Execute()
{
  if (m_InputVertices.IsEmpty())
    return;

  const plUInt32 uiNumOutputs = m_Outputs.GetCount();
  for (plUInt32 uiOutputIndex = 0; uiOutputIndex < uiNumOutputs; ++uiOutputIndex)
  {
    auto& pOutput = m_Outputs[uiOutputIndex];
    if (pOutput == nullptr || pOutput->m_pByteCode == nullptr)
      continue;

    PL_PROFILE_SCOPE("ExecuteVM");

    plUInt32 uiNumVertices = m_InputVertices.GetCount();
    m_TempData.SetCountUninitialized(uiNumVertices);

    plHybridArray<plProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.r), ExpressionInputs::s_sColorR));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.g), ExpressionInputs::s_sColorG));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.b), ExpressionInputs::s_sColorB));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.a), ExpressionInputs::s_sColorA));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_uiIndex), ExpressionInputs::s_sPointIndex, plProcessingStream::DataType::Int));
    }

    plHybridArray<plProcessingStream, 8> outputs;
    {
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(plColor, r), ExpressionOutputs::s_sOutColorR));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(plColor, g), ExpressionOutputs::s_sOutColorG));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(plColor, b), ExpressionOutputs::s_sOutColorB));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(plColor, a), ExpressionOutputs::s_sOutColorA));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumVertices, m_GlobalData, plExpressionVM::Flags::BestPerformance).Failed())
    {
      continue;
    }

    auto& outputMapping = m_OutputMappings[uiOutputIndex];
    for (plUInt32 i = 0; i < uiNumVertices; ++i)
    {
      plColor srcColor = m_TempData[i];
      plColor remappedColor;
      remappedColor.r = Remap(outputMapping.m_R, srcColor);
      remappedColor.g = Remap(outputMapping.m_G, srcColor);
      remappedColor.b = Remap(outputMapping.m_B, srcColor);
      remappedColor.a = Remap(outputMapping.m_A, srcColor);

      plColorLinearUB vertexColor = remappedColor;

      // Store output vertex colors interleaved
      m_OutputVertexColors[i * uiNumOutputs + uiOutputIndex] = *reinterpret_cast<plUInt32*>(&vertexColor.r);
    }
  }
}
