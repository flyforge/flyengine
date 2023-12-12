#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDragDropInfo;

/// \brief Adapter that defines data for specific type in the plQtDocumentTreeModel.
///
/// Adapters are defined for a given type and define the property for child elements (needs to be array or set).
/// Furthermore they implement various model functions that will be redirected to it by the model for
/// objects of the given type.
class PLASMA_EDITORFRAMEWORK_DLL plQtDocumentTreeModelAdapter : public QObject
{
  Q_OBJECT;

public:
  /// \brief Constructor. If m_sChildProperty is empty, this type does not have children.
  plQtDocumentTreeModelAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* m_sChildProperty);
  virtual const plRTTI* GetType() const;
  virtual const plString& GetChildProperty() const;

  virtual QVariant data(const plDocumentObject* pObject, int row, int column, int role = Qt::DisplayRole) const = 0;
  virtual bool setData(const plDocumentObject* pObject, int row, int column, const QVariant& value, int role) const;
  virtual Qt::ItemFlags flags(const plDocumentObject* pObject, int row, int column) const;

Q_SIGNALS:
  void dataChanged(const plDocumentObject* pObject, QVector<int> roles);

protected:
  const plDocumentObjectManager* m_pTree = nullptr;
  const plRTTI* m_pType = nullptr;
  plString m_sChildProperty;
};

/// \brief Convenience class that returns the typename as Qt::DisplayRole.
/// Use this for testing or for the document root that can't be seen and is just for defining the hierarchy.
///
/// Example:
/// plQtDummyAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children");
class PLASMA_EDITORFRAMEWORK_DLL plQtDummyAdapter : public plQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  plQtDummyAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* m_sChildProperty);

  virtual QVariant data(const plDocumentObject* pObject, int row, int column, int role) const override;
};

/// \brief Convenience class that implements getting the name via a property on the object.
class PLASMA_EDITORFRAMEWORK_DLL plQtNamedAdapter : public plQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  plQtNamedAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* m_sChildProperty, const char* szNameProperty);
  ~plQtNamedAdapter();
  virtual QVariant data(const plDocumentObject* pObject, int row, int column, int role) const override;

protected:
  virtual void TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e);

protected:
  plString m_sNameProperty;
};

/// \brief Convenience class that implements setting the name via a property on the object.
class PLASMA_EDITORFRAMEWORK_DLL plQtNameableAdapter : public plQtNamedAdapter
{
  Q_OBJECT;

public:
  plQtNameableAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* m_sChildProperty, const char* szNameProperty);
  ~plQtNameableAdapter();
  virtual bool setData(const plDocumentObject* pObject, int row, int column, const QVariant& value, int role) const override;
  virtual Qt::ItemFlags flags(const plDocumentObject* pObject, int row, int column) const override;
};

/// \brief Model that maps a document to a qt tree model.
///
/// Hierarchy is defined by plQtDocumentTreeModelAdapter that have to be added via AddAdapter.
class PLASMA_EDITORFRAMEWORK_DLL plQtDocumentTreeModel : public QAbstractItemModel
{
public:
  plQtDocumentTreeModel(const plDocumentObjectManager* pTree, const plUuid& root = plUuid());
  ~plQtDocumentTreeModel();

  const plDocumentObjectManager* GetDocumentTree() const { return m_pDocumentTree; }
  /// \brief Adds an adapter. There can only be one adapter for any object type.
  /// Added adapters are taken ownership of by the model.
  void AddAdapter(plQtDocumentTreeModelAdapter* adapter);
  /// \brief Returns the QModelIndex for the given object.
  /// Returned value is invalid if object is not mapped in model.
  QModelIndex ComputeModelIndex(const plDocumentObject* pObject) const;

  /// \brief Enable drag&drop support, disabled by default.
  void SetAllowDragDrop(bool bAllow);

  static bool MoveObjects(const plDragDropInfo& info);

public: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& child) const override;

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  virtual Qt::DropActions supportedDropActions() const override;

  virtual bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

protected:
  virtual void TreeEventHandler(const plDocumentObjectStructureEvent& e);

private:
  QModelIndex ComputeParent(const plDocumentObject* pObject) const;
  plInt32 ComputeIndex(const plDocumentObject* pObject) const;
  const plDocumentObject* GetRoot() const;
  bool IsUnderRoot(const plDocumentObject* pObject) const;

  const plQtDocumentTreeModelAdapter* GetAdapter(const plRTTI* pType) const;

protected:
  const plDocumentObjectManager* m_pDocumentTree = nullptr;
  const plUuid m_Root;
  plHashTable<const plRTTI*, plQtDocumentTreeModelAdapter*> m_Adapters;
  bool m_bAllowDragDrop = false;
  plString m_sTargetContext = "scenetree";
};

