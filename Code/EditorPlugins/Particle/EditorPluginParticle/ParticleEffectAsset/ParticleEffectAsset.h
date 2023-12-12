#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Communication/Event.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

class plParticleEffectAssetDocument;
struct plPropertyMetaStateEvent;

struct plParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
    AutoRestartChanged,
    SimulationSpeedChanged,
    RenderVisualizersChanged,
  };

  plParticleEffectAssetDocument* m_pDocument;
  Type m_Type;
};

class plParticleEffectAssetDocument : public plSimpleAssetDocument<plParticleEffectDescriptor>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEffectAssetDocument, plSimpleAssetDocument<plParticleEffectDescriptor>);

public:
  plParticleEffectAssetDocument(const char* szDocumentPath);

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  void WriteResource(plStreamWriter& stream) const;

  void TriggerRestartEffect();

  plEvent<const plParticleEffectAssetEvent&> m_Events;

  void SetAutoRestart(bool enable);
  bool GetAutoRestart() const { return m_bAutoRestart; }

  void SetSimulationPaused(bool bPaused);
  bool GetSimulationPaused() const { return m_bSimulationPaused; }

  void SetSimulationSpeed(float speed);
  float GetSimulationSpeed() const { return m_fSimulationSpeed; }

  bool GetRenderVisualizers() const { return m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  // Overridden to enable support for visualizers/manipulators
  virtual plResult ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_Result) const override;

protected:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  bool m_bSimulationPaused = false;
  bool m_bAutoRestart = true;
  bool m_bRenderVisualizers = false;
  float m_fSimulationSpeed = 1.0f;
};
