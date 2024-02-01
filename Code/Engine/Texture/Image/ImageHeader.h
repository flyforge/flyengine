#pragma once

#include <Foundation/Basics/Assert.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>

#include <Texture/Image/ImageFormat.h>
#include <Texture/TextureDLL.h>

/// \brief A class containing image meta data, such as format and dimensions.
///
/// This class has no associated behavior or functionality, and its getters and setters have no effect other than changing
/// the contained value. It is intended as a container to be modified by image utils and loaders.
class PL_TEXTURE_DLL plImageHeader
{
public:
  /// \brief Constructs an image using an unknown format and zero size.
  plImageHeader() { Clear(); }

  /// \brief Constructs an image using an unknown format and zero size.
  void Clear()
  {
    m_uiNumMipLevels = 1;
    m_uiNumFaces = 1;
    m_uiNumArrayIndices = 1;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_uiDepth = 1;
    m_Format = plImageFormat::UNKNOWN;
  }

  /// \brief Sets the image format.
  void SetImageFormat(const plImageFormat::Enum& format) { m_Format = format; }

  /// \brief Returns the image format.
  plImageFormat::Enum GetImageFormat() const { return m_Format; }

  /// \brief Sets the image width.
  void SetWidth(plUInt32 uiWidth) { m_uiWidth = uiWidth; }

  /// \brief Returns the image width for a given mip level, clamped to 1.
  plUInt32 GetWidth(plUInt32 uiMipLevel = 0) const
  {
    PL_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return plMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  /// \brief Sets the image height.
  void SetHeight(plUInt32 uiHeight) { m_uiHeight = uiHeight; }

  /// \brief Returns the image height for a given mip level, clamped to 1.
  plUInt32 GetHeight(plUInt32 uiMipLevel = 0) const
  {
    PL_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return plMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  /// \brief Sets the image depth. The default is 1.
  void SetDepth(plUInt32 uiDepth) { m_uiDepth = uiDepth; }

  /// \brief Returns the image depth for a given mip level, clamped to 1.
  plUInt32 GetDepth(plUInt32 uiMipLevel = 0) const
  {
    PL_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return plMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  /// \brief Sets the number of mip levels, including the full-size image.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumMipLevels(plUInt32 uiNumMipLevels) { m_uiNumMipLevels = uiNumMipLevels; }

  /// \brief Returns the number of mip levels, including the full-size image.
  plUInt32 GetNumMipLevels() const { return m_uiNumMipLevels; }

  /// \brief Sets the number of cubemap faces. Use 1 for a non-cubemap.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumFaces(plUInt32 uiNumFaces) { m_uiNumFaces = uiNumFaces; }

  /// \brief Returns the number of cubemap faces, or 1 for a non-cubemap.
  plUInt32 GetNumFaces() const { return m_uiNumFaces; }

  /// \brief Sets the number of array indices.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumArrayIndices(plUInt32 uiNumArrayIndices) { m_uiNumArrayIndices = uiNumArrayIndices; }

  /// \brief Returns the number of array indices.
  plUInt32 GetNumArrayIndices() const { return m_uiNumArrayIndices; }

  /// \brief Returns the number of image planes.
  plUInt32 GetPlaneCount() const
  {
    return plImageFormat::GetPlaneCount(m_Format);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  plUInt32 GetNumBlocksX(plUInt32 uiMipLevel = 0, plUInt32 uiPlaneIndex = 0) const
  {
    return plImageFormat::GetNumBlocksX(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  plUInt32 GetNumBlocksY(plUInt32 uiMipLevel = 0, plUInt32 uiPlaneIndex = 0) const
  {
    return plImageFormat::GetNumBlocksY(m_Format, GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the depth direction.
  plUInt32 GetNumBlocksZ(plUInt32 uiMipLevel = 0, plUInt32 uiPlaneIndex = 0) const
  {
    return plImageFormat::GetNumBlocksZ(m_Format, GetDepth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
  plUInt64 GetRowPitch(plUInt32 uiMipLevel = 0, plUInt32 uiPlaneIndex = 0) const
  {
    return plImageFormat::GetRowPitch(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
  plUInt64 GetDepthPitch(plUInt32 uiMipLevel = 0, plUInt32 uiPlaneIndex = 0) const
  {
    return plImageFormat::GetDepthPitch(m_Format, GetWidth(uiMipLevel), GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Computes the data size required for an image with the header's format and dimensions.
  plUInt64 ComputeDataSize() const
  {
    plUInt64 uiDataSize = 0;

    for (plUInt32 uiMipLevel = 0; uiMipLevel < GetNumMipLevels(); uiMipLevel++)
    {
      for (plUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
      {
        uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * static_cast<plUInt64>(GetDepth(uiMipLevel));
      }
    }

    return plMath::SafeMultiply64(uiDataSize, plMath::SafeMultiply32(GetNumArrayIndices(), GetNumFaces()));
  }

  /// \brief Computes the number of mip maps in the full mip chain.
  plUInt32 ComputeNumberOfMipMaps() const
  {
    plUInt32 numMipMaps = 1;
    plUInt32 width = GetWidth();
    plUInt32 height = GetHeight();
    plUInt32 depth = GetDepth();

    while (width > 1 || height > 1 || depth > 1)
    {
      width = plMath::Max(1u, width / 2);
      height = plMath::Max(1u, height / 2);
      depth = plMath::Max(1u, depth / 2);

      numMipMaps++;
    }

    return numMipMaps;
  }

protected:
  plUInt32 m_uiNumMipLevels;
  plUInt32 m_uiNumFaces;
  plUInt32 m_uiNumArrayIndices;

  plUInt32 m_uiWidth;
  plUInt32 m_uiHeight;
  plUInt32 m_uiDepth;

  plImageFormat::Enum m_Format;
};
