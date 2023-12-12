#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>

struct plAssetInfo;
struct plAssetCuratorEvent;
struct plSubAsset;

class PLASMA_EDITORFRAMEWORK_DLL plQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit plQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(const plSubAsset* pInfo) const = 0;
  virtual bool Less(const plSubAsset* pInfoA, const plSubAsset* pInfoB) const = 0;

Q_SIGNALS:
  void FilterChanged();
};

class PLASMA_EDITORFRAMEWORK_DLL plQtAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0,
    AssetGuid,
    AbsolutePath,
    RelativePath,
    ParentRelativePath,
    AssetIcon,
    TransformState,
    Type, //wether it is an asset or folder
  };

  plQtAssetBrowserModel(QObject* pParent, plQtAssetFilter* pFilter);
  ~plQtAssetBrowserModel();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

private Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2);
  void ThumbnailInvalidated(QString sPath, plUInt32 uiImageID);

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

private:
  friend struct AssetComparer;
  struct AssetEntry
  {
    plUuid m_Guid;
    mutable plUInt32 m_uiThumbnailID;
  };

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };
  void AssetCuratorEventHandler(const plAssetCuratorEvent& e);
  plInt32 FindAssetIndex(const plUuid& assetGuid) const;
  void HandleAsset(const plSubAsset* pInfo, AssetOp op);
  void Init(AssetEntry& ae, const plSubAsset* pInfo);

  plQtAssetFilter* m_pFilter;
  plDynamicArray<AssetEntry> m_AssetsToDisplay;
  plSet<plUuid> m_DisplayedEntries;

  bool m_bIconMode;
};

