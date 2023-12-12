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
  virtual QVariant data(const plDocumentObject* pObject, int row, int column, int role) const override;

private:
  const plSkeletonAssetDocument* m_pDocument;
};

