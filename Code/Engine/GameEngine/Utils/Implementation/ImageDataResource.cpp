#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Utils/ImageDataResource.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/plTexFormat/plTexFormat.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageDataResource, 1, plRTTIDefaultAllocator<plImageDataResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plImageDataResource);
// clang-format on

plImageDataResource::plImageDataResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plImageDataResource::~plImageDataResource() = default;

plResourceLoadDesc plImageDataResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plImageDataResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plImageDataResource::UpdateContent", GetResourceDescription().GetData());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  plStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  plImageDataResourceDescriptor desc;

  if (sAbsFilePath.HasExtension("plImageData"))
  {
    plAssetFileHeader AssetHash;
    if (AssetHash.Read(*Stream).Failed())
    {
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
    }

    plUInt8 uiVersion = 0;
    plUInt8 uiDataFormat = 0;

    *Stream >> uiVersion;
    *Stream >> uiDataFormat;

    if (uiVersion != 1 || uiDataFormat != 1)
    {
      plLog::Error("Unsupported plImageData file format or version");

      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
    }

    plStbImageFileFormats fmt;
    if (fmt.ReadImage(*Stream, desc.m_Image, "png").Failed())
    {
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
    }
  }
  else
  {
    plStringBuilder ext;
    ext = sAbsFilePath.GetFileExtension();

    if (plImageFileFormat::GetReaderFormat(ext)->ReadImage(*Stream, desc.m_Image, ext).Failed())
    {
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
    }
  }


  CreateResource(std::move(desc));

  res.m_State = plResourceState::Loaded;
  return res;
}

void plImageDataResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plImageDataResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += m_pDescriptor->m_Image.GetByteBlobPtr().GetCount();
  }
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plImageDataResource, plImageDataResourceDescriptor)
{
  m_pDescriptor = PLASMA_DEFAULT_NEW(plImageDataResourceDescriptor);

  *m_pDescriptor = std::move(descriptor);

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  if (m_pDescriptor->m_Image.Convert(plImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
  }

  return res;
}

// plResult plImageDataResourceDescriptor::Serialize(plStreamWriter& stream) const
//{
//  PLASMA_SUCCEED_OR_RETURN(plImageFileFormat::GetWriterFormat("png")->WriteImage(stream, m_Image, "png"));
//
//  return PLASMA_SUCCESS;
//}
//
// plResult plImageDataResourceDescriptor::Deserialize(plStreamReader& stream)
//{
//  PLASMA_SUCCEED_OR_RETURN(plImageFileFormat::GetReaderFormat("png")->ReadImage(stream, m_Image, "png"));
//
//  return PLASMA_SUCCESS;
//}
