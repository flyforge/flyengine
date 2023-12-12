#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginParticle/EnginePluginParticleDLL.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class plParticleComponent;

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

class PLASMA_ENGINEPLUGINPARTICLE_DLL plParticleContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleContext, PlasmaEngineProcessDocumentContext);

public:
  plParticleContext();
  ~plParticleContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual void OnThumbnailViewContextRequested() override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

  void RestartEffect();
  void SetAutoRestartEffect(bool loop);

private:
  plBoundingBoxSphere m_ThumbnailBoundingVolume;
  plParticleEffectResourceHandle m_hParticle;
  plMeshResourceHandle m_hPreviewMeshResource;
  plParticleComponent* m_pComponent = nullptr;
};
