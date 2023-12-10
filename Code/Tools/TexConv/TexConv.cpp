#include <TexConv/TexConvPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/plTexFormat/plTexFormat.h>

plTexConv::plTexConv()
  : plApplication("TexConv")
{
}

plResult plTexConv::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void plTexConv::AfterCoreSystemsStartup()
{
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
}

void plTexConv::BeforeCoreSystemsShutdown()
{
  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

plResult plTexConv::DetectOutputFormat()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = plTexConvOutputType::None;
    return PLASMA_SUCCESS;
  }

  plStringBuilder sExt = plPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = true;
    return PLASMA_SUCCESS;
  }
  if (sExt == "TGA" || sExt == "PNG")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return PLASMA_SUCCESS;
  }
  if (sExt == "PLTEXTURE2D")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return PLASMA_SUCCESS;
  }
  if (sExt == "PLTEXTURE3D")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return PLASMA_SUCCESS;
  }
  if (sExt == "PLTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return PLASMA_SUCCESS;
  }
  if (sExt == "PLTEXTUREATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = true;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return PLASMA_SUCCESS;
  }
  if (sExt == "PLIMAGEDATA")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return PLASMA_SUCCESS;
  }

  plLog::Error("Output file uses unsupported file format '{}'", sExt);
  return PLASMA_FAILURE;
}

bool plTexConv::IsTexFormat() const
{
  const plStringView ext = plPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("pl");
}

plResult plTexConv::WriteTexFile(plStreamWriter& stream, const plImage& image)
{
  plAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  PLASMA_SUCCEED_OR_RETURN(asset.Write(stream));

  plTexFormat texFormat;
  texFormat.m_bSRGB = plImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(stream);

  plDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(stream, image, "dds").Failed())
  {
    plLog::Error("Failed to write DDS image chunk to plTex file.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plTexConv::WriteOutputFile(plStringView sFile, const plImage& image)
{
  if (sFile.HasExtension("plImageData"))
  {
    plDeferredFileWriter file;
    file.SetOutput(sFile);

    plAssetFileHeader asset;
    asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    if (asset.Write(file).Failed())
    {
      plLog::Error("Failed to write asset header to file.");
      return PLASMA_FAILURE;
    }

    plUInt8 uiVersion = 1;
    file << uiVersion;

    plUInt8 uiFormat = 1; // 1 == PNG
    file << uiFormat;

    plStbImageFileFormats pngWriter;
    if (pngWriter.WriteImage(file, image, "png").Failed())
    {
      plLog::Error("Failed to write data as PNG to plImageData file.");
      return PLASMA_FAILURE;
    }

    return file.Close();
  }
  else if (IsTexFormat())
  {
    plDeferredFileWriter file;
    file.SetOutput(sFile);

    PLASMA_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(sFile);
  }
}

plApplication::Execution plTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
    return plApplication::Execution::Quit;

  if (m_Processor.Process().Failed())
    return plApplication::Execution::Quit;

  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
  {
    plDeferredFileWriter file;
    file.SetOutput(m_sOutputFile);

    plAssetFileHeader header;
    header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    header.Write(file).IgnoreResult();

    m_Processor.m_TextureAtlas.CopyToStream(file).IgnoreResult();

    if (file.Close().Succeeded())
    {
      SetReturnCode(0);
    }
    else
    {
      plLog::Error("Failed to write atlas output image.");
    }

    return plApplication::Execution::Quit;
  }

  if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
  {
    if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
    {
      plLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return plApplication::Execution::Quit;
    }

    plLog::Success("Wrote main result to '{}'", m_sOutputFile);
  }

  if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      plLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return plApplication::Execution::Quit;
    }

    plLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
  }

  if (!m_sOutputLowResFile.IsEmpty())
  {
    // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
    plOSFile::DeleteFile(m_sOutputLowResFile).IgnoreResult();

    if (m_Processor.m_LowResOutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
      {
        plLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
        return plApplication::Execution::Quit;
      }

      plLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
    }
  }

  SetReturnCode(0);
  return plApplication::Execution::Quit;
}

PLASMA_CONSOLEAPP_ENTRY_POINT(plTexConv);
