#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

plQtJointAdapter::plQtJointAdapter(const plSkeletonAssetDocument* pDocument)
  : plQtNamedAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plEditableSkeletonJoint>(), "Children", "Name")
  , m_pDocument(pDocument)
{
}

plQtJointAdapter::~plQtJointAdapter() = default;

QVariant plQtJointAdapter::data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg"); // Giv ICon Plpl!
      return icon;
    }
    break;
  }
  return plQtNamedAdapter::data(pObject, iRow, iColumn, iRole);
}
