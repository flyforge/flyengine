#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Declarations.h>

class PLASMA_ENGINEPLUGINASSETS_DLL plSkeletonContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSkeletonContext, PlasmaEngineProcessDocumentContext);

public:
  plSkeletonContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

  plSkeletonResourceHandle GetSkeleton() const { return m_hSkeleton; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pGameObject = nullptr;
  plSkeletonResourceHandle m_hSkeleton;
  plComponentHandle m_hSkeletonComponent;
  plComponentHandle m_hPoseComponent;
  plString m_sAnimatedMeshToUse;
  plComponentHandle m_hAnimMeshComponent;
};
