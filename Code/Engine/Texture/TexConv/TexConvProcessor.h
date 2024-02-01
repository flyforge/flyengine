#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

struct plTextureAtlasCreationDesc;

class PL_TEXTURE_DLL plTexConvProcessor
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTexConvProcessor);

public:
  plTexConvProcessor();

  plTexConvDesc m_Descriptor;

  plResult Process();

  plImage m_OutputImage;
  plImage m_LowResOutputImage;
  plImage m_ThumbnailOutputImage;
  plDefaultMemoryStreamStorage m_TextureAtlas;

private:
  //////////////////////////////////////////////////////////////////////////
  // Modifying the Descriptor

  plResult LoadInputImages();
  plResult ForceSRGBFormats();
  plResult ConvertAndScaleInputImages(plUInt32 uiResolutionX, plUInt32 uiResolutionY, plEnum<plTexConvUsage> usage);
  plResult ConvertToNormalMap(plImage& bumpMap) const;
  plResult ConvertToNormalMap(plArrayPtr<plImage> bumpMap) const;
  plResult ClampInputValues(plArrayPtr<plImage> images, float maxValue) const;
  plResult ClampInputValues(plImage& image, float maxValue) const;
  plResult DetectNumChannels(plArrayPtr<const plTexConvSliceChannelMapping> channelMapping, plUInt32& uiNumChannels);
  plResult InvertNormalMap(plImage& img);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  enum class MipmapChannelMode
  {
    AllChannels,
    SingleChannel
  };

  plResult ChooseOutputFormat(plEnum<plImageFormat>& out_Format, plEnum<plTexConvUsage> usage, plUInt32 uiNumChannels) const;
  plResult DetermineTargetResolution(
    const plImage& image, plEnum<plImageFormat> OutputImageFormat, plUInt32& out_uiTargetResolutionX, plUInt32& out_uiTargetResolutionY) const;
  plResult Assemble2DTexture(const plImageHeader& refImg, plImage& dst) const;
  plResult AssembleCubemap(plImage& dst) const;
  plResult Assemble3DTexture(plImage& dst) const;
  plResult AdjustHdrExposure(plImage& img) const;
  plResult PremultiplyAlpha(plImage& image) const;
  plResult DilateColor2D(plImage& img) const;
  plResult Assemble2DSlice(const plTexConvSliceChannelMapping& mapping, plUInt32 uiResolutionX, plUInt32 uiResolutionY, plColor* pPixelOut) const;
  plResult GenerateMipmaps(plImage& img, plUInt32 uiNumMips /* =0 */, MipmapChannelMode channelMode = MipmapChannelMode::AllChannels) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional
  static plResult AdjustUsage(plStringView sFilename, const plImage& srcImg, plEnum<plTexConvUsage>& inout_Usage);
  static plResult ConvertAndScaleImage(plStringView sImageName, plImage& inout_Image, plUInt32 uiResolutionX, plUInt32 uiResolutionY, plEnum<plTexConvUsage> usage);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static plResult GenerateOutput(plImage&& src, plImage& dst, plEnum<plImageFormat> format);
  static plResult GenerateThumbnailOutput(const plImage& srcImg, plImage& dstImg, plUInt32 uiTargetRes);
  static plResult GenerateLowResOutput(const plImage& srcImg, plImage& dstImg, plUInt32 uiLowResMip);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  struct TextureAtlasItem
  {
    plUInt32 m_uiUniqueID = 0;
    plUInt32 m_uiFlags = 0;
    plImage m_InputImage[4];
    plRectU32 m_AtlasRect[4];
  };

  plResult LoadAtlasInputs(const plTextureAtlasCreationDesc& atlasDesc, plDynamicArray<TextureAtlasItem>& items) const;
  plResult CreateAtlasLayerTexture(
    const plTextureAtlasCreationDesc& atlasDesc, plDynamicArray<TextureAtlasItem>& atlasItems, plInt32 layer, plImage& dstImg);

  static plResult WriteTextureAtlasInfo(const plDynamicArray<TextureAtlasItem>& atlasItems, plUInt32 uiNumLayers, plStreamWriter& stream);
  static plResult TrySortItemsIntoAtlas(plDynamicArray<TextureAtlasItem>& items, plUInt32 uiWidth, plUInt32 uiHeight, plInt32 layer);
  static plResult SortItemsIntoAtlas(plDynamicArray<TextureAtlasItem>& items, plUInt32& out_ResX, plUInt32& out_ResY, plInt32 layer);
  static plResult CreateAtlasTexture(plDynamicArray<TextureAtlasItem>& items, plUInt32 uiResX, plUInt32 uiResY, plImage& atlas, plInt32 layer);
  static plResult FillAtlasBorders(plDynamicArray<TextureAtlasItem>& items, plImage& atlas, plInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  plResult GenerateTextureAtlas(plMemoryStreamWriter& stream);
};
