#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/plTexFormat/plTexFormat.h>

static plTextureResourceLoader s_TextureResourceLoader;

plCVarFloat cvar_StreamingTextureLoadDelay("Streaming.TextureLoadDelay", 0.0f, plCVarFlags::Save, "Artificial texture loading slowdown");

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, TextureResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::SetResourceTypeLoader<plTexture2DResource>(&s_TextureResourceLoader);
    plResourceManager::SetResourceTypeLoader<plTexture3DResource>(&s_TextureResourceLoader);
    plResourceManager::SetResourceTypeLoader<plTextureCubeResource>(&s_TextureResourceLoader);
    plResourceManager::SetResourceTypeLoader<plRenderToTexture2DResource>(&s_TextureResourceLoader);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeLoader<plTexture2DResource>(nullptr);
    plResourceManager::SetResourceTypeLoader<plTexture3DResource>(nullptr);
    plResourceManager::SetResourceTypeLoader<plTextureCubeResource>(nullptr);
    plResourceManager::SetResourceTypeLoader<plRenderToTexture2DResource>(nullptr);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plResourceLoadData plTextureResourceLoader::OpenDataStream(const plResource* pResource)
{
  LoadedData* pData = PLASMA_DEFAULT_NEW(LoadedData);

  plResourceLoadData res;

  // Solid Color Textures
  if (plPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    plStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const plColorGammaUB color = plConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      plLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName);
    }

    pData->m_TexFormat.m_bSRGB = true;

    plImageHeader header;
    header.SetWidth(4);
    header.SetHeight(4);
    header.SetDepth(1);
    header.SetImageFormat(plImageFormat::R8G8B8A8_UNORM_SRGB);
    header.SetNumMipLevels(1);
    header.SetNumFaces(1);
    pData->m_Image.ResetAndAlloc(header);
    plUInt8* pPixels = pData->m_Image.GetPixelPointer<plUInt8>();

    for (plUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    plFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
      return res;

    const plStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
    res.m_sResourceDescription = File.GetFilePathRelative().GetView();

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
    {
      plFileStats stat;
      if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (plTextureXX format), this is a hack to get the SRGB information for the texture
    const plStringBuilder sName = plPathUtils::GetFileName(sAbsolutePath);
    pData->m_TexFormat.m_bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("plTexture2D") || sAbsolutePath.HasExtension("plTexture3D") || sAbsolutePath.HasExtension("plTextureCube") || sAbsolutePath.HasExtension("plRenderTarget") || sAbsolutePath.HasExtension("plLUT"))
    {
      if (LoadTexFile(File, *pData).Failed())
        return res;
    }
    else
    {
      // read whatever format, as long as plImage supports it
      File.Close();

      if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
        return res;

      if (pData->m_Image.GetImageFormat() == plImageFormat::B8G8R8_UNORM)
      {
        /// \todo A conversion to B8G8R8X8_UNORM currently fails

        plLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath);
        if (plImageConversion::Convert(pData->m_Image, pData->m_Image, plImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  plMemoryStreamWriter w(&pData->m_Storage);

  WriteTextureLoadStream(w, *pData);

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  if (cvar_StreamingTextureLoadDelay > 0)
  {
    plThreadUtils::Sleep(plTime::Seconds(cvar_StreamingTextureLoadDelay));
  }

  return res;
}

void plTextureResourceLoader::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  PLASMA_DEFAULT_DELETE(pData);
}

bool plTextureResourceLoader::IsResourceOutdated(const plResource* pResource) const
{
  // solid color textures are never outdated
  if (plPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

  // don't try to reload a file that cannot be found
  plStringBuilder sAbs;
  if (plFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    plFileStats stat;
    if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), plTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

plResult plTextureResourceLoader::LoadTexFile(plStreamReader& inout_stream, LoadedData& ref_data)
{
  // read the hash, ignore it
  plAssetFileHeader AssetHash;
  PLASMA_SUCCEED_OR_RETURN(AssetHash.Read(inout_stream));

  ref_data.m_TexFormat.ReadHeader(inout_stream);

  if (ref_data.m_TexFormat.m_iRenderTargetResolutionX == 0)
  {
    plDdsFileFormat fmt;
    return fmt.ReadImage(inout_stream, ref_data.m_Image, "dds");
  }
  else
  {
    return PLASMA_SUCCESS;
  }
}

void plTextureResourceLoader::WriteTextureLoadStream(plStreamWriter& w, const LoadedData& data)
{
  const plImage* pImage = &data.m_Image;
  w.WriteBytes(&pImage, sizeof(plImage*)).IgnoreResult();

  w << data.m_bIsFallback;
  data.m_TexFormat.WriteRenderTargetHeader(w);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureLoader);
