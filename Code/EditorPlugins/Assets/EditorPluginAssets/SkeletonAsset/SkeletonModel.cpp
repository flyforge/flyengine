#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

plQtJointAdapter::plQtJointAdapter(const plSkeletonAssetDocument* pDocument)
  : plQtNamedAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plEditableSkeletonJoint>(), "Children", "Name")
  , m_pDocument(pDocument)
{
}

plQtJointAdapter::~plQtJointAdapter() {}

QVariant plQtJointAdapter::data(const plDocumentObject* pObject, int row, int column, int role) const
{
  switch (role)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/SkeletonBones.svg");
      return icon;
    }
    break;
  }
  return plQtNamedAdapter::data(pObject, row, column, role);
}
