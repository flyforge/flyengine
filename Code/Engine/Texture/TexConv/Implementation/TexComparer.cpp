#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexComparer.h>

plTexComparer::plTexComparer() = default;

plResult plTexComparer::Compare()
{
  PL_PROFILE_SCOPE("Compare");

  PL_SUCCEED_OR_RETURN(LoadInputImages());

  if ((m_Descriptor.m_ActualImage.GetWidth() != m_Descriptor.m_ExpectedImage.GetWidth()) ||
      (m_Descriptor.m_ActualImage.GetHeight() != m_Descriptor.m_ExpectedImage.GetHeight()))
  {
    plLog::Error("Image sizes are not identical: {}x{} != {}x{}", m_Descriptor.m_ActualImage.GetWidth(), m_Descriptor.m_ActualImage.GetHeight(), m_Descriptor.m_ExpectedImage.GetWidth(), m_Descriptor.m_ExpectedImage.GetHeight());
    return PL_FAILURE;
  }

  PL_SUCCEED_OR_RETURN(ComputeMSE());

  if (m_OutputMSE > m_Descriptor.m_MeanSquareErrorThreshold)
  {
    m_bExceededMSE = true;

    PL_SUCCEED_OR_RETURN(ExtractImages());
  }

  return PL_SUCCESS;
}

plResult plTexComparer::LoadInputImages()
{
  PL_PROFILE_SCOPE("Load Images");

  if (!m_Descriptor.m_sActualFile.IsEmpty())
  {
    if (m_Descriptor.m_ActualImage.LoadFrom(m_Descriptor.m_sActualFile).Failed())
    {
      plLog::Error("Could not load image file '{0}'.", plArgSensitive(m_Descriptor.m_sActualFile, "File"));
      return PL_FAILURE;
    }
  }

  if (!m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    if (m_Descriptor.m_ExpectedImage.LoadFrom(m_Descriptor.m_sExpectedFile).Failed())
    {
      plLog::Error("Could not load reference file '{0}'.", plArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
      return PL_FAILURE;
    }
  }

  if (!m_Descriptor.m_ActualImage.IsValid())
  {
    plLog::Error("No image available.");
    return PL_FAILURE;
  }

  if (!m_Descriptor.m_ExpectedImage.IsValid())
  {
    plLog::Error("No reference image available.");
    return PL_FAILURE;
  }

  if (m_Descriptor.m_ActualImage.GetImageFormat() == plImageFormat::UNKNOWN)
  {
    plLog::Error("Unknown image format for '{}'", plArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return PL_FAILURE;
  }

  if (m_Descriptor.m_ExpectedImage.GetImageFormat() == plImageFormat::UNKNOWN)
  {
    plLog::Error("Unknown image format for '{}'", plArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return PL_FAILURE;
  }

  if (plImageConversion::Convert(m_Descriptor.m_ActualImage, m_Descriptor.m_ActualImage, plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    plLog::Error("Could not convert to RGBA8: '{}'", plArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return PL_FAILURE;
  }

  if (plImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_Descriptor.m_ExpectedImage, plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    plLog::Error("Could not convert to RGBA8: '{}'", plArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plTexComparer::ComputeMSE()
{
  PL_PROFILE_SCOPE("ComputeMSE");

  if (m_Descriptor.m_bRelaxedComparison)
    plImageUtils::ComputeImageDifferenceABSRelaxed(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);
  else
    plImageUtils::ComputeImageDifferenceABS(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);

  m_OutputMSE = plImageUtils::ComputeMeanSquareError(m_OutputImageDiff, 32);

  return PL_SUCCESS;
}

plResult plTexComparer::ExtractImages()
{
  PL_PROFILE_SCOPE("ExtractImages");

  plImageUtils::Normalize(m_OutputImageDiff, m_uiOutputMinDiffRgb, m_uiOutputMaxDiffRgb, m_uiOutputMinDiffAlpha, m_uiOutputMaxDiffAlpha);

  PL_SUCCEED_OR_RETURN(plImageConversion::Convert(m_OutputImageDiff, m_OutputImageDiffRgb, plImageFormat::R8G8B8_UNORM));

  plImageUtils::ExtractAlphaChannel(m_OutputImageDiff, m_OutputImageDiffAlpha);

  PL_SUCCEED_OR_RETURN(plImageConversion::Convert(m_Descriptor.m_ActualImage, m_ExtractedActualRgb, plImageFormat::R8G8B8_UNORM));
  plImageUtils::ExtractAlphaChannel(m_Descriptor.m_ActualImage, m_ExtractedActualAlpha);

  PL_SUCCEED_OR_RETURN(plImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedRgb, plImageFormat::R8G8B8_UNORM));
  plImageUtils::ExtractAlphaChannel(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedAlpha);

  return PL_SUCCESS;
}
