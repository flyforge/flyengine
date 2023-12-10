#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

class plProfilingId;
class plView;
class plRenderPipelinePass;
class plFrameDataProviderBase;
struct plPermutationVar;
class plDGMLGraph;
class plFrustum;
class plRasterizerView;

class PLASMA_RENDERERCORE_DLL plRenderPipeline : public plRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized
  };

  plRenderPipeline();
  ~plRenderPipeline();

  void AddPass(plUniquePtr<plRenderPipelinePass>&& pPass);
  void RemovePass(plRenderPipelinePass* pPass);
  void GetPasses(plDynamicArray<const plRenderPipelinePass*>& ref_passes) const;
  void GetPasses(plDynamicArray<plRenderPipelinePass*>& ref_passes);
  plRenderPipelinePass* GetPassByName(const plStringView& sPassName);
  plHashedString GetViewName() const;

  bool Connect(plRenderPipelinePass* pOutputNode, const char* szOutputPinName, plRenderPipelinePass* pInputNode, const char* szInputPinName);
  bool Connect(plRenderPipelinePass* pOutputNode, plHashedString sOutputPinName, plRenderPipelinePass* pInputNode, plHashedString sInputPinName);
  bool Disconnect(plRenderPipelinePass* pOutputNode, plHashedString sOutputPinName, plRenderPipelinePass* pInputNode, plHashedString sInputPinName);

  const plRenderPipelinePassConnection* GetInputConnection(const plRenderPipelinePass* pPass, plHashedString sInputPinName) const;
  const plRenderPipelinePassConnection* GetOutputConnection(const plRenderPipelinePass* pPass, plHashedString sOutputPinName) const;

  void AddExtractor(plUniquePtr<plExtractor>&& pExtractor);
  void RemoveExtractor(plExtractor* pExtractor);
  void GetExtractors(plDynamicArray<const plExtractor*>& ref_extractors) const;
  void GetExtractors(plDynamicArray<plExtractor*>& ref_extractors);
  plExtractor* GetExtractorByName(const plStringView& sExtractorName);

  template <typename T>
  PLASMA_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(plGetStaticRTTI<T>()));
  }

  const plExtractedRenderData& GetRenderData() const;
  plRenderDataBatchList GetRenderDataBatchesWithCategory(
    plRenderData::Category category, plRenderDataBatch::Filter filter = plRenderDataBatch::Filter()) const;

  /// \brief Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(plDGMLGraph& ref_graph);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  static plCVarBool cvar_SpatialCullingVis;
#endif

  PLASMA_DISALLOW_COPY_AND_ASSIGN(plRenderPipeline);

private:
  friend class plRenderWorld;
  friend class plView;

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const plView& view);
  bool RebuildInternal(const plView& view);
  bool SortPasses();
  bool InitRenderTargetDescriptions(const plView& view);
  bool CreateRenderTargetUsage(const plView& view);
  bool InitRenderPipelinePasses();
  void SortExtractors();
  void UpdateViewData(const plView& view, plUInt32 uiDataIndex);

  void RemoveConnections(plRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const plRenderPipelinePass* pPass, const plHybridArray<plRenderPipelinePass*, 32>& done) const;
  bool ArePassThroughInputsDone(const plRenderPipelinePass* pPass, const plHybridArray<plRenderPipelinePass*, 32>& done) const;

  plFrameDataProviderBase* GetFrameDataProvider(const plRTTI* pRtti) const;

  void ExtractData(const plView& view);
  void FindVisibleObjects(const plView& view);

  void Render(plRenderContext* pRenderer);

  plRasterizerView* PrepareOcclusionCulling(const plFrustum& frustum, const plView& view);
  void PreviewOcclusionBuffer(const plRasterizerView& rasterizer, const plView& view);

private: // Member data
  // Thread data
  plThreadID m_CurrentExtractThread;
  plThreadID m_CurrentRenderThread;

  // Pipeline render data
  plExtractedRenderData m_Data[2];
  plDynamicArray<const plGameObject*> m_VisibleObjects;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  plTime m_AverageCullingTime;
#endif

  plHashedString m_sName;
  plUInt64 m_uiLastExtractionFrame;
  plUInt64 m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    plDynamicArray<plRenderPipelinePassConnection*> m_Inputs;
    plDynamicArray<plRenderPipelinePassConnection*> m_Outputs;
  };
  plDynamicArray<plUniquePtr<plRenderPipelinePass>> m_Passes;
  plMap<const plRenderPipelinePass*, ConnectionData> m_Connections;

  /// \brief Contains all connections that share the same path-through texture and their first and last usage pass index.
  struct TextureUsageData
  {
    plHybridArray<plRenderPipelinePassConnection*, 4> m_UsedBy;
    plUInt16 m_uiFirstUsageIdx;
    plUInt16 m_uiLastUsageIdx;
    plInt32 m_iTargetTextureIndex = -1;
  };
  plDynamicArray<TextureUsageData> m_TextureUsage;
  plDynamicArray<plUInt16> m_TextureUsageIdxSortedByFirstUsage; ///< Indices map into m_TextureUsage
  plDynamicArray<plUInt16> m_TextureUsageIdxSortedByLastUsage;  ///< Indices map into m_TextureUsage

  plHashTable<plRenderPipelinePassConnection*, plUInt32> m_ConnectionToTextureIndex;

  // Extractors
  plDynamicArray<plUniquePtr<plExtractor>> m_Extractors;
  plDynamicArray<plUniquePtr<plExtractor>> m_SortedExtractors;

  // Data Providers
  mutable plDynamicArray<plUniquePtr<plFrameDataProviderBase>> m_DataProviders;
  mutable plHashTable<const plRTTI*, plUInt32> m_TypeToDataProviderIndex;

  plDynamicArray<plPermutationVar> m_PermutationVars;

  // Occlusion Culling
  plGALTextureHandle m_hOcclusionDebugViewTexture;
};
