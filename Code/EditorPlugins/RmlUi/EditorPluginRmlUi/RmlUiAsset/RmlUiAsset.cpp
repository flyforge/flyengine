#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>

plStringView FindRCSSReference(plStringView& ref_sRml)
{
  const char* szCurrent = ref_sRml.FindSubString("href");
  if (szCurrent == nullptr)
    return plStringView();

  const char* szStart = nullptr;
  const char* szEnd = nullptr;
  while (*szCurrent != '\0')
  {
    if (*szCurrent == '\"')
    {
      if (szStart == nullptr)
      {
        szStart = szCurrent + 1;
      }
      else
      {
        szEnd = szCurrent;
        break;
      }
    }

    ++szCurrent;
  }

  if (szStart != nullptr && szEnd != nullptr)
  {
    ref_sRml.SetStartPosition(szEnd);

    plStringView rcss = plStringView(szStart, szEnd);
    if (rcss.EndsWith_NoCase(".rcss"))
    {
      return rcss;
    }
  }

  return plStringView();
}

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plRmlUiAssetDocument::plRmlUiAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plRmlUiAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple)
{
}

plTransformStatus plRmlUiAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plRmlUiAssetProperties* pProp = GetProperties();

  plRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;
  desc.m_ScaleMode = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  // Find rcss dependencies
  {
    plStringBuilder sContent;
    {
      plFileReader reader;
      if (reader.Open(pProp->m_sRmlFile).Failed())
        return plStatus("Failed to read rml file");

      sContent.ReadAll(reader);
    }

    plStringBuilder sRmlFilePath = pProp->m_sRmlFile;
    sRmlFilePath = sRmlFilePath.GetFileDirectory();

    plStringView sContentView = sContent;

    while (true)
    {
      plStringView rcssReference = FindRCSSReference(sContentView);
      if (rcssReference.IsEmpty())
        break;

      plStringBuilder sRcssRef = rcssReference;
      if (!plFileSystem::ExistsFile(sRcssRef))
      {
        plStringBuilder sTemp;
        sTemp.AppendPath(sRmlFilePath, sRcssRef);
        sRcssRef = sTemp;
      }

      if (plFileSystem::ExistsFile(sRcssRef))
      {
        desc.m_DependencyFile.AddFileDependency(sRcssRef);
      }
      else
      {
        plLog::Warning("RCSS file '{}' was not added as dependency since it doesn't exist", sRcssRef);
      }
    }
  }

  PLASMA_SUCCEED_OR_RETURN(desc.Save(stream));

  return plStatus(PLASMA_SUCCESS);
}

plTransformStatus plRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
