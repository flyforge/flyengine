#pragma once

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <QTreeView>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtPropertyAnimModel;
class QItemSelection;
class QItemSelectionModel;
class plQtCurve1DEditorWidget;
class plQtEventTrackEditorWidget;
struct plDocumentObjectPropertyEvent;
struct plDocumentObjectStructureEvent;
class plPropertyAnimAssetDocument;
class plQtColorGradientEditorWidget;
class plColorGradientAssetData;
class plQtQuadViewWidget;
class plQtTimeScrubberToolbar;
struct plPropertyAnimAssetDocumentEvent;
class QKeyEvent;
class plQtDocumentPanel;

class plQtPropertyAnimAssetTreeView : public QTreeView
{
  Q_OBJECT

public:
  plQtPropertyAnimAssetTreeView(QWidget* pParent);
  void initialize();

Q_SIGNALS:
  void DeleteSelectedItemsEvent();
  void FrameSelectedItemsEvent();
  void RebindSelectedItemsEvent();

protected slots:
  void onBeforeModelReset();
  void onAfterModelReset();

protected:
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  void storeExpandState(const QModelIndex& parent);
  void restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection);

  QSet<QString> m_NotExpandedState;
  QSet<QString> m_SelectedItems;
};

class plQtPropertyAnimAssetDocumentWindow : public plQtGameObjectDocumentWindow, public plGameObjectGizmoInterface
{
  Q_OBJECT

public:
  plQtPropertyAnimAssetDocumentWindow(plPropertyAnimAssetDocument* pDocument);
  ~plQtPropertyAnimAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "PropertyAnimAsset"; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name plGameObjectGizmoInterface implementation
  ///@{
  virtual plObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void InternalRedraw() override;
  void PropertyAnimAssetEventHandler(const plPropertyAnimAssetDocumentEvent& e);

private Q_SLOTS:
  void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
  void onScrubberPosChanged(plUInt64 uiTick);
  void onDeleteSelectedItems();
  void onRebindSelectedItems();
  void onPlaybackTick();
  void onPlayPauseClicked();
  void onRepeatClicked();
  void onAdjustDurationClicked();
  void onDurationChangedEvent(double duration);
  void onTreeItemDoubleClicked(const QModelIndex& index);
  void onFrameSelectedTracks();

  //////////////////////////////////////////////////////////////////////////
  // Curve editor events

  void onCurveInsertCpAt(plUInt32 uiCurveIdx, plInt64 tickX, double newPosY);
  void onCurveCpMoved(plUInt32 curveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY);
  void onCurveCpDeleted(plUInt32 curveIdx, plUInt32 cpIdx);
  void onCurveTangentMoved(plUInt32 curveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(plUInt32 curveIdx, plUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(plUInt32 curveIdx, plUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

  //////////////////////////////////////////////////////////////////////////
  // Color gradient editor events

  void onGradientColorCpAdded(double posX, const plColorGammaUB& color);
  void onGradientAlphaCpAdded(double posX, plUInt8 alpha);
  void onGradientIntensityCpAdded(double posX, float intensity);
  void MoveGradientCP(plInt32 idx, double newPosX, const char* szArrayName);
  void onGradientColorCpMoved(plInt32 idx, double newPosX);
  void onGradientAlphaCpMoved(plInt32 idx, double newPosX);
  void onGradientIntensityCpMoved(plInt32 idx, double newPosX);
  void RemoveGradientCP(plInt32 idx, const char* szArrayName);
  void onGradientColorCpDeleted(plInt32 idx);
  void onGradientAlphaCpDeleted(plInt32 idx);
  void onGradientIntensityCpDeleted(plInt32 idx);
  void onGradientColorCpChanged(plInt32 idx, const plColorGammaUB& color);
  void onGradientAlphaCpChanged(plInt32 idx, plUInt8 alpha);
  void onGradientIntensityCpChanged(plInt32 idx, float intensity);
  void onGradientBeginOperation();
  void onGradientEndOperation(bool commit);
  // void onGradientNormalizeRange();

  //////////////////////////////////////////////////////////////////////////
  // Event track editor events
  void onEventTrackInsertCpAt(plInt64 tickX, QString value);
  void onEventTrackCpMoved(plUInt32 cpIdx, plInt64 iTickX);
  void onEventTrackCpDeleted(plUInt32 cpIdx);
  void onEventTrackBeginOperation(QString name);
  void onEventTrackEndOperation(bool commit);
  void onEventTrackBeginCpChanges(QString name);
  void onEventTrackEndCpChanges();

  //////////////////////////////////////////////////////////////////////////

private:
  plPropertyAnimAssetDocument* GetPropertyAnimDocument();
  // void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void UpdateCurveEditor();
  void UpdateGradientEditor();
  void UpdateEventTrackEditor();
  void UpdateSelectionData();

  plQtQuadViewWidget* m_pQuadViewWidget;
  plCurveGroupData m_CurvesToDisplay;
  plColorGradientAssetData* m_pGradientToDisplay = nullptr;
  plInt32 m_iMapGradientToTrack = -1;
  plDynamicArray<plInt32> m_MapSelectionToTrack;
  plQtPropertyAnimAssetTreeView* m_pPropertyTreeView = nullptr;
  plQtPropertyAnimModel* m_pPropertiesModel;
  QItemSelectionModel* m_pSelectionModel = nullptr;
  plQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
  plQtEventTrackEditorWidget* m_pEventTrackEditor = nullptr;
  plQtColorGradientEditorWidget* m_pGradientEditor = nullptr;
  plQtTimeScrubberToolbar* m_pScrubberToolbar = nullptr;
  plQtDocumentPanel* m_pCurvePanel = nullptr;
  plQtDocumentPanel* m_pColorGradientPanel = nullptr;
  plQtDocumentPanel* m_pEventTrackPanel = nullptr;
  bool m_bAnimTimerInFlight = false;
};

