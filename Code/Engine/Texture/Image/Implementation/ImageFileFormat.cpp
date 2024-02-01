#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plImageFileFormat);

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
  PL_LOG_BLOCK("Read Image Header", sFileName);

  PL_PROFILE_SCOPE(plPathUtils::GetFileNameAndExtension(sFileName).GetStartPointer());

  plFileReader reader;
  if (reader.Open(sFileName) == PL_FAILURE)
  {
    plLog::Warning("Failed to open image file '{0}'", plArgSensitive(sFileName, "File"));
    return PL_FAILURE;
  }

  plStringView it = plPathUtils::GetFileExtension(sFileName);

  if (plImageFileFormat* pFormat = plImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it.GetStartPointer()) != PL_SUCCESS)
    {
      plLog::Warning("Failed to read image file '{0}'", plArgSensitive(sFileName, "File"));
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  plLog::Warning("No known image file format for extension '{0}'", it);
  return PL_FAILURE;
}


