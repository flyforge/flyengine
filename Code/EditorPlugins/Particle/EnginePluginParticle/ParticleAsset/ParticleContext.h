#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class plParticleComponent;

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

class PL_ENGINEPLUGINPARTICLE_DLL plParticleContext : public plEngineProcessDocumentContext
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleContext, plEngineProcessDocumentContext);

public:
  plParticleContext();
  ~plParticleContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual void OnThumbnailViewContextRequested() override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

private:
  plBoundingBoxSphere m_ThumbnailBoundingVolume;
  plParticleEffectResourceHandle m_hParticle;
  plMeshResourceHandle m_hPreviewMeshResource;
  plParticleComponent* m_pComponent = nullptr;
};
