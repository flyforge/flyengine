#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateAssetObject, 1, plRTTIDefaultAllocator<plBlackboardTemplateAssetObject>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("BaseTemplates", m_BaseTemplates)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate", plDependencyFlags::Transform)),
    PL_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBlackboardTemplateAssetDocument::plBlackboardTemplateAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plBlackboardTemplateAssetObject>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

plStatus plBlackboardTemplateAssetDocument::WriteAsset(plStreamWriter& inout_stream, const plPlatformProfile* pAssetProfile) const
{
  plBlackboardTemplateResourceDescriptor desc;
  PL_SUCCEED_OR_RETURN(RetrieveState(GetProperties(), desc));
  PL_SUCCEED_OR_RETURN(desc.Serialize(inout_stream));

  return plStatus(PL_SUCCESS);
}

plStatus plBlackboardTemplateAssetDocument::RetrieveState(const plBlackboardTemplateAssetObject* pProp, plBlackboardTemplateResourceDescriptor& inout_Desc) const
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
    PL_SUCCEED_OR_RETURN(pOther->m_pAssetInfo->GetManager()->OpenDocument(pOther->m_Data.m_sSubAssetsDocumentTypeName, pOther->m_pAssetInfo->m_Path, pDoc, plDocumentFlags::None, nullptr));

    if (plBlackboardTemplateAssetDocument* pTmpDoc = plDynamicCast<plBlackboardTemplateAssetDocument*>(pDoc))
    {
      PL_SUCCEED_OR_RETURN(RetrieveState(pTmpDoc->GetProperties(), inout_Desc));
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

  return plStatus(PL_SUCCESS);
}

plTransformStatus plBlackboardTemplateAssetDocument::InternalTransformAsset(plStreamWriter& inout_stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  return WriteAsset(inout_stream, pAssetProfile);
}
