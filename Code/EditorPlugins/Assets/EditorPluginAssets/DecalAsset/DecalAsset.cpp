#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plDecalMode, 1)
  PLASMA_ENUM_CONSTANT(plDecalMode::BaseColor),
  PLASMA_ENUM_CONSTANT(plDecalMode::BaseColorNormal),
  PLASMA_ENUM_CONSTANT(plDecalMode::BaseColorORM),
  PLASMA_ENUM_CONSTANT(plDecalMode::BaseColorNormalORM),
  PLASMA_ENUM_CONSTANT(plDecalMode::BaseColorEmissive)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetProperties, 3, plRTTIDefaultAllocator<plDecalAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plDecalMode, m_Mode),
    PLASMA_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    PLASMA_MEMBER_PROPERTY("AlphaMask", m_sAlphaMask)->AddAttributes(new plFileBrowserAttribute("Select Alpha Mask", plFileBrowserAttribute::ImagesLdrOnly)),
    PLASMA_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new plFileBrowserAttribute("Select Base Color Map", plFileBrowserAttribute::ImagesLdrOnly)),
    PLASMA_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new plFileBrowserAttribute("Select Normal Map", plFileBrowserAttribute::ImagesLdrOnly), new plDefaultValueAttribute(plStringView("Textures/NeutralNormal.tga"))), // wrap in plStringView to prevent a memory leak report
    PLASMA_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new plFileBrowserAttribute("Select ORM Map", plFileBrowserAttribute::ImagesLdrOnly)),
    PLASMA_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new plFileBrowserAttribute("Select Emissive Map", plFileBrowserAttribute::ImagesLdrOnly)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalAssetProperties::plDecalAssetProperties() = default;

void plDecalAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plDecalAssetProperties>())
  {
    plInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<plInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Normal"].m_Visibility = plPropertyUiState::Invisible;
    props["ORM"].m_Visibility = plPropertyUiState::Invisible;
    props["Emissive"].m_Visibility = plPropertyUiState::Invisible;

    if (mode == plDecalMode::BaseColorNormal)
    {
      props["Normal"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorORM)
    {
      props["ORM"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorNormalORM)
    {
      props["Normal"].m_Visibility = plPropertyUiState::Default;
      props["ORM"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorEmissive)
    {
      props["Emissive"].m_Visibility = plPropertyUiState::Default;
    }
  }
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetDocument, 5, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalAssetDocument::plDecalAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plDecalAssetProperties>(szDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

plTransformStatus plDecalAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  return static_cast<plDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

plTransformStatus plDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
{
  const plDecalAssetProperties* pProp = GetProperties();

  QStringList arguments;
  plStringBuilder temp;

  const plStringBuilder sThumbnail = GetThumbnailFilePath();

  arguments << "-usage";
  arguments << "Color";

  {
    // Thumbnail
    const plStringBuilder sDir = sThumbnail.GetFileDirectory();
    PLASMA_SUCCEED_OR_RETURN(plOSFile::CreateDirectoryStructure(sDir));

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
  }

  {
    plQtEditorApp* pEditorApp = plQtEditorApp::GetSingleton();

    temp.Format("-in0");

    plStringBuilder sAbsPath = pProp->m_sBaseColor;
    if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
    {
      return plStatus(plFmt("Failed to make path absolute: '{}'", sAbsPath));
    }

    arguments << temp.GetData();
    arguments << QString(sAbsPath.GetData());

    if (!pProp->m_sAlphaMask.IsEmpty())
    {
      plStringBuilder sAbsPath2 = pProp->m_sAlphaMask;
      if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath2))
      {
        return plStatus(plFmt("Failed to make path absolute: '{}'", sAbsPath2));
      }

      arguments << "-in1";
      arguments << QString(sAbsPath2.GetData());

      arguments << "-rgb";
      arguments << "in0.rgb";

      arguments << "-a";
      arguments << "in1.r";
    }
    else
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
  }

  PLASMA_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  {
    plUInt64 uiThumbnailHash = plAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    PLASMA_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return plStatus(PLASMA_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plDecalAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plDecalAssetDocumentGenerator::plDecalAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
}

plDecalAssetDocumentGenerator::~plDecalAssetDocumentGenerator() {}

void plDecalAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;

  const plStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  /// \todo Make this configurable
  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = isDecal ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "DecalImport.All";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Decal.svg";
  }
}

plStatus plDecalAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();
  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plDecalAssetDocument* pAssetDoc = plDynamicCast<plDecalAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plDecalAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", sDataDirRelativePath);

  return plStatus(PLASMA_SUCCESS);
}
