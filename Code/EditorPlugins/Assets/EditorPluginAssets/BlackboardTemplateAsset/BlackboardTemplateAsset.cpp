#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateAssetObject, 1, plRTTIDefaultAllocator<plBlackboardTemplateAssetObject>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("BaseTemplates", m_BaseTemplates)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate", plDependencyFlags::Transform)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBlackboardTemplateAssetDocument::plBlackboardTemplateAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plBlackboardTemplateAssetObject>(szDocumentPath, plAssetDocEngineConnection::None)
{
}

plStatus plBlackboardTemplateAssetDocument::RetrieveState(const plBlackboardTemplateAssetObject* pProp, plBlackboardTemplateResourceDescriptor& inout_Desc)
{
  for (const plString& sTempl : pProp->m_BaseTemplates)
  {
    if (sTempl.IsEmpty())
      continue;

    auto pOther = plAssetCurator::GetSingleton()->FindSubAsset(sTempl);
    if (!pOther.isValid())
    {
      return plStatus(plFmt("Base template '{}' not found.", sTempl));
    }

    plDocument* pDoc;
    PLASMA_SUCCEED_OR_RETURN(pOther->m_pAssetInfo->GetManager()->OpenDocument(pOther->m_Data.m_sSubAssetsDocumentTypeName, pOther->m_pAssetInfo->m_sAbsolutePath, pDoc, plDocumentFlags::None, nullptr));

    if (plBlackboardTemplateAssetDocument* pTmpDoc = plDynamicCast<plBlackboardTemplateAssetDocument*>(pDoc))
    {
      PLASMA_SUCCEED_OR_RETURN(RetrieveState(pTmpDoc->GetProperties(), inout_Desc));
    }

    pOther->m_pAssetInfo->GetManager()->CloseDocument(pDoc);
  }

  for (const auto& e : pProp->m_Entries)
  {
    for (auto& e2 : inout_Desc.m_Entries)
    {
      if (e2.m_sName == e.m_sName)
      {
        e2 = e;
        goto next;
      }
    }

    inout_Desc.m_Entries.PushBack(e);

  next:;
  }

  return plStatus(PLASMA_SUCCESS);
}

plTransformStatus plBlackboardTemplateAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plBlackboardTemplateResourceDescriptor desc;
  const plStatus s = RetrieveState(GetProperties(), desc);
  if (s.Failed())
  {
    return plTransformStatus(s);
  }

  PLASMA_SUCCEED_OR_RETURN(desc.Serialize(stream));

  return plStatus(PLASMA_SUCCESS);
}
