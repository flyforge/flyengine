#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

PL_DECLARE_FLAGS(plUInt8, plImageConversionFlags, InPlace);

/// A structure describing the pairs of source/target format that may be converted using the conversion routine.
struct plImageConversionEntry
{
  plImageConversionEntry(plImageFormat::Enum source, plImageFormat::Enum target, plImageConversionFlags::Enum flags)
    : m_sourceFormat(source)
    , m_targetFormat(target)
    , m_flags(flags)
  {
  }

  const plImageFormat::Enum m_sourceFormat;
  const plImageFormat::Enum m_targetFormat;
  const plBitflags<plImageConversionFlags> m_flags;

  /// This member adds an additional amount to the cost estimate for this conversion step.
  /// It can be used to bias the choice between steps when there are comparable conversion
  /// steps available.
  float m_additionalPenalty = 0.0f;
};

/// \brief Interface for a single image conversion step.
///
/// The actual functionality is implemented as either plImageConversionStepLinear or plImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class PL_TEXTURE_DLL plImageConversionStep : public plEnumerable<plImageConversionStep>
{
  PL_DECLARE_ENUMERABLE_CLASS(plImageConversionStep);

protected:
  plImageConversionStep();
  virtual ~plImageConversionStep();

public:
  /// \brief Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const = 0;
};

/// \brief Interface for a single image conversion step where both the source and target format are uncompressed.
class PL_TEXTURE_DLL plImageConversionStepLinear : public plImageConversionStep
{
public:
  /// \brief Converts a batch of pixels.
  virtual plResult ConvertPixels(plConstByteBlobPtr source, plByteBlobPtr target, plUInt64 uiNumElements, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class PL_TEXTURE_DLL plImageConversionStepDecompressBlocks : public plImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class PL_TEXTURE_DLL plImageConversionStepCompressBlocks : public plImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual plResult CompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocksX, plUInt32 uiNumBlocksY,
    plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a linear to a planar format.
class PL_TEXTURE_DLL plImageConversionStepPlanarize : public plImageConversionStep
{
public:
  /// \brief Converts a batch of pixels into the given target planes.
  virtual plResult ConvertPixels(const plImageView& source, plArrayPtr<plImage> target, plUInt32 uiNumPixelsX, plUInt32 uiNumPixelsY, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a planar to a linear format.
class PL_TEXTURE_DLL plImageConversionStepDeplanarize : public plImageConversionStep
{
public:
  /// \brief Converts a batch of pixels from the given source planes.
  virtual plResult ConvertPixels(plArrayPtr<plImageView> source, plImage target, plUInt32 uiNumPixelsX, plUInt32 uiNumPixelsY, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const = 0;
};


/// \brief Helper class containing utilities to convert between different image formats and layouts.
class PL_TEXTURE_DLL plImageConversion
{
public:
  /// \brief Checks if there is a known conversion path between the two formats
  static bool IsConvertible(plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  static plImageFormat::Enum FindClosestCompatibleFormat(plImageFormat::Enum format, plArrayPtr<const plImageFormat::Enum> compatibleFormats);

  /// \brief A single node along a computed conversion path.
  struct ConversionPathNode
  {
    PL_DECLARE_POD_TYPE();

    const plImageConversionStep* m_step;
    plImageFormat::Enum m_sourceFormat;
    plImageFormat::Enum m_targetFormat;
    plUInt32 m_sourceBufferIndex;
    plUInt32 m_targetBufferIndex;
    bool m_inPlace;
  };

  /// \brief Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain
  /// additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not
  ///                               the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work
  ///                               correctly when source and target are the same.
  /// \param path_out               The generated path.
  /// \param numScratchBuffers_out The number of scratch buffers required for the conversion path.
  /// \returns                      PL_SUCCESS if a path was found, PL_FAILURE otherwise.
  static plResult BuildPath(plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
    plHybridArray<ConversionPathNode, 16>& ref_path_out, plUInt32& ref_uiNumScratchBuffers_out);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static plResult Convert(const plImageView& source, plImage& ref_target, plImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static plResult Convert(const plImageView& source, plImage& ref_target, plArrayPtr<ConversionPathNode> path, plUInt32 uiNumScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static plResult ConvertRaw(
    plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumElements, plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static plResult ConvertRaw(
    plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumElements, plArrayPtr<ConversionPathNode> path, plUInt32 uiNumScratchBuffers);

private:
  plImageConversion();
  plImageConversion(const plImageConversion&);

  static plResult ConvertSingleStep(const plImageConversionStep* pStep, const plImageView& source, plImage& target, plImageFormat::Enum targetFormat);

  static plResult ConvertSingleStepDecompress(const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat, const plImageConversionStep* pStep);

  static plResult ConvertSingleStepCompress(const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat, const plImageConversionStep* pStep);

    static plResult ConvertSingleStepDeplanarize(const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat, const plImageConversionStep* pStep);

  static plResult ConvertSingleStepPlanarize(const plImageView& source, plImage& target, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat, const plImageConversionStep* pStep);

  static void RebuildConversionTable();
};
