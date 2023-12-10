#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Time/Clock.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plQtTimeScrubberWidget;
class plQtEventTrackEditorWidget;
class plQtDocumentPanel;
struct plCommandHistoryEvent;

class plQtAnimationClipAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtAnimationClipAssetDocumentWindow(plAnimationClipAssetDocument* pDocument);
  ~plQtAnimationClipAssetDocumentWindow();

  plAnimationClipAssetDocument* GetAnimationClipDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationClipAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;

  virtual void CommonAssetUiEventHandler(const plCommonAssetUiState& e) override;
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);

protected Q_SLOTS:
  void OnScrubberPosChangedEvent(plUInt64 uiNewScrubberTickPos);

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(plInt64 tickX, QString value);
  void onEventTrackCpMoved(plUInt32 cpIdx, plInt64 iTickX);
  void onEventTrackCpDeleted(plUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void UpdateEventTrackEditor();

  plClock m_Clock;
  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  plQtTimeScrubberWidget* m_pTimeScrubber = nullptr;
  plTime m_ClipDuration;
  plTime m_PlaybackPosition;

  plQtDocumentPanel* m_pEventTrackPanel = nullptr;
  plQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
};
