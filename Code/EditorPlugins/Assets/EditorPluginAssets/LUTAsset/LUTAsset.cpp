#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>



#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/plTexFormat/plTexFormat.h>



// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLUTAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLUTAssetDocument::plLUTAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plLUTAssetProperties>(szDocumentPath, plAssetDocEngineConnection::None)
{
}

plTransformStatus plLUTAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const auto props = GetProperties();

  // Read CUBE file, convert to 3D texture and write to file
  plFileStats Stats;
  bool bStat = plOSFile::GetFileStats(props->GetAbsoluteInputFilePath(), Stats).Succeeded();

  plStringBuilder inputPath = props->GetAbsoluteInputFilePath();

  plFileReader cubeFile;
  if (!bStat || cubeFile.Open(inputPath).Failed())
  {
    return plStatus(plFmt("Couldn't open CUBE file '{0}'.", inputPath));
  }

  plImage img;
  bool isDDS = false;
  if (inputPath.GetFileExtension() == "cube")
  {
    plAdobeCUBEReader cubeReader;
    auto parseRes = cubeReader.ParseFile(cubeFile);
    if (parseRes.Failed())
      return parseRes;

    const plUInt32 lutSize = cubeReader.GetLUTSize();

    // Build an plImage from the data
    plImageHeader imgHeader;
    imgHeader.SetImageFormat(plImageFormat::R8G8B8A8_UNORM_SRGB);
    imgHeader.SetWidth(lutSize);
    imgHeader.SetHeight(lutSize);
    imgHeader.SetDepth(lutSize);

    img.ResetAndAlloc(imgHeader);

    if (!img.IsValid())
    {
      return plStatus("Allocated plImage for LUT data is not valid.");
    }



    for (plUInt32 b = 0; b < lutSize; ++b)
    {
      for (plUInt32 g = 0; g < lutSize; ++g)
      {
        for (plUInt32 r = 0; r < lutSize; ++r)
        {
          const plVec3 val = cubeReader.GetLUTEntry(r, g, b);

          plColor col(val.x, val.y, val.z);
          plColorGammaUB colUb(col);

          plColorGammaUB* pPixel = img.GetPixelPointer<plColorGammaUB>(0, 0, 0, r, g, b);

          *pPixel = colUb;
        }
      }
    }
  }
  else
  {
    isDDS = true;
    if(img.LoadFrom(inputPath).Failed())
    {
      return plStatus(plFmt("Failed to load dds file {0}", inputPath));
    }

    if(img.GetDepth() == 1)
    {
      return plStatus(plFmt("File {0} is not a 3D texture", inputPath));
    }
  }

  plDeferredFileWriter file;
  file.SetOutput(szTargetFile);
  PLASMA_SUCCEED_OR_RETURN(AssetHeader.Write(file));

  plTexFormat texFormat;
  if(isDDS)
  {
    texFormat.m_bSRGB = false;
    texFormat.m_AddressModeU = plImageAddressMode::Repeat;
    texFormat.m_AddressModeV = plImageAddressMode::Repeat;
    texFormat.m_AddressModeW = plImageAddressMode::Repeat;
  }
  else
  {
    texFormat.m_bSRGB = true;
    texFormat.m_AddressModeU = plImageAddressMode::Clamp;
    texFormat.m_AddressModeV = plImageAddressMode::Clamp;
    texFormat.m_AddressModeW = plImageAddressMode::Clamp;
  }
  texFormat.m_TextureFilter = plTextureFilterSetting::FixedBilinear;

  texFormat.WriteTextureHeader(file);

  plDdsFileFormat fmt;
  if (fmt.WriteImage(file, img, "dds").Failed())
    return plStatus(plFmt("Writing image to target file failed: '{0}'", szTargetFile));

  if (file.Close().Failed())
    return plStatus(plFmt("Writing to target file failed: '{0}'", szTargetFile));

  return plStatus(PLASMA_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLUTAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plLUTAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plLUTAssetDocumentGenerator::plLUTAssetDocumentGenerator()
{
  AddSupportedFileType("cube");
  AddSupportedFileType("dds");
}

plLUTAssetDocumentGenerator::~plLUTAssetDocumentGenerator() = default;

void plLUTAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;

  const plStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  plAssetDocumentGenerator::Info& info = out_modes.ExpandAndGetRef();
  info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
  info.m_sOutputFileParentRelative = baseOutputFile;

  info.m_sName = "LUTImport.Cube";
  info.m_sIcon = ":/AssetIcons/LUT.svg";
}

plStatus plLUTAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plLUTAssetDocument* pAssetDoc = plDynamicCast<plLUTAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plLUTAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("Input", sDataDirRelativePath);

  return plStatus(PLASMA_SUCCESS);
}
