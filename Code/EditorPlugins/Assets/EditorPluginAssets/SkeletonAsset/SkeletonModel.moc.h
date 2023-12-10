#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plSkeletonAssetDocument;

class plQtJointAdapter : public plQtNamedAdapter
{
  Q_OBJECT;

public:
  plQtJointAdapter(const plSkeletonAssetDocument* pDocument);
  ~plQtJointAdapter();
  virtual QVariant data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;

private:
  const plSkeletonAssetDocument* m_pDocument;
};

