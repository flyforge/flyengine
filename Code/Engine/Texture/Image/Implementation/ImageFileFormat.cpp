#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plImageFileFormat);

plImageFileFormat* plImageFileFormat::GetReaderFormat(plStringView sExtension)
{
  for (plImageFileFormat* pFormat = plImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

plImageFileFormat* plImageFileFormat::GetWriterFormat(plStringView sExtension)
{
  for (plImageFileFormat* pFormat = plImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

plResult plImageFileFormat::ReadImageHeader(plStringView sFileName, plImageHeader& ref_header)
{
  PLASMA_LOG_BLOCK("Read Image Header", sFileName);

  PLASMA_PROFILE_SCOPE(plPathUtils::GetFileNameAndExtension(sFileName).GetStartPointer());

  plFileReader reader;
  if (reader.Open(sFileName) == PLASMA_FAILURE)
  {
    plLog::Warning("Failed to open image file '{0}'", plArgSensitive(sFileName, "File"));
    return PLASMA_FAILURE;
  }

  plStringView it = plPathUtils::GetFileExtension(sFileName);

  if (plImageFileFormat* pFormat = plImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it.GetStartPointer()) != PLASMA_SUCCESS)
    {
      plLog::Warning("Failed to read image file '{0}'", plArgSensitive(sFileName, "File"));
      return PLASMA_FAILURE;
    }

    return PLASMA_SUCCESS;
  }

  plLog::Warning("No known image file format for extension '{0}'", it);
  return PLASMA_FAILURE;
}

PLASMA_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);
