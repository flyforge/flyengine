#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
plCVarBool plRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, plCVarFlags::Default, "Enables debug visualization of visibility culling");
plCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, plCVarFlags::Default, "Display some stats of the visibility culling");
#endif

plCVarBool cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, plCVarFlags::Default, "Use software rasterization for occlusion culling.");
plCVarBool cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, plCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
plCVarFloat cvar_SpatialCullingOcclusionBoundsInlation("Spatial.Occlusion.BoundsInflation", 0.5f, plCVarFlags::Default, "How much to inflate bounds during occlusion check.");
plCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, plCVarFlags::Default, "Far plane distance for finding occluders.");

plRenderPipeline::plRenderPipeline()

{
  m_CurrentExtractThread = (plThreadID)0;
  m_CurrentRenderThread = (plThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = plTime::MakeFromSeconds(0.1f);
#endif
}

plRenderPipeline::~plRenderPipeline()
{
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
    m_hOcclusionDebugViewTexture.Invalidate();
  }

  m_Data[0].Clear();
  m_Data[1].Clear();

  ClearRenderPassGraphTextures();
  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
  }
}

void plRenderPipeline::AddPass(plUniquePtr<plRenderPipelinePass>&& pPass)
{
  m_PipelineState = PipelineState::Uninitialized;
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  auto it = m_Connections.Insert(pPass.Borrow(), ConnectionData());
  it.Value().m_Inputs.SetCount(pPass->GetInputPins().GetCount());
  it.Value().m_Outputs.SetCount(pPass->GetOutputPins().GetCount());
  m_Passes.PushBack(std::move(pPass));
}

void plRenderPipeline::RemovePass(plRenderPipelinePass* pPass)
{
  for (plUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Passes[i].Borrow() == pPass)
    {
      m_PipelineState = PipelineState::Uninitialized;
      RemoveConnections(pPass);
      m_Connections.Remove(pPass);
      pPass->m_pPipeline = nullptr;
      m_Passes.RemoveAtAndCopy(i);
      break;
    }
  }
}

void plRenderPipeline::GetPasses(plDynamicArray<const plRenderPipelinePass*>& ref_passes) const
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void plRenderPipeline::GetPasses(plDynamicArray<plRenderPipelinePass*>& ref_passes)
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

plRenderPipelinePass* plRenderPipeline::GetPassByName(const plStringView& sPassName)
{
  for (auto& pPass : m_Passes)
  {
    if (sPassName.IsEqual(pPass->GetName()))
    {
      return pPass.Borrow();
    }
  }

  return nullptr;
}

plHashedString plRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool plRenderPipeline::Connect(plRenderPipelinePass* pOutputNode, const char* szOutputPinName, plRenderPipelinePass* pInputNode, const char* szInputPinName)
{
  plHashedString sOutputPinName;
  sOutputPinName.Assign(szOutputPinName);
  plHashedString sInputPinName;
  sInputPinName.Assign(szInputPinName);
  return Connect(pOutputNode, sOutputPinName, pInputNode, sInputPinName);
}

bool plRenderPipeline::Connect(plRenderPipelinePass* pOutputNode, plHashedString sOutputPinName, plRenderPipelinePass* pInputNode, plHashedString sInputPinName)
{
  plLogBlock b("plRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    plLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    plLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const plRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    plLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const plRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    plLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    plLog::Error("Pins already connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Add at output
  plRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection = PLASMA_DEFAULT_NEW(plRenderPipelinePassConnection);
    pConnection->m_pOutput = pPinSource;
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected
    if (pPinTarget->m_Type == plRenderPipelineNodePin::Type::PassThrough)
    {
      for (const plRenderPipelineNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Type == plRenderPipelineNodePin::Type::PassThrough)
        {
          plLog::Error("A pass through pin is already connected to the '{0}' pin!", sOutputPinName);
          return false;
        }
      }
    }
  }

  // Add at input
  pConnection->m_Inputs.PushBack(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = pConnection;
  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

bool plRenderPipeline::Disconnect(plRenderPipelinePass* pOutputNode, plHashedString sOutputPinName, plRenderPipelinePass* pInputNode, plHashedString sInputPinName)
{
  plLogBlock b("plRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    plLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    plLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const plRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    plLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const plRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    plLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr || itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    plLog::Error("Pins not connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Remove at input
  plRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  pConnection->m_Inputs.RemoveAndCopy(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = nullptr;

  if (pConnection->m_Inputs.IsEmpty())
  {
    // Remove at output
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = nullptr;
    PLASMA_DEFAULT_DELETE(pConnection);
  }

  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

const plRenderPipelinePassConnection* plRenderPipeline::GetInputConnection(const plRenderPipelinePass* pPass, plHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const plRenderPipelineNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFF)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const plRenderPipelinePassConnection* plRenderPipeline::GetOutputConnection(const plRenderPipelinePass* pPass, plHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const plRenderPipelineNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin)
    return nullptr;

  return data.m_Outputs[pPin->m_uiOutputIndex];
}

plRenderPipeline::PipelineState plRenderPipeline::Rebuild(const plView& view)
{
  plLogBlock b("plRenderPipeline::Rebuild");

  ClearRenderPassGraphTextures();

  bool bRes = RebuildInternal(view);
  if (!bRes)
  {
    ClearRenderPassGraphTextures();
  }
  else
  {
    // make sure the renderdata stores the updated view data
    UpdateViewData(view, plRenderWorld::GetDataIndexForRendering());
  }

  m_PipelineState = bRes ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

bool plRenderPipeline::RebuildInternal(const plView& view)
{
  if (!SortPasses())
    return false;
  if (!InitRenderTargetDescriptions(view))
    return false;
  if (!CreateRenderTargetUsage(view))
    return false;
  if (!InitRenderPipelinePasses())
    return false;

  SortExtractors();

  return true;
}

bool plRenderPipeline::SortPasses()
{
  plLogBlock b("Sort Passes");
  plHybridArray<plRenderPipelinePass*, 32> done;
  done.Reserve(m_Passes.GetCount());

  plHybridArray<plRenderPipelinePass*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  plHybridArray<plRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    // if (std::all_of(cbegin(it.Value().m_Inputs), cend(it.Value().m_Inputs), [](plRenderPipelinePassConnection* pConn){return pConn ==
    // nullptr; }))
    if (AreInputDescriptionsAvailable(pPass.Borrow(), done))
    {
      usable.PushBack(pPass.Borrow());
    }
  }

  // Via a depth first traversal, order the passes
  while (!usable.IsEmpty())
  {
    plRenderPipelinePass* pPass = usable.PeekBack();
    plLogBlock b2("Traverse", pPass->GetName());

    usable.PopBack();
    ConnectionData& data = m_Connections[pPass];

    PLASMA_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    PLASMA_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    // Check for new candidate passes. Can't be done in the previous loop as multiple connections may be required by a node.
    for (plUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        // Go through all inputs this connection is connected to and test the corresponding node for availability
        for (const plRenderPipelineNodePin* pPin : data.m_Outputs[i]->m_Inputs)
        {
          PLASMA_ASSERT_DEBUG(pPin->m_pParent != nullptr, "Pass was not initialized!");
          plRenderPipelinePass* pTargetPass = static_cast<plRenderPipelinePass*>(pPin->m_pParent);
          if (done.Contains(pTargetPass))
          {
            plLog::Error("Loop detected, graph not supported!");
            return false;
          }

          if (!usable.Contains(pTargetPass) && !candidates.Contains(pTargetPass))
          {
            candidates.PushBack(pTargetPass);
          }
        }
      }
    }

    done.PushBack(pPass);

    // Check for usable candidates. Reverse order for depth first traversal.
    for (plInt32 i = (plInt32)candidates.GetCount() - 1; i >= 0; i--)
    {
      plRenderPipelinePass* pCandidatePass = candidates[i];
      if (AreInputDescriptionsAvailable(pCandidatePass, done) && ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAtAndCopy(i);
      }
    }
  }

  if (done.GetCount() < m_Passes.GetCount())
  {
    plLog::Error("Pipeline: Not all nodes could be initialized");
    for (auto& pass : m_Passes)
    {
      if (!done.Contains(pass.Borrow()))
      {
        plLog::Error("Failed to initialize node: {} - {}", pass->GetName(), pass->GetDynamicRTTI()->GetTypeName());
      }
    }
    return false;
  }

  struct plPipelineSorter
  {
    /// \brief Returns true if a is less than b
    PLASMA_FORCE_INLINE bool Less(const plUniquePtr<plRenderPipelinePass>& a, const plUniquePtr<plRenderPipelinePass>& b) const { return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow()); }

    /// \brief Returns true if a is equal to b
    PLASMA_ALWAYS_INLINE bool Equal(const plUniquePtr<plRenderPipelinePass>& a, const plUniquePtr<plRenderPipelinePass>& b) const { return a.Borrow() == b.Borrow(); }

    plHybridArray<plRenderPipelinePass*, 32>* m_pDone;
  };

  plPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool plRenderPipeline::InitRenderTargetDescriptions(const plView& view)
{
  plLogBlock b("Init Render Target Descriptions");
  plHybridArray<plGALTextureCreationDescription*, 10> inputs;
  plHybridArray<plGALTextureCreationDescription, 10> outputs;

  for (auto& pPass : m_Passes)
  {
    plLogBlock b2("InitPass", pPass->GetName());

    if (view.GetCamera()->IsStereoscopic() && !pPass->IsStereoAware())
    {
      plLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", view.GetName(), pPass->GetName());
    }

    ConnectionData& data = m_Connections[pPass.Borrow()];

    PLASMA_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    PLASMA_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    inputs.SetCount(data.m_Inputs.GetCount());
    outputs.Clear();
    outputs.SetCount(data.m_Outputs.GetCount());
    // Fill inputs array
    for (plUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
    {
      if (data.m_Inputs[i] != nullptr)
      {
        inputs[i] = &data.m_Inputs[i]->m_Desc;
      }
      else
      {
        inputs[i] = nullptr;
      }
    }

    bool bRes = pPass->GetRenderTargetDescriptions(view, inputs, outputs);
    if (!bRes)
    {
      plLog::Error("The pass could not be successfully queried for render target descriptions.");
      return false;
    }

    // Copy queried outputs into the output connections.
    for (plUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Desc = outputs[i];
      }
    }

    // Check pass-through consistency of input / output target desc.
    auto inputPins = pPass->GetInputPins();
    for (const plRenderPipelineNodePin* pPin : inputPins)
    {
      if (pPin->m_Type == plRenderPipelineNodePin::Type::PassThrough)
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            // plLog::Error("The pass of type '{0}' has a pass through pin '{1}' that has an output but no input!",
            // pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));  return false;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Desc.CalculateHash() != data.m_Inputs[pPin->m_uiInputIndex]->m_Desc.CalculateHash())
          {
            plLog::Error("The pass has a pass through pin '{0}' that has different descriptors for input and output!", pPass->GetPinName(pPin));
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool plRenderPipeline::CreateRenderTargetUsage(const plView& view)
{
  plLogBlock b("Create Render Target Usage Data");
  PLASMA_ASSERT_DEBUG(m_TextureUsage.IsEmpty(), "Need to call ClearRenderPassGraphTextures before re-creating the pipeline.");

  m_ConnectionToTextureIndex.Clear();

  // Gather all connections that share the same path-through texture and their first and last usage pass index.
  for (plUInt16 i = 0; i < static_cast<plUInt16>(m_Passes.GetCount()); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    ConnectionData& data = m_Connections[pPass];
    for (plRenderPipelinePassConnection* pConn : data.m_Inputs)
    {
      if (pConn != nullptr)
      {
        plUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
        m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (plRenderPipelinePassConnection* pConn : data.m_Outputs)
    {
      if (pConn != nullptr)
      {
        if (pConn->m_pOutput->m_Type == plRenderPipelineNodePin::Type::PassThrough && data.m_Inputs[pConn->m_pOutput->m_uiInputIndex] != nullptr)
        {
          plRenderPipelinePassConnection* pCorrespondingInputConn = data.m_Inputs[pConn->m_pOutput->m_uiInputIndex];
          PLASMA_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pCorrespondingInputConn), "");
          plUInt32 uiDataIdx = m_ConnectionToTextureIndex[pCorrespondingInputConn];
          m_TextureUsage[uiDataIdx].m_UsedBy.PushBack(pConn);
          m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;

          PLASMA_ASSERT_DEV(!m_ConnectionToTextureIndex.Contains(pConn), "");
          m_ConnectionToTextureIndex[pConn] = uiDataIdx;
        }
        else
        {
          m_ConnectionToTextureIndex[pConn] = m_TextureUsage.GetCount();
          TextureUsageData& texData = m_TextureUsage.ExpandAndGetRef();

          texData.m_iTargetTextureIndex = -1;
          texData.m_uiFirstUsageIdx = i;
          texData.m_uiLastUsageIdx = i;
          texData.m_UsedBy.PushBack(pConn);
        }
      }
    }
  }

  static plUInt32 defaultTextureDescHash = plGALTextureCreationDescription().CalculateHash();
  // Set view's render target textures to target pass connections.
  for (plUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    if (pPass->IsInstanceOf<plTargetPass>())
    {
      const plGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

      plTargetPass* pTargetPass = static_cast<plTargetPass*>(pPass);
      ConnectionData& data = m_Connections[pPass];
      for (plUInt32 j = 0; j < data.m_Inputs.GetCount(); j++)
      {
        plRenderPipelinePassConnection* pConn = data.m_Inputs[j];
        if (pConn != nullptr)
        {
          const plGALTextureHandle* hTexture = pTargetPass->GetTextureHandle(renderTargets, pPass->GetInputPins()[j]);
          PLASMA_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pConn), "");

          plUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
          if (!hTexture)
          {
            m_TextureUsage[uiDataIdx].m_iTargetTextureIndex = -1;
            for (auto pUsedByConn : m_TextureUsage[uiDataIdx].m_UsedBy)
            {
              pUsedByConn->m_TextureHandle.Invalidate();
            }
          }
          else if (!hTexture->IsInvalidated() || pConn->m_Desc.CalculateHash() == defaultTextureDescHash)
          {
            m_TextureUsage[uiDataIdx].m_iTargetTextureIndex = static_cast<plInt32>(hTexture - reinterpret_cast<const plGALTextureHandle*>(&renderTargets));
            PLASMA_ASSERT_DEV(reinterpret_cast<const plGALTextureHandle*>(&renderTargets)[m_TextureUsage[uiDataIdx].m_iTargetTextureIndex] == *hTexture, "Offset computation broken.");

            for (auto pUsedByConn : m_TextureUsage[uiDataIdx].m_UsedBy)
            {
              pUsedByConn->m_TextureHandle = *hTexture;
            }
          }
          else
          {
            // In this case, the plTargetPass does not provide a render target for the connection but the descriptor is set so we can instead use the pool to supplement the missing texture.
          }
        }
      }
    }
  }

  // Stupid loop to gather all TextureUsageData indices that are not view render target textures.
  for (plUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
  {
    TextureUsageData& data = m_TextureUsage[i];
    if (data.m_iTargetTextureIndex != -1)
      continue;

    m_TextureUsageIdxSortedByFirstUsage.PushBack((plUInt16)i);
    m_TextureUsageIdxSortedByLastUsage.PushBack((plUInt16)i);
  }

  // Sort first and last usage arrays, these will determine the lifetime of the pool textures.
  struct FirstUsageComparer
  {
    FirstUsageComparer(plDynamicArray<TextureUsageData>& ref_textureUsage)
      : m_TextureUsage(ref_textureUsage)
    {
    }

    PLASMA_ALWAYS_INLINE bool Less(plUInt16 a, plUInt16 b) const { return m_TextureUsage[a].m_uiFirstUsageIdx < m_TextureUsage[b].m_uiFirstUsageIdx; }

    plDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(plDynamicArray<TextureUsageData>& ref_textureUsage)
      : m_TextureUsage(ref_textureUsage)
    {
    }

    PLASMA_ALWAYS_INLINE bool Less(plUInt16 a, plUInt16 b) const { return m_TextureUsage[a].m_uiLastUsageIdx < m_TextureUsage[b].m_uiLastUsageIdx; }

    plDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  m_TextureUsageIdxSortedByFirstUsage.Sort(FirstUsageComparer(m_TextureUsage));
  m_TextureUsageIdxSortedByLastUsage.Sort(LastUsageComparer(m_TextureUsage));

  return true;
}

bool plRenderPipeline::InitRenderPipelinePasses()
{
  plLogBlock b("Init Render Pipeline Passes");
  // Init every pass now.
  for (auto& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];
    pPass->InitRenderPipelinePass(data.m_Inputs, data.m_Outputs);
  }

  return true;
}

void plRenderPipeline::SortExtractors()
{
  struct Helper
  {
    static bool FindDependency(const plHashedString& sDependency, plArrayPtr<plUniquePtr<plExtractor>> container)
    {
      for (auto& extractor : container)
      {
        if (sDependency == plTempHashedString(extractor->GetDynamicRTTI()->GetTypeNameHash()))
        {
          return true;
        }
      }

      return false;
    }
  };

  m_SortedExtractors.Clear();
  m_SortedExtractors.Reserve(m_Extractors.GetCount());

  plUInt32 uiIndex = 0;
  while (!m_Extractors.IsEmpty())
  {
    plUniquePtr<plExtractor>& extractor = m_Extractors[uiIndex];

    bool allDependenciesFound = true;
    for (auto& sDependency : extractor->m_DependsOn)
    {
      if (!Helper::FindDependency(sDependency, m_SortedExtractors))
      {
        allDependenciesFound = false;
        break;
      }
    }

    if (allDependenciesFound)
    {
      m_SortedExtractors.PushBack(std::move(extractor));
      m_Extractors.RemoveAtAndCopy(uiIndex);
    }
    else
    {
      ++uiIndex;
    }

    if (uiIndex >= m_Extractors.GetCount())
    {
      uiIndex = 0;
    }
  }

  m_Extractors.Swap(m_SortedExtractors);
}

void plRenderPipeline::UpdateViewData(const plView& view, plUInt32 uiDataIndex)
{
  if (!view.IsValid())
    return;

  if (uiDataIndex == plRenderWorld::GetDataIndexForExtraction() && m_CurrentExtractThread != (plThreadID)0)
    return;

  PLASMA_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1");
  auto& data = m_Data[uiDataIndex];

  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
}

void plRenderPipeline::AddExtractor(plUniquePtr<plExtractor>&& pExtractor)
{
  m_Extractors.PushBack(std::move(pExtractor));
}

void plRenderPipeline::RemoveExtractor(plExtractor* pExtractor)
{
  for (plUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
  {
    if (m_Extractors[i].Borrow() == pExtractor)
    {
      m_Extractors.RemoveAtAndCopy(i);
      break;
    }
  }
}

void plRenderPipeline::GetExtractors(plDynamicArray<const plExtractor*>& ref_extractors) const
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void plRenderPipeline::GetExtractors(plDynamicArray<plExtractor*>& ref_extractors)
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}


plExtractor* plRenderPipeline::GetExtractorByName(const plStringView& sExtractorName)
{
  for (auto& pExtractor : m_Extractors)
  {
    if (sExtractorName.IsEqual(pExtractor->GetName()))
    {
      return pExtractor.Borrow();
    }
  }

  return nullptr;
}

void plRenderPipeline::RemoveConnections(plRenderPipelinePass* pPass)
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return;

  ConnectionData& data = it.Value();
  for (plUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    plRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      plRenderPipelinePass* pSource = static_cast<plRenderPipelinePass*>(pConn->m_pOutput->m_pParent);
      bool bRes = Disconnect(pSource, pSource->GetPinName(pConn->m_pOutput), pPass, pPass->GetPinName(pPass->GetInputPins()[i]));
      PLASMA_IGNORE_UNUSED(bRes);
      PLASMA_ASSERT_DEBUG(bRes, "plRenderPipeline::RemoveConnections should not fail to disconnect pins!");
    }
  }
  for (plUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
  {
    plRenderPipelinePassConnection* pConn = data.m_Outputs[i];
    while (pConn != nullptr)
    {
      plRenderPipelinePass* pTarget = static_cast<plRenderPipelinePass*>(pConn->m_Inputs[0]->m_pParent);
      bool bRes = Disconnect(pPass, pPass->GetPinName(pConn->m_pOutput), pTarget, pTarget->GetPinName(pConn->m_Inputs[0]));
      PLASMA_IGNORE_UNUSED(bRes);
      PLASMA_ASSERT_DEBUG(bRes, "plRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConn = data.m_Outputs[i];
    }
  }
}

void plRenderPipeline::ClearRenderPassGraphTextures()
{
  m_TextureUsage.Clear();
  m_TextureUsageIdxSortedByFirstUsage.Clear();
  m_TextureUsageIdxSortedByLastUsage.Clear();

  // plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  for (auto it = m_Connections.GetIterator(); it.IsValid(); ++it)
  {
    auto& conn = it.Value();
    for (auto pConn : conn.m_Outputs)
    {
      if (pConn)
      {
        pConn->m_Desc = plGALTextureCreationDescription();
        if (!pConn->m_TextureHandle.IsInvalidated())
        {
          pConn->m_TextureHandle.Invalidate();
        }
      }
    }
  }
}

bool plRenderPipeline::AreInputDescriptionsAvailable(const plRenderPipelinePass* pPass, const plHybridArray<plRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  for (plUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    const plRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      // If the connections source is not done yet, the connections output is undefined yet and the inputs can't be processed yet.
      if (!done.Contains(static_cast<plRenderPipelinePass*>(pConn->m_pOutput->m_pParent)))
      {
        return false;
      }
    }
  }

  return true;
}

bool plRenderPipeline::ArePassThroughInputsDone(const plRenderPipelinePass* pPass, const plHybridArray<plRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  auto inputs = pPass->GetInputPins();
  for (plUInt32 i = 0; i < inputs.GetCount(); i++)
  {
    const plRenderPipelineNodePin* pPin = inputs[i];
    if (pPin->m_Type == plRenderPipelineNodePin::Type::PassThrough)
    {
      const plRenderPipelinePassConnection* pConn = data.m_Inputs[pPin->m_uiInputIndex];
      if (pConn != nullptr)
      {
        for (const plRenderPipelineNodePin* pInputPin : pConn->m_Inputs)
        {
          // Any input that is also connected to the source of pPin must be done before we can use the pass through input
          if (pInputPin != pPin && !done.Contains(static_cast<plRenderPipelinePass*>(pInputPin->m_pParent)))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

plFrameDataProviderBase* plRenderPipeline::GetFrameDataProvider(const plRTTI* pRtti) const
{
  plUInt32 uiIndex = 0;
  if (m_TypeToDataProviderIndex.TryGetValue(pRtti, uiIndex))
  {
    return m_DataProviders[uiIndex].Borrow();
  }

  plUniquePtr<plFrameDataProviderBase> pNewDataProvider = pRtti->GetAllocator()->Allocate<plFrameDataProviderBase>();
  plFrameDataProviderBase* pResult = pNewDataProvider.Borrow();
  pResult->m_pOwnerPipeline = this;

  m_TypeToDataProviderIndex.Insert(pRtti, m_DataProviders.GetCount());
  m_DataProviders.PushBack(std::move(pNewDataProvider));

  return pResult;
}

void plRenderPipeline::ExtractData(const plView& view)
{
  PLASMA_ASSERT_DEV(m_CurrentExtractThread == (plThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = plThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == plRenderWorld::GetFrameCounter())
  {
    PLASMA_REPORT_FAILURE("View '{0}' is extracted multiple times", view.GetName());
    return;
  }

  m_uiLastExtractionFrame = plRenderWorld::GetFrameCounter();

  // Determine visible objects
  FindVisibleObjects(view);

  // Extract and sort data
  auto& data = m_Data[plRenderWorld::GetDataIndexForExtraction()];

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetLodCamera(*view.GetLodCamera());
  data.SetViewData(view.GetData());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      PLASMA_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  data.SortAndBatch();

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      PLASMA_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

  m_CurrentExtractThread = (plThreadID)0;
}

plUniquePtr<plRasterizerViewPool> g_pRasterizerViewPool;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, SwRasterizer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pRasterizerViewPool = PLASMA_DEFAULT_NEW(plRasterizerViewPool);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pRasterizerViewPool.Clear();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plRenderPipeline::FindVisibleObjects(const plView& view)
{
  PLASMA_PROFILE_SCOPE("Visibility Culling");

  plFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  PLASMA_LOCK(view.GetWorld()->GetReadMarker());

  const bool bIsMainView = (view.GetCameraUsageHint() == plCameraUsageHint::MainView || view.GetCameraUsageHint() == plCameraUsageHint::EditorView);
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  const bool bRecordStats = cvar_SpatialCullingShowStats && bIsMainView;
  plSpatialSystem::QueryStats stats;
#endif

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderStatic.GetBitmask() | plDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  queryParams.m_IncludeTags = view.m_IncludeTags;
  queryParams.m_ExcludeTags = view.m_ExcludeTags;
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  plFrustum limitedFrustum = frustum;
  const plPlane farPlane = limitedFrustum.GetPlane(plFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(plFrustum::PlaneType::FarPlane) = plPlane::MakeFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  plRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  PLASMA_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const plVisibilityState visType = bIsMainView ? plVisibilityState::Direct : plVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    PLASMA_PROFILE_SCOPE("Occlusion::FindVisibleObjects");

    auto IsOccluded = [=](const plSimdBBox& aabb) {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      const plSimdVec4f c = aabb.GetCenter();
      const plSimdVec4f e = aabb.GetHalfExtents();
      const plSimdBBox aabb2 = plSimdBBox::MakeFromCenterAndHalfExtents(c, e.CompMul(plSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

      return !pRasterizer->IsVisible(aabb2);
    };

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, IsOccluded, visType);
  }
  else
  {
    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, visType);
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  if (pRasterizer)
  {
    if (view.GetCameraUsageHint() == plCameraUsageHint::EditorView || view.GetCameraUsageHint() == plCameraUsageHint::MainView)
    {
      PreviewOcclusionBuffer(*pRasterizer, view);
    }
  }
#endif

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  plViewHandle hView = view.GetHandle();

  if (cvar_SpatialCullingVis && bIsMainView)
  {
    plDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, plColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    plStringBuilder sb;

    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", "Visibility Culling Stats", plColor::LimeGreen);

    sb.Format("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", sb, plColor::LimeGreen);

    sb.Format("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", sb, plColor::LimeGreen);

    sb.Format("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", sb, plColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = plMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.Format("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", sb, plColor::LimeGreen);

    view.GetWorld()->GetSpatialSystem()->GetInternalStats(sb);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "VisCulling", sb, plColor::AntiqueWhite);
  }
#endif
}

void plRenderPipeline::Render(plRenderContext* pRenderContext)
{
  // PLASMA_PROFILE_AND_MARKER(pRenderContext->GetGALContext(), m_sName.GetData());
  PLASMA_PROFILE_SCOPE(m_sName.GetData());

  PLASMA_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
  {
    return;
  }

  PLASMA_ASSERT_DEV(m_CurrentRenderThread == (plThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = plThreadUtils::GetCurrentThreadID();

  PLASMA_ASSERT_DEV(m_uiLastRenderFrame != plRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = plRenderWorld::GetFrameCounter();


  auto& data = m_Data[plRenderWorld::GetDataIndexForRendering()];
  const plCamera* pCamera = &data.GetCamera();
  const plCamera* pLodCamera = &data.GetLodCamera();
  const plViewData* pViewData = &data.GetViewData();

  auto& gc = pRenderContext->WriteGlobalConstants();
  for (int i = 0; i < 2; ++i)
  {
    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i] = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i] = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i] = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i] = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const plRectFloat& viewport = pViewData->m_ViewPortRect;
  gc.ViewportSize = plVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

  float fNear = pCamera->GetNearPlane();
  float fFar = pCamera->GetFarPlane();
  gc.ClipPlanes = plVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsDirectionalLightShadow = pViewData->m_CameraUsageHint == plCameraUsageHint::Shadow && pCamera->IsOrthographic();
  gc.MaxZValue = bIsDirectionalLightShadow ? 0.0f : plMath::MinValue<float>();

  // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
  gc.DeltaTime = (float)plClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)plMath::Mod(plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
  gc.WorldTime = (float)plMath::Mod(data.GetWorldTime().GetSeconds(), 20790.0);

  gc.Exposure = pCamera->GetExposure();
  gc.RenderPass = plViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);

  plRenderViewContext renderViewContext;
  renderViewContext.m_pCamera = pCamera;
  renderViewContext.m_pLodCamera = pLodCamera;
  renderViewContext.m_pViewData = pViewData;
  renderViewContext.m_pRenderContext = pRenderContext;
  renderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  renderViewContext.m_pViewDebugContext = &data.GetViewDebugContext();

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static plHashedString sCameraMode = plMakeHashedString("CAMERA_MODE");
  static plHashedString sOrtho = plMakeHashedString("CAMERA_MODE_ORTHO");
  static plHashedString sPerspective = plMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static plHashedString sStereo = plMakeHashedString("CAMERA_MODE_STEREO");

  static plHashedString sVSRTAI = plMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static plHashedString sClipSpaceFlipped = plMakeHashedString("CLIP_SPACE_FLIPPED");
  static plHashedString sTrue = plMakeHashedString("TRUE");
  static plHashedString sFalse = plMakeHashedString("FALSE");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  if (plGALDevice::GetDefaultDevice()->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    pRenderContext->SetShaderPermutationVariable(sVSRTAI, sTrue);
  else
    pRenderContext->SetShaderPermutationVariable(sVSRTAI, sFalse);

  pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, plClipSpaceYMode::RenderToTextureDefault == plClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation vars
  for (auto& var : m_PermutationVars)
  {
    pRenderContext->SetShaderPermutationVariable(var.m_sName, var.m_sValue);
  }

  plRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = plRenderWorldRenderEvent::Type::BeforePipelineExecution;
  renderEvent.m_pPipeline = this;
  renderEvent.m_pRenderViewContext = &renderViewContext;
  renderEvent.m_uiFrameCounter = plRenderWorld::GetFrameCounter();
  {
    PLASMA_PROFILE_SCOPE("BeforePipelineExecution");
    plRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  pDevice->BeginPipeline(m_sName, renderViewContext.m_pViewData->m_hSwapChain);

  if (const plGALSwapChain* pSwapChain = pDevice->GetSwapChain(renderViewContext.m_pViewData->m_hSwapChain))
  {
    const plGALRenderTargets& renderTargets = pSwapChain->GetRenderTargets();
    // Update target textures after the swap chain acquired new textures.
    for (plUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
    {
      TextureUsageData& textureUsageData = m_TextureUsage[i];
      if (textureUsageData.m_iTargetTextureIndex != -1)
      {
        plGALTextureHandle hTexture = reinterpret_cast<const plGALTextureHandle*>(&renderTargets)[textureUsageData.m_iTargetTextureIndex];
        for (auto pUsedByConn : textureUsageData.m_UsedBy)
        {
          pUsedByConn->m_TextureHandle = hTexture;
        }
      }
    }
  }

  plUInt32 uiCurrentFirstUsageIdx = 0;
  plUInt32 uiCurrentLastUsageIdx = 0;
  for (plUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    auto& pPass = m_Passes[i];
    PLASMA_PROFILE_SCOPE(pPass->GetName());
    plLogBlock passBlock("Render Pass", pPass->GetName());

    // Create pool textures
    for (; uiCurrentFirstUsageIdx < m_TextureUsageIdxSortedByFirstUsage.GetCount();)
    {
      plUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByFirstUsage[uiCurrentFirstUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiFirstUsageIdx == i)
      {
        plGALTextureHandle hTexture = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(usageData.m_UsedBy[0]->m_Desc);
        PLASMA_ASSERT_DEV(!hTexture.IsInvalidated(), "GPU pool returned an invalidated texture!");
        for (plRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle = hTexture;
        }
        ++uiCurrentFirstUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiFirstUsageIdx isn't reached yet so wait.
        break;
      }
    }

    // Execute pass block
    {
      ConnectionData& connectionData = m_Connections[pPass.Borrow()];
      if (pPass->m_bActive)
      {
        pPass->Execute(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
      }
      else
      {
        pPass->ExecuteInactive(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
      }
    }

    // Release pool textures
    for (; uiCurrentLastUsageIdx < m_TextureUsageIdxSortedByLastUsage.GetCount();)
    {
      plUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByLastUsage[uiCurrentLastUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiLastUsageIdx == i)
      {
        plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(usageData.m_UsedBy[0]->m_TextureHandle);
        for (plRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle.Invalidate();
        }
        ++uiCurrentLastUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiLastUsageIdx isn't reached yet so wait.
        break;
      }
    }
  }
  PLASMA_ASSERT_DEV(uiCurrentFirstUsageIdx == m_TextureUsageIdxSortedByFirstUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");
  PLASMA_ASSERT_DEV(uiCurrentLastUsageIdx == m_TextureUsageIdxSortedByLastUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");

  pDevice->EndPipeline(renderViewContext.m_pViewData->m_hSwapChain);

  renderEvent.m_Type = plRenderWorldRenderEvent::Type::AfterPipelineExecution;
  {
    PLASMA_PROFILE_SCOPE("AfterPipelineExecution");
    plRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  pRenderContext->ResetContextState();

  data.Clear();

  m_CurrentRenderThread = (plThreadID)0;
}

const plExtractedRenderData& plRenderPipeline::GetRenderData() const
{
  return m_Data[plRenderWorld::GetDataIndexForRendering()];
}

plRenderDataBatchList plRenderPipeline::GetRenderDataBatchesWithCategory(plRenderData::Category category, plRenderDataBatch::Filter filter) const
{
  auto& data = m_Data[plRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category, filter);
}

void plRenderPipeline::CreateDgmlGraph(plDGMLGraph& ref_graph)
{
  plStringBuilder sTmp;
  plHashTable<const plRenderPipelineNode*, plUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_TextureUsage.GetCount() * 3);
  for (plUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTmp.Format("#{}: {}", p, plStringUtils::IsNullOrEmpty(pPass->GetName()) ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    plDGMLGraph::NodeDesc nd;
    nd.m_Color = plColor::Gray;
    nd.m_Shape = plDGMLGraph::NodeShape::Rectangle;
    plUInt32 uiGraphNode = ref_graph.AddNode(sTmp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (plUInt32 i = 0; i < m_TextureUsage.GetCount(); ++i)
  {
    const TextureUsageData& data = m_TextureUsage[i];

    for (const plRenderPipelinePassConnection* pCon : data.m_UsedBy)
    {
      plDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_iTargetTextureIndex != -1 ? plColor::Black : plColorScheme::GetColor(static_cast<plColorScheme::Enum>(i % plColorScheme::Count), 4);
      nd.m_Shape = plDGMLGraph::NodeShape::RoundedRectangle;

      plStringBuilder sFormat;
      if (!plReflectionUtils::EnumerationToString(plGetStaticRTTI<plGALResourceFormat>(), pCon->m_Desc.m_Format, sFormat, plReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sFormat.Format("Unknown Format {}", (int)pCon->m_Desc.m_Format);
      }
      sTmp.Format("{} #{}: {}x{}:{}, MSAA:{}, {}Format: {}", data.m_iTargetTextureIndex != -1 ? "RenderTarget" : "PoolTexture", i, pCon->m_Desc.m_uiWidth, pCon->m_Desc.m_uiHeight, pCon->m_Desc.m_uiArraySize, (int)pCon->m_Desc.m_SampleCount, plGALResourceFormat::IsDepthFormat(pCon->m_Desc.m_Format) ? "Depth" : "Color", sFormat);
      plUInt32 uiTextureNode = ref_graph.AddNode(sTmp, &nd);

      plUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const plRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        plUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
}

plRasterizerView* plRenderPipeline::PrepareOcclusionCulling(const plFrustum& frustum, const plView& view)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  if (!plSystemInformation::Get().GetCpuFeatures().IsAvx1Available())
    return nullptr;

  plRasterizerView* pRasterizer = nullptr;

  // extract all occlusion geometry from the scene
  PLASMA_PROFILE_SCOPE("Occlusion::RasterizeView");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<plUInt32>(view.GetViewport().width / 2), static_cast<plUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    PLASMA_PROFILE_SCOPE("Occlusion::FindOccluders");

    plSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | plDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_IncludeTags = view.m_IncludeTags;
    queryParams.m_ExcludeTags = view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, plVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  for (const plGameObject* pObj : m_VisibleObjects)
  {
    plMsgExtractOccluderData msg;
    pObj->SendMessage(msg);

    for (const auto& ed : msg.m_ExtractedOccluderData)
    {
      pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
    }
  }

  pRasterizer->EndScene();

  return pRasterizer;
#else
  return nullptr;
#endif
}

void plRenderPipeline::PreviewOcclusionBuffer(const plRasterizerView& rasterizer, const plView& view)
{
  if (!cvar_SpatialCullingOcclusionVisView || !rasterizer.HasRasterizedAnyOccluders())
    return;

  PLASMA_PROFILE_SCOPE("Occlusion::DebugPreview");

  const plUInt32 uiImgWidth = rasterizer.GetResolutionX();
  const plUInt32 uiImgHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  plDynamicArray<plColorLinearUB> fb;
  fb.SetCountUninitialized(uiImgWidth * uiImgHeight);
  rasterizer.ReadBackFrame(fb);

  const float w = (float)uiImgWidth;
  const float h = (float)uiImgHeight;
  plRectFloat rectInPixel1 = plRectFloat(5.0f, 5.0f, w + 10, h + 10);
  plRectFloat rectInPixel2 = plRectFloat(10.0f, 10.0f, w, h);

  plDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, plColor::MediumPurple);

  // TODO: it would be better to update a single texture every frame, however since this is a render pass,
  // we currently can't create nested passes
  // so either this has to be done elsewhere, or nested passes have to be allowed
  if (false)
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    // check whether we need to re-create the texture
    if (!m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      const plGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

      if (pTexture->GetDescription().m_uiWidth != uiImgWidth ||
          pTexture->GetDescription().m_uiHeight != uiImgHeight)
      {
        pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
        m_hOcclusionDebugViewTexture.Invalidate();
      }
    }

    // create the texture
    if (m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      plGALTextureCreationDescription desc;
      desc.m_uiWidth = uiImgWidth;
      desc.m_uiHeight = uiImgHeight;
      desc.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hOcclusionDebugViewTexture = pDevice->CreateTexture(desc);
    }

    // upload the image to the texture
    {
      plGALPass* pGALPass = pDevice->BeginPass("RasterizerDebugViewUpdate");
      auto pCommandEncoder = pGALPass->BeginCompute();

      plBoundingBoxu32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = plVec3U32(uiImgWidth, uiImgHeight, 1);

      plGALSystemMemoryDescription sourceData;
      sourceData.m_pData = fb.GetData();
      sourceData.m_uiRowPitch = uiImgWidth * sizeof(plColorLinearUB);

      pCommandEncoder->UpdateTexture(m_hOcclusionDebugViewTexture, plGALTextureSubresource(), destBox, sourceData);

      pGALPass->EndCompute(pCommandEncoder);
      pDevice->EndPass(pGALPass);
    }

    plDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, plColor::White, pDevice->GetDefaultResourceView(m_hOcclusionDebugViewTexture), plVec2(1, -1));
  }
  else
  {
    plTexture2DResourceDescriptor d;
    d.m_DescGAL.m_uiWidth = rasterizer.GetResolutionX();
    d.m_DescGAL.m_uiHeight = rasterizer.GetResolutionY();
    d.m_DescGAL.m_Format = plGALResourceFormat::RGBAByteNormalized;

    plGALSystemMemoryDescription content[1];
    content[0].m_pData = fb.GetData();
    content[0].m_uiRowPitch = sizeof(plColorLinearUB) * d.m_DescGAL.m_uiWidth;
    content[0].m_uiSlicePitch = content[0].m_uiRowPitch * d.m_DescGAL.m_uiHeight;
    d.m_InitialContent = content;

    static plAtomicInteger32 name = 0;
    name.Increment();

    plStringBuilder sName;
    sName.Format("RasterizerPreview-{}", name);

    plTexture2DResourceHandle hDebug = plResourceManager::CreateResource<plTexture2DResource>(sName, std::move(d));

    plDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, plColor::White, hDebug, plVec2(1, -1));
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);
