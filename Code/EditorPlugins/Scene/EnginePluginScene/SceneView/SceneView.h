#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plView;
class plViewRedrawMsgToEngine;
class plEngineProcessDocumentContext;
class plEditorEngineDocumentMsg;
class plEditorRenderPass;
class plSelectedObjectsExtractorBase;
class plSceneContext;
using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;
class plViewMarqueePickingMsgToEngine;

struct ObjectData
{
  plMat4 m_ModelView;
  float m_PickingID[4];
};

class plSceneViewContext : public plEngineProcessViewContext
{
public:
  plSceneViewContext(plSceneContext* pSceneContext);
  ~plSceneViewContext();

  virtual void HandleViewMessage(const plEditorEngineViewMsg* pMsg) override;
  virtual void SetupRenderTarget(plGALSwapChainHandle hSwapChain, const plGALRenderTargets* pRenderTargets, plUInt16 uiWidth, plUInt16 uiHeight) override;

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);
  void SetInvisibleLayerTags(const plArrayPtr<plTag> removeTags, const plArrayPtr<plTag> addTags);

protected:
  virtual void Redraw(bool bRenderEditorGizmos) override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;
  virtual plViewHandle CreateView() override;

  void PickObjectAt(plUInt16 x, plUInt16 y);
  void MarqueePickObjects(const plViewMarqueePickingMsgToEngine* pMsg);

private:
  plSceneContext* m_pSceneContext;

  bool m_bUpdatePickingData;

  plCamera m_CullingCamera;
};
