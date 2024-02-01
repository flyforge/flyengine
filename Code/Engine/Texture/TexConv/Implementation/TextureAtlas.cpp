#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <Texture/Utils/TexturePacker.h>

plResult plTexConvProcessor::GenerateTextureAtlas(plMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != plTexConvOutputType::Atlas)
    return PL_SUCCESS;


  if (m_Descriptor.m_sTextureAtlasDescFile.IsEmpty())
  {
    plLog::Error("Texture atlas description file is not specified.");
    return PL_FAILURE;
  }

  plTextureAtlasCreationDesc atlasDesc;
  plDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sTextureAtlasDescFile).Failed())
  {
    plLog::Error("Failed to load texture atlas description '{0}'", plArgSensitive(m_Descriptor.m_sTextureAtlasDescFile, "File"));
    return PL_FAILURE;
  }

  m_Descriptor.m_uiMinResolution = plMath::Max(32u, m_Descriptor.m_uiMinResolution);

  PL_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const plUInt8 uiVersion = 3;
  stream << uiVersion;

  plDdsFileFormat ddsWriter;
  plImage atlasImg;

  for (plUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    PL_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg));

    if (ddsWriter.WriteImage(stream, atlasImg, "dds").Failed())
    {
      plLog::Error("Failed to write DDS image to texture atlas file.");
      return PL_FAILURE;
    }

    // debug: write out atlas slices as pure DDS
    if (false)
    {
      plStringBuilder sOut;
      sOut.SetFormat("D:/atlas_{}.dds", layerIdx);

      plFileWriter fOut;
      if (fOut.Open(sOut).Succeeded())
      {
        PL_SUCCEED_OR_RETURN(ddsWriter.WriteImage(fOut, atlasImg, "dds"));
      }
    }
  }

  PL_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, atlasDesc.m_Layers.GetCount(), stream));

  return PL_SUCCESS;
}

plResult plTexConvProcessor::LoadAtlasInputs(const plTextureAtlasCreationDesc& atlasDesc, plDynamicArray<TextureAtlasItem>& items) const
{
  items.Clear();

  for (const auto& srcItem : atlasDesc.m_Items)
  {
    auto& item = items.ExpandAndGetRef();
    item.m_uiUniqueID = srcItem.m_uiUniqueID;
    item.m_uiFlags = srcItem.m_uiFlags;

    for (plUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          plLog::Error("Failed to load texture atlas texture '{0}'", plArgSensitive(srcItem.m_sLayerInput[layer], "File"));
          return PL_FAILURE;
        }

        if (atlasDesc.m_Layers[layer].m_Usage == plTexConvUsage::Color)
        {
          // enforce sRGB format for all color textures
          item.m_InputImage[layer].ReinterpretAs(plImageFormat::AsSrgb(item.m_InputImage[layer].GetImageFormat()));
        }

        plUInt32 uiResX = 0, uiResY = 0;
        PL_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], plImageFormat::UNKNOWN, uiResX, uiResY));

        PL_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, atlasDesc.m_Layers[layer].m_Usage));
      }
    }


    if (!srcItem.m_sAlphaInput.IsEmpty())
    {
      plImage alphaImg;

      if (alphaImg.LoadFrom(srcItem.m_sAlphaInput).Failed())
      {
        plLog::Error("Failed to load texture atlas alpha mask '{0}'", srcItem.m_sAlphaInput);
        return PL_FAILURE;
      }

      plUInt32 uiResX = 0, uiResY = 0;
      PL_SUCCEED_OR_RETURN(DetermineTargetResolution(alphaImg, plImageFormat::UNKNOWN, uiResX, uiResY));

      PL_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sAlphaInput, alphaImg, uiResX, uiResY, plTexConvUsage::Linear));


      // layer 0 must have the exact same size as the alpha texture
      PL_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[0], item.m_InputImage[0], uiResX, uiResY, plTexConvUsage::Linear));

      // copy alpha channel into layer 0
      PL_SUCCEED_OR_RETURN(plImageUtils::CopyChannel(item.m_InputImage[0], 3, alphaImg, 0));

      // rescale all layers to be no larger than the alpha mask texture
      for (plUInt32 layer = 1; layer < atlasDesc.m_Layers.GetCount(); ++layer)
      {
        if (item.m_InputImage[layer].GetWidth() <= uiResX && item.m_InputImage[layer].GetHeight() <= uiResY)
          continue;

        PL_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, plTexConvUsage::Linear));
      }
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::WriteTextureAtlasInfo(const plDynamicArray<TextureAtlasItem>& atlasItems, plUInt32 uiNumLayers, plStreamWriter& stream)
{
  plTextureAtlasRuntimeDesc runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e = runtimeAtlas.m_Items[item.m_uiUniqueID];
    e.m_uiFlags = item.m_uiFlags;

    for (plUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

constexpr plUInt32 uiAtlasCellSize = 32;

plResult plTexConvProcessor::TrySortItemsIntoAtlas(plDynamicArray<TextureAtlasItem>& items, plUInt32 uiWidth, plUInt32 uiHeight, plInt32 layer)
{
  plTexturePacker packer;

  // TODO: review, currently the texture packer only works on 32 sized cells
  plUInt32 uiPixelAlign = uiAtlasCellSize;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign, (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  PL_SUCCEED_OR_RETURN(packer.PackTextures());

  plUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x = tex.m_Position.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].y = tex.m_Position.y * uiAtlasCellSize;
      item.m_AtlasRect[layer].width = tex.m_Size.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].height = tex.m_Size.y * uiAtlasCellSize;
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::SortItemsIntoAtlas(plDynamicArray<TextureAtlasItem>& items, plUInt32& out_ResX, plUInt32& out_ResY, plInt32 layer)
{
  for (plUInt32 power = 8; power < 14; ++power)
  {
    const plUInt32 halfRes = 1 << (power - 1);
    const plUInt32 resolution = 1 << power;
    const plUInt32 resDivCellSize = resolution / uiAtlasCellSize;
    const plUInt32 halfResDivCellSize = halfRes / uiAtlasCellSize;

    if (TrySortItemsIntoAtlas(items, resDivCellSize, halfResDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return PL_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return PL_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return PL_SUCCESS;
    }
  }

  plLog::Error("Could not sort items into texture atlas. Too many too large textures.");
  return PL_FAILURE;
}

plResult plTexConvProcessor::CreateAtlasTexture(plDynamicArray<TextureAtlasItem>& items, plUInt32 uiResX, plUInt32 uiResY, plImage& atlas, plInt32 layer)
{
  plImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(plImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<plUInt8>();
    plMemoryUtils::ZeroFill(pixelData.GetPtr(), static_cast<size_t>(pixelData.GetCount()));
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      plImage& itemImage = item.m_InputImage[layer];

      plRectU32 r;
      r.x = 0;
      r.y = 0;
      r.width = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      PL_SUCCEED_OR_RETURN(plImageUtils::Copy(itemImage, r, atlas, plVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0)));
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::FillAtlasBorders(plDynamicArray<TextureAtlasItem>& items, plImage& atlas, plInt32 layer)
{
  const plUInt32 uiBorderPixels = 2;

  const plUInt32 uiNumMipmaps = atlas.GetHeader().GetNumMipLevels();
  for (plUInt32 uiMipLevel = 0; uiMipLevel < uiNumMipmaps; ++uiMipLevel)
  {
    for (auto& item : items)
    {
      if (!item.m_InputImage[layer].IsValid())
        continue;

      plRectU32& itemRect = item.m_AtlasRect[layer];
      const plUInt32 uiRectX = itemRect.x >> uiMipLevel;
      const plUInt32 uiRectY = itemRect.y >> uiMipLevel;
      const plUInt32 uiWidth = plMath::Max(1u, itemRect.width >> uiMipLevel);
      const plUInt32 uiHeight = plMath::Max(1u, itemRect.height >> uiMipLevel);

      // fill the border of the item rect with alpha 0 to prevent bleeding into other decals in the atlas
      if (uiWidth <= 2 * uiBorderPixels || uiHeight <= 2 * uiBorderPixels)
      {
        for (plUInt32 y = 0; y < uiHeight; ++y)
        {
          for (plUInt32 x = 0; x < uiWidth; ++x)
          {
            const plUInt32 xClamped = plMath::Min(uiRectX + x, atlas.GetWidth(uiMipLevel));
            const plUInt32 yClamped = plMath::Min(uiRectY + y, atlas.GetHeight(uiMipLevel));
            atlas.GetPixelPointer<plColor>(uiMipLevel, 0, 0, xClamped, yClamped)->a = 0.0f;
          }
        }
      }
      else
      {
        for (plUInt32 i = 0; i < uiBorderPixels; ++i)
        {
          for (plUInt32 y = 0; y < uiHeight; ++y)
          {
            atlas.GetPixelPointer<plColor>(uiMipLevel, 0, 0, uiRectX + i, uiRectY + y)->a = 0.0f;
            atlas.GetPixelPointer<plColor>(uiMipLevel, 0, 0, uiRectX + uiWidth - 1 - i, uiRectY + y)->a = 0.0f;
          }

          for (plUInt32 x = 0; x < uiWidth; ++x)
          {
            atlas.GetPixelPointer<plColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + i)->a = 0.0f;
            atlas.GetPixelPointer<plColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + uiHeight - 1 - i)->a = 0.0f;
          }
        }
      }
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::CreateAtlasLayerTexture(const plTextureAtlasCreationDesc& atlasDesc, plDynamicArray<TextureAtlasItem>& atlasItems, plInt32 layer, plImage& dstImg)
{
  plUInt32 uiTexWidth, uiTexHeight;
  PL_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer));

  plLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  plImage atlasImg;
  PL_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  plUInt32 uiNumMipmaps = atlasImg.GetHeader().ComputeNumberOfMipMaps();
  PL_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  if (atlasDesc.m_Layers[layer].m_uiNumChannels == 4)
  {
    PL_SUCCEED_OR_RETURN(FillAtlasBorders(atlasItems, atlasImg, layer));
  }

  plEnum<plImageFormat> OutputImageFormat;

  PL_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  PL_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return PL_SUCCESS;
}


