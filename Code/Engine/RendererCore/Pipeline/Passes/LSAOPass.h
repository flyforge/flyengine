#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/LSAOConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Defines the depth compare function to be used to decide sample weights.
struct PLASMA_RENDERERCORE_DLL plLSAODepthCompareFunction
{
  using StorageType = plUInt8;

  enum Enum
  {
    Depth,                   ///< A hard cutoff function between the linear depth values. Samples with an absolute distance greater than
                             ///< plLSAOPass::SetDepthCutoffDistance are ignored.
    Normal,                  ///< Samples that are on the same plane as constructed by the center position and normal will be weighted higher than those samples that
                             ///< are above or below the plane.
    NormalAndSampleDistance, ///< Same as Normal, but if two samples are tested, their distance to the center position is is inversely multiplied as
                             ///< well, giving closer matches a higher weight.
    Default = NormalAndSampleDistance
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plLSAODepthCompareFunction);

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class PLASMA_RENDERERCORE_DLL plLSAOPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLSAOPass, plRenderPipelinePass);

public:
  plLSAOPass();
  ~plLSAOPass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  plUInt32 GetLineToLinePixelOffset() const { return m_iLineToLinePixelOffset; }
  void SetLineToLinePixelOffset(plUInt32 uiPixelOffset);
  plUInt32 GetLineSamplePixelOffset() const { return m_iLineSamplePixelOffsetFactor; }
  void SetLineSamplePixelOffset(plUInt32 uiPixelOffset);

  // Factor used for depth cutoffs (determines when a depth difference is too large to be considered)
  float GetDepthCutoffDistance() const;
  void SetDepthCutoffDistance(float fDepthCutoffDistance);

  // Determines how quickly the occlusion falls of.
  float GetOcclusionFalloff() const;
  void SetOcclusionFalloff(float fFalloff);


protected:
  /// Destroys all GPU data that might have been created in in SetupLineSweepData
  void DestroyLineSweepData();
  void SetupLineSweepData(const plVec3I32& imageResolution);


  void AddLinesForDirection(const plVec3I32& imageResolution, const plVec2I32& sampleDir, plUInt32 lineIndex, plDynamicArray<LineInstruction>& outinLineInstructions, plUInt32& outinTotalNumberOfSamples);

  plRenderPipelineNodeInputPin m_PinDepthInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plConstantBufferStorageHandle m_hLineSweepCB;

  bool m_bSweepDataDirty = true;
  bool m_bConstantsDirty = true;

  /// Output of the line sweep pass.
  plGALBufferHandle m_hLineSweepOutputBuffer;
  plGALUnorderedAccessViewHandle m_hLineSweepOutputUAV;
  plGALResourceViewHandle m_hLineSweepOutputSRV;

  /// Structured buffer containing instructions for every single line to trace.
  plGALBufferHandle m_hLineInfoBuffer;
  plGALResourceViewHandle m_hLineSweepInfoSRV;

  /// Total number of lines to be traced.
  plUInt32 m_uiNumSweepLines = 0;

  plInt32 m_iLineToLinePixelOffset = 2;
  plInt32 m_iLineSamplePixelOffsetFactor = 1;
  float m_fOcclusionFalloff = 0.2f;
  float m_fDepthCutoffDistance = 4.0f;

  plEnum<plLSAODepthCompareFunction> m_DepthCompareFunction;
  bool m_bDistributedGathering = true;

  plShaderResourceHandle m_hShaderLineSweep;
  plShaderResourceHandle m_hShaderGather;
  plShaderResourceHandle m_hShaderAverage;
};
