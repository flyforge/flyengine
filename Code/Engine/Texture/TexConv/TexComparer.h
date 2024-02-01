#pragma once

/// \brief Input options for plTexComparer
class PL_TEXTURE_DLL plTexCompareDesc
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTexCompareDesc);

public:
  plTexCompareDesc() = default;

  /// Path to a file to load as a reference image. Optional, if m_ExpectedImage is already filled out.
  plString m_sExpectedFile;

  /// Path to a file to load as the input image. Optional, if m_ActualImage is already filled out.
  plString m_sActualFile;

  /// The reference image to compare. Ignored if m_sExpectedFile is filled out.
  plImage m_ExpectedImage;

  /// The image to compare. Ignored if m_sActualFile is filled out.
  plImage m_ActualImage;

  /// If enabled, the image comparison allows for more wiggle room.
  /// For images containing single-pixel rasterized lines.
  bool m_bRelaxedComparison = false;

  /// If the comparison yields a larger MSE than this, the images are considered to be too different.
  plUInt32 m_MeanSquareErrorThreshold = 100;
};

/// \brief Compares two images and generates various outputs.
class PL_TEXTURE_DLL plTexComparer
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTexComparer);

public:
  plTexComparer();

  /// The input data to compare.
  plTexCompareDesc m_Descriptor;

  /// Executes the comparison and fill out the public variables to describe the result.
  plResult Compare();

  /// If true, the mean-square error of the difference was larger than the threshold.
  bool m_bExceededMSE = false;
  /// The MSE of the difference image.
  plUInt32 m_OutputMSE = 0;

  /// The (normalized) difference image.
  plImage m_OutputImageDiff;
  /// Only the RGB part of the (normalized) difference image.
  plImage m_OutputImageDiffRgb;
  /// Only the Alpha part of the (normalized) difference image.
  plImage m_OutputImageDiffAlpha;

  /// Only the RGB part of the actual input image.
  plImage m_ExtractedActualRgb;
  /// Only the RGB part of the reference input image.
  plImage m_ExtractedExpectedRgb;
  /// Only the Alpha part of the actual input image.
  plImage m_ExtractedActualAlpha;
  /// Only the Alpha part of the reference input image.
  plImage m_ExtractedExpectedAlpha;

  /// Min/Max difference of the RGB and Alpha images.
  plUInt8 m_uiOutputMinDiffRgb = 0;
  plUInt8 m_uiOutputMaxDiffRgb = 0;
  plUInt8 m_uiOutputMinDiffAlpha = 0;
  plUInt8 m_uiOutputMaxDiffAlpha = 0;

private:
  plResult LoadInputImages();
  plResult ComputeMSE();
  plResult ExtractImages();
};
