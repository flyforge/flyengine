#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

class PL_TEXTURE_DLL plImageUtils
{
public:
  /// \brief Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const plImageView& imageA, const plImageView& imageB, plImage& out_difference);

  /// \brief Same as ComputeImageDifferenceABS, but for every pixel in imageA, the minimum diff in imageB is searched in a 1-pixel radius, allowing pixels in B to shift slightly without incurring a difference.
  static void ComputeImageDifferenceABSRelaxed(const plImageView& imageA, const plImageView& imageB, plImage& out_difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static plUInt32 ComputeMeanSquareError(const plImageView& differenceImage, plUInt8 uiBlockSize, plUInt32 uiOffsetx, plUInt32 uiOffsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static plUInt32 ComputeMeanSquareError(const plImageView& differenceImage, plUInt8 uiBlockSize);

  /// \brief Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(plImage& ref_image);
  static void Normalize(plImage& ref_image, plUInt8& ref_uiMinRgb, plUInt8& ref_uiMaxRgb, plUInt8& ref_uiMinAlpha, plUInt8& ref_uiMaxAlpha);

  /// \brief Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const plImageView& inputImage, plImage& ref_outputImage);

  /// \brief Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const plImageView& input, const plVec2I32& vOffset, const plSizeU32& newsize, plImage& ref_output);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(plImage& ref_image, plUInt32 uiMipLevel = 0, plUInt32 uiFace = 0, plUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static plResult Copy(const plImageView& srcImg, const plRectU32& srcRect, plImage& ref_dstImg, const plVec3U32& vDstOffset, plUInt32 uiDstMipLevel = 0,
    plUInt32 uiDstFace = 0, plUInt32 uiDstArrayIndex = 0);

  /// \brief Copies the lower uiNumMips data of a 2D image into another one.
  static plResult ExtractLowerMipChain(const plImageView& src, plImage& ref_dst, plUInt32 uiNumMips);

  /// Mip map generation options
  struct MipMapOptions
  {
    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const plImageFilter* m_filter = nullptr;

    /// Rescale RGB components to unit length.
    bool m_renormalizeNormals = false;

    /// If true, the alpha values are scaled to preserve the average coverage when alpha testing is enabled,
    bool m_preserveCoverage = false;

    /// The alpha test threshold to use when m_preserveCoverage == true.
    float m_alphaThreshold = 0.5f;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    plImageAddressMode::Enum m_addressModeU = plImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    plImageAddressMode::Enum m_addressModeV = plImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    plImageAddressMode::Enum m_addressModeW = plImageAddressMode::Clamp;

    /// The border color if texture address mode equals BORDER.
    plColor m_borderColor = plColor::Black;

    /// How many mip maps should be generated. Pass 0 to generate all mip map levels.
    plUInt32 m_numMipMaps = 0;
  };

  /// Scales the image.
  static plResult Scale(const plImageView& source, plImage& ref_target, plUInt32 uiWidth, plUInt32 uiHeight, const plImageFilter* pFilter = nullptr,
    plImageAddressMode::Enum addressModeU = plImageAddressMode::Clamp, plImageAddressMode::Enum addressModeV = plImageAddressMode::Clamp,
    const plColor& borderColor = plColor::Black);

  /// Scales the image.
  static plResult Scale3D(const plImageView& source, plImage& ref_target, plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiDepth,
    const plImageFilter* pFilter = nullptr, plImageAddressMode::Enum addressModeU = plImageAddressMode::Clamp,
    plImageAddressMode::Enum addressModeV = plImageAddressMode::Clamp, plImageAddressMode::Enum addressModeW = plImageAddressMode::Clamp,
    const plColor& borderColor = plColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in plImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const plImageView& source, plImage& ref_target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(plImage& ref_source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(plImage& ref_image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(plImage& ref_roughnessMap, const plImageView& normalMap);

  /// \brief Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(plImage& ref_image, float fBias);

  /// \brief Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static plResult CreateCubemapFromSingleFile(plImage& ref_dstImg, const plImageView& srcImg);

  /// \brief Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static plResult CreateCubemapFrom6Files(plImage& ref_dstImg, const plImageView* pSourceImages);

  static plResult CreateVolumeTextureFromSingleFile(plImage& ref_dstImg, const plImageView& srcImg);

  static plUInt32 GetSampleIndex(plUInt32 uiNumTexels, plInt32 iIndex, plImageAddressMode::Enum addressMode, bool& out_bUseBorderColor);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static plColor NearestSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// Prefer this function over the one that takes an plImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as plImage::GetPixelPointer() is rather slow due to validation overhead.
  static plColor NearestSample(const plColor* pPixelPointer, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static plColor BilinearSample(const plImageView& image, plImageAddressMode::Enum addressMode, plVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// Prefer this function over the one that takes an plImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as plImage::GetPixelPointer() is rather slow due to validation overhead.
  static plColor BilinearSample(const plColor* pPixelPointer, plUInt32 uiWidth, plUInt32 uiHeight, plImageAddressMode::Enum addressMode, plVec2 vUv);

  /// \brief Copies channel 0, 1, 2 or 3 from srcImg into dstImg.
  ///
  /// Currently only supports images of format R32G32B32A32_FLOAT and with identical resolution.
  /// Returns failure if any of those requirements are not met.
  static plResult CopyChannel(plImage& ref_dstImg, plUInt8 uiDstChannelIdx, const plImage& srcImg, plUInt8 uiSrcChannelIdx);

  /// \brief Embeds the image as Base64 encoded text into an HTML file.
  static void EmbedImageData(plStringBuilder& out_sHtml, const plImage& image);

  /// \brief Generates an HTML file containing the given images with mouse-over functionality to compare them.
  static void CreateImageDiffHtml(plStringBuilder& out_sHtml, plStringView sTitle, const plImage& referenceImgRgb, const plImage& referenceImgAlpha, const plImage& capturedImgRgb, const plImage& capturedImgAlpha, const plImage& diffImgRgb, const plImage& diffImgAlpha, plUInt32 uiError, plUInt32 uiThreshold, plUInt8 uiMinDiffRgb, plUInt8 uiMaxDiffRgb, plUInt8 uiMinDiffAlpha, plUInt8 uiMaxDiffAlpha);
};
