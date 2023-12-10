#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDocumentInfo, 2, plRTTIDefaultAllocator<plAssetDocumentInfo>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_SET_MEMBER_PROPERTY("Dependencies", m_TransformDependencies),
    PLASMA_SET_MEMBER_PROPERTY("References", m_ThumbnailDependencies),
    PLASMA_SET_MEMBER_PROPERTY("PackageDeps", m_PackageDependencies),
    PLASMA_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    PLASMA_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    PLASMA_ACCESSOR_PROPERTY("AssetType", GetAssetsDocumentTypeName, SetAssetsDocumentTypeName),
    PLASMA_ARRAY_MEMBER_PROPERTY("MetaInfo", m_MetaInfo)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAssetDocumentInfo::plAssetDocumentInfo()
{
  m_uiSettingsHash = 0;
}

plAssetDocumentInfo::~plAssetDocumentInfo()
{
  ClearMetaData();
}

plAssetDocumentInfo::plAssetDocumentInfo(plAssetDocumentInfo&& rhs)
{
  (*this) = std::move(rhs);
}

void plAssetDocumentInfo::operator=(plAssetDocumentInfo&& rhs)
{
  m_uiSettingsHash = rhs.m_uiSettingsHash;
  m_TransformDependencies = rhs.m_TransformDependencies;
  m_ThumbnailDependencies = rhs.m_ThumbnailDependencies;
  m_PackageDependencies = rhs.m_PackageDependencies;
  m_Outputs = rhs.m_Outputs;
  m_sAssetsDocumentTypeName = rhs.m_sAssetsDocumentTypeName;
  m_MetaInfo = std::move(rhs.m_MetaInfo);
}

void plAssetDocumentInfo::CreateShallowClone(plAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash = m_uiSettingsHash;
  rhs.m_TransformDependencies = m_TransformDependencies;
  rhs.m_ThumbnailDependencies = m_ThumbnailDependencies;
  rhs.m_PackageDependencies = m_PackageDependencies;
  rhs.m_Outputs = m_Outputs;
  rhs.m_sAssetsDocumentTypeName = m_sAssetsDocumentTypeName;
  rhs.m_MetaInfo.Clear();
}

void plAssetDocumentInfo::ClearMetaData()
{
  for (auto* pObj : m_MetaInfo)
  {
    pObj->GetDynamicRTTI()->GetAllocator()->Deallocate(pObj);
  }
  m_MetaInfo.Clear();
}

const char* plAssetDocumentInfo::GetAssetsDocumentTypeName() const
{
  return m_sAssetsDocumentTypeName.GetData();
}

void plAssetDocumentInfo::SetAssetsDocumentTypeName(const char* szSz)
{
  m_sAssetsDocumentTypeName.Assign(szSz);
}

const plReflectedClass* plAssetDocumentInfo::GetMetaInfo(const plRTTI* pType) const
{
  for (auto* pObj : m_MetaInfo)
  {
    if (pObj->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pObj;
  }
  return nullptr;
}
