#pragma once

#include <Foundation/Reflection/Reflection.h>

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

struct plPostprocessTileStatistics;

class PL_RENDERERCORE_DLL plSSRPass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plSSRPass, plRenderPipelinePass);

public:
  plSSRPass();
  ~plSSRPass() override;

  bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;


protected:
  void UpdateSSRConstantBuffer() const;

  float m_fMaxDist;
  float m_fResolution;
  float m_fThickness;
  int m_iSteps;

  plRenderPipelineNodeInputPin m_PinInputColor;
  plRenderPipelineNodeInputPin m_PinInputDepth;
  plRenderPipelineNodeInputPin m_PinInputMaterial;
  plRenderPipelineNodeInputPin m_PinInputVelocity;
  plRenderPipelineNodeOutputPin m_PinOutput;

  plConstantBufferStorageHandle m_hSSRConstantBuffer;
  plConstantBufferStorageHandle m_hPostProcessConstantBuffer;
  plShaderResourceHandle m_hShaderTileMinMaxRoughnessHorizontal;
  plShaderResourceHandle m_hShaderTileMinMaxRoughnessVerticle;
  plShaderResourceHandle m_hShaderDepthHierarchy;
  plShaderResourceHandle m_hShaderSSRTrace;

  plGALTextureHandle m_hTextureTileMinMaxRoughnessHorizontal;
  plGALTextureHandle m_hTileMinMaxRoughness;
  plGALTextureHandle m_hDepthHierarchy;
  plGALTextureHandle m_hDepthHierarchyTmp;
  plGALTextureHandle m_hIndirectSpecular;
  plGALTextureHandle m_hDirectionPDF;
  plGALTextureHandle m_hRayLength;

  plTexture2DResourceHandle m_hBlueNoiseTexture;

  plGALBufferHandle m_hTileStatisticsBuffer;
  plArrayPtr<plPostprocessTileStatistics> m_TileStatistics;

  plGALBufferHandle m_hTilesTracingEarlyexitBuffer;
  plGALBufferHandle m_hTilesTracingCheapBuffer;
  plGALBufferHandle m_hTilesTracingExpensiveBuffer;
  plArrayPtr<plUInt32> m_TilesTracingEarlyexit;
  plArrayPtr<plUInt32> m_TilesTracingCheap;
  plArrayPtr<plUInt32> m_TilesTracingExpensive;

  float m_MinMaxRoughnessWidth;
  float m_MinMaxRoughnessHeight;
};
