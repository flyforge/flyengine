#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Types/Uuid.h>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

struct plAssetInfo;
struct plAssetCuratorEvent;
struct plSubAsset;
class plQtAssetFilter;

/// \brief Interface class of the asset filter used to decide which items are shown in the asset browser.
class PLASMA_EDITORFRAMEWORK_DLL plQtAssetFilter : public QObject
{
  Q_OBJECT
public:
  explicit plQtAssetFilter(QObject* pParent);
  virtual bool IsAssetFiltered(plStringView sDataDirParentRelativePath, bool bIsFolder, const plSubAsset* pInfo) const = 0;
  virtual plStringView GetFilterRelativePath(plStringView sDataDirParentRelativePath) const { return sDataDirParentRelativePath; }
  virtual bool GetSortByRecentUse() const { return false; }

Q_SIGNALS:
  void FilterChanged();
};

/// \brief Each item in the asset browser can be multiple things at the same time as described by these flags.
/// Retrieved via user role plQtAssetBrowserModel::UserRoles::ItemFlags.
struct PLASMA_EDITORFRAMEWORK_DLL plAssetBrowserItemFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    Folder = PLASMA_BIT(0),        // Any folder inside a data directory
    DataDirectory = PLASMA_BIT(1), // mutually exclusive with Folder
    File = PLASMA_BIT(2),          // any file, could also be an Asset
    Asset = PLASMA_BIT(3),         // main asset: mutually exclusive with SubAsset
    SubAsset = PLASMA_BIT(4),      // sub-asset (imaginary, not a File or Asset)
    Default = 0
  };

  struct Bits
  {
    StorageType Folder : 1;
    StorageType DataDirectory : 1;
    StorageType File : 1;
    StorageType Asset : 1;
    StorageType SubAsset : 1;
  };
};
PLASMA_DECLARE_FLAGS_OPERATORS(plAssetBrowserItemFlags);

/// \brief Model of the item view in the asset browser.
class PLASMA_EDITORFRAMEWORK_DLL plQtAssetBrowserModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum UserRoles
  {
    SubAssetGuid = Qt::UserRole + 0, // plUuid
    AssetGuid,                       // plUuid
    AbsolutePath,                    // QString
    RelativePath,                    // QString
    AssetIcon,                       // QIcon
    TransformState,                  // QString
    Importable,                      // bool
    ItemFlags,                       // plAssetBrowserItemFlags as int
  };

  plQtAssetBrowserModel(QObject* pParent, plQtAssetFilter* pFilter);
  ~plQtAssetBrowserModel();

  void resetModel();

  void SetIconMode(bool bIconMode) { m_bIconMode = bIconMode; }
  bool GetIconMode() { return m_bIconMode; }

  plInt32 FindAssetIndex(const plUuid& assetGuid) const;
  plInt32 FindIndex(plStringView sAbsPath) const;

public Q_SLOTS:
  void ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2);
  void ThumbnailInvalidated(QString sPath, plUInt32 uiImageID);
  void OnFileSystemUpdate();

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset) const;

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
  virtual Qt::DropActions supportedDropActions() const override;

private:
  friend struct FileComparer;

  enum class AssetOp
  {
    Add,
    Remove,
    Updated,
  };

  struct VisibleEntry
  {
    plDataDirPath m_sAbsFilePath;
    plUuid m_Guid;
    plBitflags<plAssetBrowserItemFlags> m_Flags;
    mutable plUInt32 m_uiThumbnailID;
  };

  struct FsEvent
  {
    plFileChangedEvent m_FileEvent;
    plFolderChangedEvent m_FolderEvent;
  };

private:
  void AssetCuratorEventHandler(const plAssetCuratorEvent& e);
  void HandleEntry(const VisibleEntry& entry, AssetOp op);
  void FileSystemFileEventHandler(const plFileChangedEvent& e);
  void FileSystemFolderEventHandler(const plFolderChangedEvent& e);
  void HandleFile(const plFileChangedEvent& e);
  void HandleFolder(const plFolderChangedEvent& e);

private:
  plQtAssetFilter* m_pFilter = nullptr;
  bool m_bIconMode = true;
  plSet<plString> m_ImportExtensions;

  plMutex m_Mutex;
  plDynamicArray<FsEvent> m_QueuedFileSystemEvents;

  plDynamicArray<VisibleEntry> m_EntriesToDisplay;
  plSet<plUuid> m_DisplayedEntries;

  QFileIconProvider m_IconProvider;
};
