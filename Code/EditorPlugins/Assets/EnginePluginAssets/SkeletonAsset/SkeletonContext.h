#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Declarations.h>

class PL_ENGINEPLUGINASSETS_DLL plSkeletonContext : public plEngineProcessDocumentContext
{
  PL_ADD_DYNAMIC_REFLECTION(plSkeletonContext, plEngineProcessDocumentContext);

public:
  plSkeletonContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  plSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pGameObject = nullptr;
  plSkeletonResourceHandle m_hSkeleton;
  plComponentHandle m_hSkeletonComponent;
  plComponentHandle m_hPoseComponent;
  plString m_sAnimatedMeshToUse;
  plComponentHandle m_hAnimMeshComponent;
};
