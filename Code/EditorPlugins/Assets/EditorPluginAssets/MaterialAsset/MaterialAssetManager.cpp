#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialAssetDocumentManager, 1, plRTTIDefaultAllocator<plMaterialAssetDocumentManager>)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

const char* const plMaterialAssetDocumentManager::s_szShaderOutputTag = "VISUAL_SHADER";

plMaterialAssetDocumentManager::plMaterialAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plMaterialAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  plAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Material", "plMaterial");

  m_DocTypeDesc.m_sDocumentTypeName = "Material";
  m_DocTypeDesc.m_sFileExtension = "plMaterialAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Material.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plMaterialAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Material");

  m_DocTypeDesc.m_sResourceFileExtension = "plMaterialBin";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plMaterialAssetDocumentManager::~plMaterialAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

plString plMaterialAssetDocumentManager::GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const plPlatformProfile* pAssetProfile) const
{
  if (plStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    plStringBuilder sRelativePath(szDocumentPath);
    sRelativePath.MakeRelativeTo(szDataDirectory).IgnoreResult();
    plAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "autogen.plShader", false);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, szDataDirectory, szDocumentPath, szOutputTag, pAssetProfile);
}


bool plMaterialAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  if (plStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    const plString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, szDocumentPath, szOutputTag);

    plStringBuilder sExpectedHeader;
    sExpectedHeader.Format("//{0}|{1}\n", uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());

    plFileReader file;
    if (file.Open(sTargetFile, 256).Failed())
      return false;

    // this might happen if writing to the file failed
    if (file.GetFileSize() < sExpectedHeader.GetElementCount())
      return false;

    plUInt8 Temp[256] = {0};
    const plUInt32 uiRead = (plUInt32)file.ReadBytes(Temp, sExpectedHeader.GetElementCount());
    plStringBuilder sFileHeader = plStringView((const char*)&Temp[0], (const char*)&Temp[uiRead]);

    return sFileHeader.IsEqual(sExpectedHeader);
  }

  return plAssetDocumentManager::IsOutputUpToDate(szDocumentPath, szOutputTag, uiHash, pTypeDescriptor);
}


void plMaterialAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plMaterialAssetDocument>())
      {
        plQtMaterialAssetDocumentWindow* pDocWnd = new plQtMaterialAssetDocumentWindow(static_cast<plMaterialAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plMaterialAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plMaterialAssetDocument(szPath);
}

void plMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
