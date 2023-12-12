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

plCVarFloat cvar_RendererGamma("Renderer.Gamma", 2.4f, plCVarFlags::Save, "The value of the gamma parameter used in shaders.");

plCVarFloat cvar_RendererDiffuseFromProbes("Renderer.DiffuseFromProbes", true, plCVarFlags::Save, "If enabled, indirect diffuse will come from reflection probes instead of skylight.");

plRenderPipeline::plRenderPipeline()
  : m_PipelineState(PipelineState::Uninitialized)
{
  m_CurrentExtractThread = (plThreadID)0;
  m_CurrentRenderThread = (plThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = plTime::Seconds(0.1f);
#endif
}

plRenderPipeline::~plRenderPipeline()
{
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    const plGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

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

void plRenderPipeline::GetPasses(plHybridArray<const plRenderPipelinePass*, 16>& passes) const
{
  passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    passes.PushBack(pPass.Borrow());
  }
}

void plRenderPipeline::GetPasses(plHybridArray<plRenderPipelinePass*, 16>& passes)
{
  passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    passes.PushBack(pPass.Borrow());
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

const plRenderPipelinePassConnection* plRenderPipeline::GetInputConnection(plRenderPipelinePass* pPass, plHashedString sInputPinName) const
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

const plRenderPipelinePassConnection* plRenderPipeline::GetOutputConnection(plRenderPipelinePass* pPass, plHashedString sOutputPinName) const
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

          if (!hTexture || !hTexture->IsInvalidated() || pConn->m_Desc.CalculateHash() == defaultTextureDescHash)
          {
            plUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
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
    FirstUsageComparer(plDynamicArray<TextureUsageData>& textureUsage)
      : m_TextureUsage(textureUsage)
    {
    }

    PLASMA_ALWAYS_INLINE bool Less(plUInt16 a, plUInt16 b) const { return m_TextureUsage[a].m_uiFirstUsageIdx < m_TextureUsage[b].m_uiFirstUsageIdx; }

    plDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(plDynamicArray<TextureUsageData>& textureUsage)
      : m_TextureUsage(textureUsage)
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
    auto& extractor = m_Extractors[uiIndex];

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

void plRenderPipeline::GetExtractors(plHybridArray<const plExtractor*, 16>& extractors) const
{
  extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    extractors.PushBack(pExtractor.Borrow());
  }
}

void plRenderPipeline::GetExtractors(plHybridArray<plExtractor*, 16>& extractors)
{
  extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    extractors.PushBack(pExtractor.Borrow());
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

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == plCameraUsageHint::MainView || view.GetCameraUsageHint() == plCameraUsageHint::EditorView);
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
  limitedFrustum.AccessPlane(plFrustum::PlaneType::FarPlane).SetFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  plRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  PLASMA_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const plVisibilityState visType = bIsMainView ? plVisibilityState::Direct : plVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    PLASMA_PROFILE_SCOPE("Occlusion::FindVisibleObjects");

    auto IsOccluded = [=](const plSimdBBox& aabb) {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      plSimdBBox aabb2;
      const plSimdVec4f c = aabb.GetCenter();
      const plSimdVec4f e = aabb.GetHalfExtents();
      aabb2.SetCenterAndHalfExtents(c, e.CompMul(plSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

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

const plVec4& GetHaltonSequence(int idx)
{
  static const plVec4 HALTON[] = {
    plVec4(0.5000000000f, 0.3333333333f, 0.2000000000f, 0.1428571429f),
    plVec4(0.2500000000f, 0.6666666667f, 0.4000000000f, 0.2857142857f),
    plVec4(0.7500000000f, 0.1111111111f, 0.6000000000f, 0.4285714286f),
    plVec4(0.1250000000f, 0.4444444444f, 0.8000000000f, 0.5714285714f),
    plVec4(0.6250000000f, 0.7777777778f, 0.0400000000f, 0.7142857143f),
    plVec4(0.3750000000f, 0.2222222222f, 0.2400000000f, 0.8571428571f),
    plVec4(0.8750000000f, 0.5555555556f, 0.4400000000f, 0.0204081633f),
    plVec4(0.0625000000f, 0.8888888889f, 0.6400000000f, 0.1632653061f),
    plVec4(0.5625000000f, 0.0370370370f, 0.8400000000f, 0.3061224490f),
    plVec4(0.3125000000f, 0.3703703704f, 0.0800000000f, 0.4489795918f),
    plVec4(0.8125000000f, 0.7037037037f, 0.2800000000f, 0.5918367347f),
    plVec4(0.1875000000f, 0.1481481481f, 0.4800000000f, 0.7346938776f),
    plVec4(0.6875000000f, 0.4814814815f, 0.6800000000f, 0.8775510204f),
    plVec4(0.4375000000f, 0.8148148148f, 0.8800000000f, 0.0408163265f),
    plVec4(0.9375000000f, 0.2592592593f, 0.1200000000f, 0.1836734694f),
    plVec4(0.0312500000f, 0.5925925926f, 0.3200000000f, 0.3265306122f),
    plVec4(0.5312500000f, 0.9259259259f, 0.5200000000f, 0.4693877551f),
    plVec4(0.2812500000f, 0.0740740741f, 0.7200000000f, 0.6122448980f),
    plVec4(0.7812500000f, 0.4074074074f, 0.9200000000f, 0.7551020408f),
    plVec4(0.1562500000f, 0.7407407407f, 0.1600000000f, 0.8979591837f),
    plVec4(0.6562500000f, 0.1851851852f, 0.3600000000f, 0.0612244898f),
    plVec4(0.4062500000f, 0.5185185185f, 0.5600000000f, 0.2040816327f),
    plVec4(0.9062500000f, 0.8518518519f, 0.7600000000f, 0.3469387755f),
    plVec4(0.0937500000f, 0.2962962963f, 0.9600000000f, 0.4897959184f),
    plVec4(0.5937500000f, 0.6296296296f, 0.0080000000f, 0.6326530612f),
    plVec4(0.3437500000f, 0.9629629630f, 0.2080000000f, 0.7755102041f),
    plVec4(0.8437500000f, 0.0123456790f, 0.4080000000f, 0.9183673469f),
    plVec4(0.2187500000f, 0.3456790123f, 0.6080000000f, 0.0816326531f),
    plVec4(0.7187500000f, 0.6790123457f, 0.8080000000f, 0.2244897959f),
    plVec4(0.4687500000f, 0.1234567901f, 0.0480000000f, 0.3673469388f),
    plVec4(0.9687500000f, 0.4567901235f, 0.2480000000f, 0.5102040816f),
    plVec4(0.0156250000f, 0.7901234568f, 0.4480000000f, 0.6530612245f),
    plVec4(0.5156250000f, 0.2345679012f, 0.6480000000f, 0.7959183673f),
    plVec4(0.2656250000f, 0.5679012346f, 0.8480000000f, 0.9387755102f),
    plVec4(0.7656250000f, 0.9012345679f, 0.0880000000f, 0.1020408163f),
    plVec4(0.1406250000f, 0.0493827160f, 0.2880000000f, 0.2448979592f),
    plVec4(0.6406250000f, 0.3827160494f, 0.4880000000f, 0.3877551020f),
    plVec4(0.3906250000f, 0.7160493827f, 0.6880000000f, 0.5306122449f),
    plVec4(0.8906250000f, 0.1604938272f, 0.8880000000f, 0.6734693878f),
    plVec4(0.0781250000f, 0.4938271605f, 0.1280000000f, 0.8163265306f),
    plVec4(0.5781250000f, 0.8271604938f, 0.3280000000f, 0.9591836735f),
    plVec4(0.3281250000f, 0.2716049383f, 0.5280000000f, 0.1224489796f),
    plVec4(0.8281250000f, 0.6049382716f, 0.7280000000f, 0.2653061224f),
    plVec4(0.2031250000f, 0.9382716049f, 0.9280000000f, 0.4081632653f),
    plVec4(0.7031250000f, 0.0864197531f, 0.1680000000f, 0.5510204082f),
    plVec4(0.4531250000f, 0.4197530864f, 0.3680000000f, 0.6938775510f),
    plVec4(0.9531250000f, 0.7530864198f, 0.5680000000f, 0.8367346939f),
    plVec4(0.0468750000f, 0.1975308642f, 0.7680000000f, 0.9795918367f),
    plVec4(0.5468750000f, 0.5308641975f, 0.9680000000f, 0.0029154519f),
    plVec4(0.2968750000f, 0.8641975309f, 0.0160000000f, 0.1457725948f),
    plVec4(0.7968750000f, 0.3086419753f, 0.2160000000f, 0.2886297376f),
    plVec4(0.1718750000f, 0.6419753086f, 0.4160000000f, 0.4314868805f),
    plVec4(0.6718750000f, 0.9753086420f, 0.6160000000f, 0.5743440233f),
    plVec4(0.4218750000f, 0.0246913580f, 0.8160000000f, 0.7172011662f),
    plVec4(0.9218750000f, 0.3580246914f, 0.0560000000f, 0.8600583090f),
    plVec4(0.1093750000f, 0.6913580247f, 0.2560000000f, 0.0233236152f),
    plVec4(0.6093750000f, 0.1358024691f, 0.4560000000f, 0.1661807580f),
    plVec4(0.3593750000f, 0.4691358025f, 0.6560000000f, 0.3090379009f),
    plVec4(0.8593750000f, 0.8024691358f, 0.8560000000f, 0.4518950437f),
    plVec4(0.2343750000f, 0.2469135802f, 0.0960000000f, 0.5947521866f),
    plVec4(0.7343750000f, 0.5802469136f, 0.2960000000f, 0.7376093294f),
    plVec4(0.4843750000f, 0.9135802469f, 0.4960000000f, 0.8804664723f),
    plVec4(0.9843750000f, 0.0617283951f, 0.6960000000f, 0.0437317784f),
    plVec4(0.0078125000f, 0.3950617284f, 0.8960000000f, 0.1865889213f),
    plVec4(0.5078125000f, 0.7283950617f, 0.1360000000f, 0.3294460641f),
    plVec4(0.2578125000f, 0.1728395062f, 0.3360000000f, 0.4723032070f),
    plVec4(0.7578125000f, 0.5061728395f, 0.5360000000f, 0.6151603499f),
    plVec4(0.1328125000f, 0.8395061728f, 0.7360000000f, 0.7580174927f),
    plVec4(0.6328125000f, 0.2839506173f, 0.9360000000f, 0.9008746356f),
    plVec4(0.3828125000f, 0.6172839506f, 0.1760000000f, 0.0641399417f),
    plVec4(0.8828125000f, 0.9506172840f, 0.3760000000f, 0.2069970845f),
    plVec4(0.0703125000f, 0.0987654321f, 0.5760000000f, 0.3498542274f),
    plVec4(0.5703125000f, 0.4320987654f, 0.7760000000f, 0.4927113703f),
    plVec4(0.3203125000f, 0.7654320988f, 0.9760000000f, 0.6355685131f),
    plVec4(0.8203125000f, 0.2098765432f, 0.0240000000f, 0.7784256560f),
    plVec4(0.1953125000f, 0.5432098765f, 0.2240000000f, 0.9212827988f),
    plVec4(0.6953125000f, 0.8765432099f, 0.4240000000f, 0.0845481050f),
    plVec4(0.4453125000f, 0.3209876543f, 0.6240000000f, 0.2274052478f),
    plVec4(0.9453125000f, 0.6543209877f, 0.8240000000f, 0.3702623907f),
    plVec4(0.0390625000f, 0.9876543210f, 0.0640000000f, 0.5131195335f),
    plVec4(0.5390625000f, 0.0041152263f, 0.2640000000f, 0.6559766764f),
    plVec4(0.2890625000f, 0.3374485597f, 0.4640000000f, 0.7988338192f),
    plVec4(0.7890625000f, 0.6707818930f, 0.6640000000f, 0.9416909621f),
    plVec4(0.1640625000f, 0.1152263374f, 0.8640000000f, 0.1049562682f),
    plVec4(0.6640625000f, 0.4485596708f, 0.1040000000f, 0.2478134111f),
    plVec4(0.4140625000f, 0.7818930041f, 0.3040000000f, 0.3906705539f),
    plVec4(0.9140625000f, 0.2263374486f, 0.5040000000f, 0.5335276968f),
    plVec4(0.1015625000f, 0.5596707819f, 0.7040000000f, 0.6763848397f),
    plVec4(0.6015625000f, 0.8930041152f, 0.9040000000f, 0.8192419825f),
    plVec4(0.3515625000f, 0.0411522634f, 0.1440000000f, 0.9620991254f),
    plVec4(0.8515625000f, 0.3744855967f, 0.3440000000f, 0.1253644315f),
    plVec4(0.2265625000f, 0.7078189300f, 0.5440000000f, 0.2682215743f),
    plVec4(0.7265625000f, 0.1522633745f, 0.7440000000f, 0.4110787172f),
    plVec4(0.4765625000f, 0.4855967078f, 0.9440000000f, 0.5539358601f),
    plVec4(0.9765625000f, 0.8189300412f, 0.1840000000f, 0.6967930029f),
    plVec4(0.0234375000f, 0.2633744856f, 0.3840000000f, 0.8396501458f),
    plVec4(0.5234375000f, 0.5967078189f, 0.5840000000f, 0.9825072886f),
    plVec4(0.2734375000f, 0.9300411523f, 0.7840000000f, 0.0058309038f),
    plVec4(0.7734375000f, 0.0781893004f, 0.9840000000f, 0.1486880466f),
    plVec4(0.1484375000f, 0.4115226337f, 0.0320000000f, 0.2915451895f),
    plVec4(0.6484375000f, 0.7448559671f, 0.2320000000f, 0.4344023324f),
    plVec4(0.3984375000f, 0.1893004115f, 0.4320000000f, 0.5772594752f),
    plVec4(0.8984375000f, 0.5226337449f, 0.6320000000f, 0.7201166181f),
    plVec4(0.0859375000f, 0.8559670782f, 0.8320000000f, 0.8629737609f),
    plVec4(0.5859375000f, 0.3004115226f, 0.0720000000f, 0.0262390671f),
    plVec4(0.3359375000f, 0.6337448560f, 0.2720000000f, 0.1690962099f),
    plVec4(0.8359375000f, 0.9670781893f, 0.4720000000f, 0.3119533528f),
    plVec4(0.2109375000f, 0.0164609053f, 0.6720000000f, 0.4548104956f),
    plVec4(0.7109375000f, 0.3497942387f, 0.8720000000f, 0.5976676385f),
    plVec4(0.4609375000f, 0.6831275720f, 0.1120000000f, 0.7405247813f),
    plVec4(0.9609375000f, 0.1275720165f, 0.3120000000f, 0.8833819242f),
    plVec4(0.0546875000f, 0.4609053498f, 0.5120000000f, 0.0466472303f),
    plVec4(0.5546875000f, 0.7942386831f, 0.7120000000f, 0.1895043732f),
    plVec4(0.3046875000f, 0.2386831276f, 0.9120000000f, 0.3323615160f),
    plVec4(0.8046875000f, 0.5720164609f, 0.1520000000f, 0.4752186589f),
    plVec4(0.1796875000f, 0.9053497942f, 0.3520000000f, 0.6180758017f),
    plVec4(0.6796875000f, 0.0534979424f, 0.5520000000f, 0.7609329446f),
    plVec4(0.4296875000f, 0.3868312757f, 0.7520000000f, 0.9037900875f),
    plVec4(0.9296875000f, 0.7201646091f, 0.9520000000f, 0.0670553936f),
    plVec4(0.1171875000f, 0.1646090535f, 0.1920000000f, 0.2099125364f),
    plVec4(0.6171875000f, 0.4979423868f, 0.3920000000f, 0.3527696793f),
    plVec4(0.3671875000f, 0.8312757202f, 0.5920000000f, 0.4956268222f),
    plVec4(0.8671875000f, 0.2757201646f, 0.7920000000f, 0.6384839650f),
    plVec4(0.2421875000f, 0.6090534979f, 0.9920000000f, 0.7813411079f),
    plVec4(0.7421875000f, 0.9423868313f, 0.0016000000f, 0.9241982507f),
    plVec4(0.4921875000f, 0.0905349794f, 0.2016000000f, 0.0874635569f),
    plVec4(0.9921875000f, 0.4238683128f, 0.4016000000f, 0.2303206997f),
    plVec4(0.0039062500f, 0.7572016461f, 0.6016000000f, 0.3731778426f),
    plVec4(0.5039062500f, 0.2016460905f, 0.8016000000f, 0.5160349854f),
    plVec4(0.2539062500f, 0.5349794239f, 0.0416000000f, 0.6588921283f),
    plVec4(0.7539062500f, 0.8683127572f, 0.2416000000f, 0.8017492711f),
    plVec4(0.1289062500f, 0.3127572016f, 0.4416000000f, 0.9446064140f),
    plVec4(0.6289062500f, 0.6460905350f, 0.6416000000f, 0.1078717201f),
    plVec4(0.3789062500f, 0.9794238683f, 0.8416000000f, 0.2507288630f),
    plVec4(0.8789062500f, 0.0288065844f, 0.0816000000f, 0.3935860058f),
    plVec4(0.0664062500f, 0.3621399177f, 0.2816000000f, 0.5364431487f),
    plVec4(0.5664062500f, 0.6954732510f, 0.4816000000f, 0.6793002915f),
    plVec4(0.3164062500f, 0.1399176955f, 0.6816000000f, 0.8221574344f),
    plVec4(0.8164062500f, 0.4732510288f, 0.8816000000f, 0.9650145773f),
    plVec4(0.1914062500f, 0.8065843621f, 0.1216000000f, 0.1282798834f),
    plVec4(0.6914062500f, 0.2510288066f, 0.3216000000f, 0.2711370262f),
    plVec4(0.4414062500f, 0.5843621399f, 0.5216000000f, 0.4139941691f),
    plVec4(0.9414062500f, 0.9176954733f, 0.7216000000f, 0.5568513120f),
    plVec4(0.0351562500f, 0.0658436214f, 0.9216000000f, 0.6997084548f),
    plVec4(0.5351562500f, 0.3991769547f, 0.1616000000f, 0.8425655977f),
    plVec4(0.2851562500f, 0.7325102881f, 0.3616000000f, 0.9854227405f),
    plVec4(0.7851562500f, 0.1769547325f, 0.5616000000f, 0.0087463557f),
    plVec4(0.1601562500f, 0.5102880658f, 0.7616000000f, 0.1516034985f),
    plVec4(0.6601562500f, 0.8436213992f, 0.9616000000f, 0.2944606414f),
    plVec4(0.4101562500f, 0.2880658436f, 0.0096000000f, 0.4373177843f),
    plVec4(0.9101562500f, 0.6213991770f, 0.2096000000f, 0.5801749271f),
    plVec4(0.0976562500f, 0.9547325103f, 0.4096000000f, 0.7230320700f),
    plVec4(0.5976562500f, 0.1028806584f, 0.6096000000f, 0.8658892128f),
    plVec4(0.3476562500f, 0.4362139918f, 0.8096000000f, 0.0291545190f),
    plVec4(0.8476562500f, 0.7695473251f, 0.0496000000f, 0.1720116618f),
    plVec4(0.2226562500f, 0.2139917695f, 0.2496000000f, 0.3148688047f),
    plVec4(0.7226562500f, 0.5473251029f, 0.4496000000f, 0.4577259475f),
    plVec4(0.4726562500f, 0.8806584362f, 0.6496000000f, 0.6005830904f),
    plVec4(0.9726562500f, 0.3251028807f, 0.8496000000f, 0.7434402332f),
    plVec4(0.0195312500f, 0.6584362140f, 0.0896000000f, 0.8862973761f),
    plVec4(0.5195312500f, 0.9917695473f, 0.2896000000f, 0.0495626822f),
    plVec4(0.2695312500f, 0.0082304527f, 0.4896000000f, 0.1924198251f),
    plVec4(0.7695312500f, 0.3415637860f, 0.6896000000f, 0.3352769679f),
    plVec4(0.1445312500f, 0.6748971193f, 0.8896000000f, 0.4781341108f),
    plVec4(0.6445312500f, 0.1193415638f, 0.1296000000f, 0.6209912536f),
    plVec4(0.3945312500f, 0.4526748971f, 0.3296000000f, 0.7638483965f),
    plVec4(0.8945312500f, 0.7860082305f, 0.5296000000f, 0.9067055394f),
    plVec4(0.0820312500f, 0.2304526749f, 0.7296000000f, 0.0699708455f),
    plVec4(0.5820312500f, 0.5637860082f, 0.9296000000f, 0.2128279883f),
    plVec4(0.3320312500f, 0.8971193416f, 0.1696000000f, 0.3556851312f),
    plVec4(0.8320312500f, 0.0452674897f, 0.3696000000f, 0.4985422741f),
    plVec4(0.2070312500f, 0.3786008230f, 0.5696000000f, 0.6413994169f),
    plVec4(0.7070312500f, 0.7119341564f, 0.7696000000f, 0.7842565598f),
    plVec4(0.4570312500f, 0.1563786008f, 0.9696000000f, 0.9271137026f),
    plVec4(0.9570312500f, 0.4897119342f, 0.0176000000f, 0.0903790087f),
    plVec4(0.0507812500f, 0.8230452675f, 0.2176000000f, 0.2332361516f),
    plVec4(0.5507812500f, 0.2674897119f, 0.4176000000f, 0.3760932945f),
    plVec4(0.3007812500f, 0.6008230453f, 0.6176000000f, 0.5189504373f),
    plVec4(0.8007812500f, 0.9341563786f, 0.8176000000f, 0.6618075802f),
    plVec4(0.1757812500f, 0.0823045267f, 0.0576000000f, 0.8046647230f),
    plVec4(0.6757812500f, 0.4156378601f, 0.2576000000f, 0.9475218659f),
    plVec4(0.4257812500f, 0.7489711934f, 0.4576000000f, 0.1107871720f),
    plVec4(0.9257812500f, 0.1934156379f, 0.6576000000f, 0.2536443149f),
    plVec4(0.1132812500f, 0.5267489712f, 0.8576000000f, 0.3965014577f),
    plVec4(0.6132812500f, 0.8600823045f, 0.0976000000f, 0.5393586006f),
    plVec4(0.3632812500f, 0.3045267490f, 0.2976000000f, 0.6822157434f),
    plVec4(0.8632812500f, 0.6378600823f, 0.4976000000f, 0.8250728863f),
    plVec4(0.2382812500f, 0.9711934156f, 0.6976000000f, 0.9679300292f),
    plVec4(0.7382812500f, 0.0205761317f, 0.8976000000f, 0.1311953353f),
    plVec4(0.4882812500f, 0.3539094650f, 0.1376000000f, 0.2740524781f),
    plVec4(0.9882812500f, 0.6872427984f, 0.3376000000f, 0.4169096210f),
    plVec4(0.0117187500f, 0.1316872428f, 0.5376000000f, 0.5597667638f),
    plVec4(0.5117187500f, 0.4650205761f, 0.7376000000f, 0.7026239067f),
    plVec4(0.2617187500f, 0.7983539095f, 0.9376000000f, 0.8454810496f),
    plVec4(0.7617187500f, 0.2427983539f, 0.1776000000f, 0.9883381924f),
    plVec4(0.1367187500f, 0.5761316872f, 0.3776000000f, 0.0116618076f),
    plVec4(0.6367187500f, 0.9094650206f, 0.5776000000f, 0.1545189504f),
    plVec4(0.3867187500f, 0.0576131687f, 0.7776000000f, 0.2973760933f),
    plVec4(0.8867187500f, 0.3909465021f, 0.9776000000f, 0.4402332362f),
    plVec4(0.0742187500f, 0.7242798354f, 0.0256000000f, 0.5830903790f),
    plVec4(0.5742187500f, 0.1687242798f, 0.2256000000f, 0.7259475219f),
    plVec4(0.3242187500f, 0.5020576132f, 0.4256000000f, 0.8688046647f),
    plVec4(0.8242187500f, 0.8353909465f, 0.6256000000f, 0.0320699708f),
    plVec4(0.1992187500f, 0.2798353909f, 0.8256000000f, 0.1749271137f),
    plVec4(0.6992187500f, 0.6131687243f, 0.0656000000f, 0.3177842566f),
    plVec4(0.4492187500f, 0.9465020576f, 0.2656000000f, 0.4606413994f),
    plVec4(0.9492187500f, 0.0946502058f, 0.4656000000f, 0.6034985423f),
    plVec4(0.0429687500f, 0.4279835391f, 0.6656000000f, 0.7463556851f),
    plVec4(0.5429687500f, 0.7613168724f, 0.8656000000f, 0.8892128280f),
    plVec4(0.2929687500f, 0.2057613169f, 0.1056000000f, 0.0524781341f),
    plVec4(0.7929687500f, 0.5390946502f, 0.3056000000f, 0.1953352770f),
    plVec4(0.1679687500f, 0.8724279835f, 0.5056000000f, 0.3381924198f),
    plVec4(0.6679687500f, 0.3168724280f, 0.7056000000f, 0.4810495627f),
    plVec4(0.4179687500f, 0.6502057613f, 0.9056000000f, 0.6239067055f),
    plVec4(0.9179687500f, 0.9835390947f, 0.1456000000f, 0.7667638484f),
    plVec4(0.1054687500f, 0.0329218107f, 0.3456000000f, 0.9096209913f),
    plVec4(0.6054687500f, 0.3662551440f, 0.5456000000f, 0.0728862974f),
    plVec4(0.3554687500f, 0.6995884774f, 0.7456000000f, 0.2157434402f),
    plVec4(0.8554687500f, 0.1440329218f, 0.9456000000f, 0.3586005831f),
    plVec4(0.2304687500f, 0.4773662551f, 0.1856000000f, 0.5014577259f),
    plVec4(0.7304687500f, 0.8106995885f, 0.3856000000f, 0.6443148688f),
    plVec4(0.4804687500f, 0.2551440329f, 0.5856000000f, 0.7871720117f),
    plVec4(0.9804687500f, 0.5884773663f, 0.7856000000f, 0.9300291545f),
    plVec4(0.0273437500f, 0.9218106996f, 0.9856000000f, 0.0932944606f),
    plVec4(0.5273437500f, 0.0699588477f, 0.0336000000f, 0.2361516035f),
    plVec4(0.2773437500f, 0.4032921811f, 0.2336000000f, 0.3790087464f),
    plVec4(0.7773437500f, 0.7366255144f, 0.4336000000f, 0.5218658892f),
    plVec4(0.1523437500f, 0.1810699588f, 0.6336000000f, 0.6647230321f),
    plVec4(0.6523437500f, 0.5144032922f, 0.8336000000f, 0.8075801749f),
    plVec4(0.4023437500f, 0.8477366255f, 0.0736000000f, 0.9504373178f),
    plVec4(0.9023437500f, 0.2921810700f, 0.2736000000f, 0.1137026239f),
    plVec4(0.0898437500f, 0.6255144033f, 0.4736000000f, 0.2565597668f),
    plVec4(0.5898437500f, 0.9588477366f, 0.6736000000f, 0.3994169096f),
    plVec4(0.3398437500f, 0.1069958848f, 0.8736000000f, 0.5422740525f),
    plVec4(0.8398437500f, 0.4403292181f, 0.1136000000f, 0.6851311953f),
    plVec4(0.2148437500f, 0.7736625514f, 0.3136000000f, 0.8279883382f),
    plVec4(0.7148437500f, 0.2181069959f, 0.5136000000f, 0.9708454810f),
    plVec4(0.4648437500f, 0.5514403292f, 0.7136000000f, 0.1341107872f),
    plVec4(0.9648437500f, 0.8847736626f, 0.9136000000f, 0.2769679300f),
    plVec4(0.0585937500f, 0.3292181070f, 0.1536000000f, 0.4198250729f),
    plVec4(0.5585937500f, 0.6625514403f, 0.3536000000f, 0.5626822157f),
    plVec4(0.3085937500f, 0.9958847737f, 0.5536000000f, 0.7055393586f),
    plVec4(0.8085937500f, 0.0013717421f, 0.7536000000f, 0.8483965015f),
    plVec4(0.1835937500f, 0.3347050754f, 0.9536000000f, 0.9912536443f),
    plVec4(0.6835937500f, 0.6680384088f, 0.1936000000f, 0.0145772595f),
    plVec4(0.4335937500f, 0.1124828532f, 0.3936000000f, 0.1574344023f),
    plVec4(0.9335937500f, 0.4458161866f, 0.5936000000f, 0.3002915452f),
    plVec4(0.1210937500f, 0.7791495199f, 0.7936000000f, 0.4431486880f),
    plVec4(0.6210937500f, 0.2235939643f, 0.9936000000f, 0.5860058309f),
    plVec4(0.3710937500f, 0.5569272977f, 0.0032000000f, 0.7288629738f),
    plVec4(0.8710937500f, 0.8902606310f, 0.2032000000f, 0.8717201166f),
    plVec4(0.2460937500f, 0.0384087791f, 0.4032000000f, 0.0349854227f),
    plVec4(0.7460937500f, 0.3717421125f, 0.6032000000f, 0.1778425656f),
    plVec4(0.4960937500f, 0.7050754458f, 0.8032000000f, 0.3206997085f),
    plVec4(0.9960937500f, 0.1495198903f, 0.0432000000f, 0.4635568513f),
    plVec4(0.0019531250f, 0.4828532236f, 0.2432000000f, 0.6064139942f),
  };
  return HALTON[idx % (sizeof(HALTON) / sizeof(HALTON[0]))];
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
    gc.LastCameraToScreenMatrix[i] = pViewData->m_LastProjectionMatrix[i];
    gc.LastScreenToCameraMatrix[i] = pViewData->m_LastInverseProjectionMatrix[i];
    gc.LastWorldToCameraMatrix[i] = pViewData->m_LastViewMatrix[i];
    gc.LastCameraToWorldMatrix[i] = pViewData->m_LastInverseViewMatrix[i];
    gc.LastWorldToScreenMatrix[i] = pViewData->m_LastViewProjectionMatrix[i];
    gc.LastScreenToWorldMatrix[i] = pViewData->m_LastInverseViewProjectionMatrix[i];

    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i] = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i] = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i] = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i] = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const plRectFloat& renderViewport = pViewData->m_ViewPortRect;
  const plRectFloat& targetViewport = pViewData->m_TargetViewportRect;

  gc.ViewportSize = plVec4(renderViewport.width, renderViewport.height, 1.0f / renderViewport.width, 1.0f / renderViewport.height);
  gc.TargetViewportSize = plVec4(targetViewport.width, targetViewport.height, 1.0f / targetViewport.width, 1.0f / targetViewport.height);

  float fNear = pCamera->GetNearPlane();
  float fFar = pCamera->GetFarPlane();
  gc.ClipPlanes = plVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsDirectionalLightShadow = pViewData->m_CameraUsageHint == plCameraUsageHint::Shadow && pCamera->IsOrthographic();
  gc.MaxZValue = bIsDirectionalLightShadow ? 0.0f : plMath::MinValue<float>();

  // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
  gc.DeltaTime = (float)plClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)plMath::Mod(plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
  gc.WorldTime = (float)plMath::Mod(data.GetWorldTime().GetSeconds(), 20790.0);

  gc.ShutterSpeed = pCamera->GetShutterSpeed();
  gc.Exposure = pCamera->GetExposure();
  gc.Aperture = pCamera->GetAperture();
  gc.ISO = pCamera->GetISO();
  gc.FocusDistance = pCamera->GetFocusDistance();

  gc.Gamma = cvar_RendererGamma;
  gc.DiffuseFromProbes = cvar_RendererDiffuseFromProbes;

//  if(plRenderWorld::GetTAAEnabled())
//  {
    const plVec4& halton = GetHaltonSequence(plRenderWorld::GetFrameCounter() % 256);
    plVec2 jitter;
    jitter.x = ((halton.x * 2 - 1) / renderViewport.width);
    jitter.y = ((halton.y * 2 - 1) / renderViewport.height);

    gc.TAAJitterCurrent = jitter;
    gc.TAAJitterPrevious = m_LastJitter;

    m_LastJitter = jitter;
//  }
//  else
//  {
//    gc.TAAJitterCurrent = plVec2(0.0f, 0.0f);
//    gc.TAAJitterPrevious = plVec2(0.0f, 0.0f);
//  }

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

void plRenderPipeline::CreateDgmlGraph(plDGMLGraph& graph)
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
    plUInt32 uiGraphNode = graph.AddNode(sTmp, &nd);
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
      plUInt32 uiTextureNode = graph.AddNode(sTmp, &nd);

      plUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const plRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        plUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
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
