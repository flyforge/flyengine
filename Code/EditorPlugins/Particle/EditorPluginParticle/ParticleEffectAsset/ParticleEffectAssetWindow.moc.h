#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plParticleEffectAssetDocument;
class QComboBox;
class QToolButton;
class plQtPropertyGridWidget;


class plQtParticleEffectAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtParticleEffectAssetDocumentWindow(plAssetDocument* pDocument);
  ~plQtParticleEffectAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override;
  plParticleEffectAssetDocument* GetParticleDocument();

private Q_SLOTS:
  void onSystemSelected(int index);
  void onAddSystem(bool);
  void onRemoveSystem(bool);
  void onRenameSystem(bool);

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();
  void RestoreResource();
  void SendLiveResourcePreview();
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void ParticleEventHandler(const plParticleEffectAssetEvent& e);
  void UpdateSystemList();
  void SelectSystem(plDocumentObject* pObject);

  plParticleEffectAssetDocument* m_pAssetDoc;

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;

  QComboBox* m_pSystemsCombo = nullptr;
  QToolButton* m_pAddSystem = nullptr;
  QToolButton* m_pRemoveSystem = nullptr;
  QToolButton* m_pRenameSystem = nullptr;
  plQtPropertyGridWidget* m_pPropertyGridSystems = nullptr;
  plQtPropertyGridWidget* m_pPropertyGridEmitter = nullptr;
  plQtPropertyGridWidget* m_pPropertyGridInitializer = nullptr;
  plQtPropertyGridWidget* m_pPropertyGridBehavior = nullptr;
  plQtPropertyGridWidget* m_pPropertyGridType = nullptr;

  plString m_sSelectedSystem;
  plMap<plString, plDocumentObject*> m_ParticleSystems;
};

