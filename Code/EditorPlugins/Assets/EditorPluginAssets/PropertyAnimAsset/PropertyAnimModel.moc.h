#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <QAbstractItemModel>
#include <QIcon>

class plPropertyAnimAssetDocument;
struct plDocumentObjectPropertyEvent;
struct plDocumentObjectStructureEvent;
class plPropertyAnimationTrack;

struct plQtPropertyAnimModelTreeEntry
{
  plString m_sPathToItem;
  plInt32 m_iParent = -1;
  plUInt16 m_uiOwnRowIndex = 0;
  plPropertyAnimationTrack* m_pTrack = nullptr;
  plInt32 m_iTrackIdx = -1;
  plString m_sDisplay;
  plDynamicArray<plInt32> m_Children;
  QIcon m_Icon;

  bool operator==(const plQtPropertyAnimModelTreeEntry& rhs) const
  {
    return (m_iParent == rhs.m_iParent) && (m_uiOwnRowIndex == rhs.m_uiOwnRowIndex) && (m_pTrack == rhs.m_pTrack) &&
           (m_iTrackIdx == rhs.m_iTrackIdx) && (m_sDisplay == rhs.m_sDisplay) && (m_Children == rhs.m_Children);
  }

  bool operator!=(const plQtPropertyAnimModelTreeEntry& rhs) const { return !(*this == rhs); }
};

class plQtPropertyAnimModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  plQtPropertyAnimModel(plPropertyAnimAssetDocument* pDocument, QObject* pParent);
  ~plQtPropertyAnimModel();

  enum UserRoles
  {
    TrackPtr = Qt::UserRole + 1,
    TreeItem = Qt::UserRole + 2,
    TrackIdx = Qt::UserRole + 3,
    Path = Qt::UserRole + 4,
  };

  const plDeque<plQtPropertyAnimModelTreeEntry>& GetAllEntries() const { return m_AllEntries[m_iInUse]; }

private Q_SLOTS:
  void onBuildMappingTriggered();

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void DocumentStructureEventHandler(const plDocumentObjectStructureEvent& e);
  void DocumentPropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void TriggerBuildMapping();
  void BuildMapping();
  void BuildMapping(plInt32 iToUse);
  void BuildMapping(plInt32 iToUse, plInt32 iTrackIdx, plPropertyAnimationTrack* pTrack, plDynamicArray<plInt32>& treeItems, plInt32 iParentEntry,
    const char* szPath);

  bool m_bBuildMappingQueued = false;
  plInt32 m_iInUse = 0;
  plDynamicArray<plInt32> m_TopLevelEntries[2];
  plDeque<plQtPropertyAnimModelTreeEntry> m_AllEntries[2];

  plPropertyAnimAssetDocument* m_pAssetDoc = nullptr;
};

