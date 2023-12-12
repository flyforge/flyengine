#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plPickingRenderPass : public plRenderPipelinePass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPickingRenderPass, plRenderPipelinePass);

public:
  plPickingRenderPass();
  ~plPickingRenderPass();

  plGALTextureHandle GetPickingIdRT() const;
  plGALTextureHandle GetPickingDepthRT() const;

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs,
    plArrayPtr<plGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(
    const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs,
    const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

  virtual void ReadBackProperties(plView* pView) override;

  bool m_bPickSelected = true;
  bool m_bPickTransparent = true;

  plVec2 m_PickingPosition;
  plUInt32 m_PickingIdOut;
  float m_PickingDepthOut;
  plVec2 m_MarqueePickPosition0;
  plVec2 m_MarqueePickPosition1;
  plUInt32 m_uiMarqueeActionID = 0xFFFFFFFF; // used to prevent reusing an old result for a new marquee action
  plUInt32 m_uiWindowWidth;
  plUInt32 m_uiWindowHeight;

private:
  void CreateTarget();
  void DestroyTarget();

  void ReadBackPropertiesSinglePick(plView* pView);
  void ReadBackPropertiesMarqueePick(plView* pView);

private:
  plRectFloat m_TargetRect;

  plGALTextureHandle m_hPickingIdRT;
  plGALTextureHandle m_hPickingDepthRT;
  plGALRenderTargetSetup m_RenderTargetSetup;

  plHashSet<plGameObjectHandle> m_SelectionSet;


  /// we need this matrix to compute the world space position of picked pixels
  plMat4 m_mPickingInverseViewProjectionMatrix;

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  plDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the plComponent, etc. that rendered to that pixel
  plDynamicArray<plUInt32> m_PickingResultsID;
};
