#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCollectionAssetEntry, 1, plRTTIDefaultAllocator<plCollectionAssetEntry>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sLookupName),
    PLASMA_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new plAssetBrowserAttribute("", plDependencyFlags::Package))
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCollectionAssetData, 1, plRTTIDefaultAllocator<plCollectionAssetData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCollectionAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCollectionAssetDocument::plCollectionAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plCollectionAssetData>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

static bool InsertEntry(plStringView sID, plStringView sLookupName, plMap<plString, plCollectionEntry>& inout_found)
{
  auto it = inout_found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return true;
  }

  plStringBuilder tmp;
  plAssetCurator::plLockedSubAsset pInfo = plAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    // this happens for non-asset types (e.g. 'xyz.color' and other non-asset file types)
    // these are benign and can just be skipped
    return false;
  }

  // insert item itself
  {
    plCollectionEntry& entry = inout_found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID = sID;
    entry.m_sAssetTypeName = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // insert dependencies
  {
    const plAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const plString& doc : pDocInfo->m_PackageDependencies)
    {
      // ignore return value, we are only interested in top-level information
      InsertEntry(doc, {}, inout_found);
    }
  }

  return true;
}

plTransformStatus plCollectionAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const plCollectionAssetData* pProp = GetProperties();

  plMap<plString, plCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    if (!InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries))
    {
      // this should be treated as an error for top-level references, since they are manually added (in contrast to the transitive dependencies)
      return plStatus(plFmt("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset));
    }
  }

  plCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return plStatus(PLASMA_SUCCESS);
}
