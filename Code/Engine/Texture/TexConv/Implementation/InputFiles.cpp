#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

plResult plTexConvProcessor::LoadInputImages()
{
  PL_PROFILE_SCOPE("Load Images");

  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    plLog::Error("No input images have been specified.");
    return PL_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    plLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return PL_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
  {
    // make sure the two arrays have the same size
    m_Descriptor.m_InputFiles.SetCount(m_Descriptor.m_InputImages.GetCount());

    plStringBuilder tmp;
    for (plUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
    {
      tmp.SetFormat("InputImage{}", plArgI(i, 2, true));
      m_Descriptor.m_InputFiles[i] = tmp;
    }
  }
  else
  {
    m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

    for (const auto& file : m_Descriptor.m_InputFiles)
    {
      auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
      if (img.LoadFrom(file).Failed())
      {
        plLog::Error("Could not load input file '{0}'.", plArgSensitive(file, "File"));
        return PL_FAILURE;
      }
    }
  }

  for (plUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == plImageFormat::UNKNOWN)
    {
      plLog::Error("Unknown image format for '{}'", plArgSensitive(m_Descriptor.m_InputFiles[i], "File"));
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::ConvertAndScaleImage(plStringView sImageName, plImage& inout_Image, plUInt32 uiResolutionX, plUInt32 uiResolutionY, plEnum<plTexConvUsage> usage)
{
  const bool bSingleChannel = plImageFormat::GetNumChannels(inout_Image.GetImageFormat()) == 1;

  if (inout_Image.Convert(plImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    plLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", sImageName);
    return PL_FAILURE;
  }

  // some scale operations fail when they are done in place, so use a scratch image as destination for now
  plImage scratch;
  if (plImageUtils::Scale(inout_Image, scratch, uiResolutionX, uiResolutionY, nullptr, plImageAddressMode::Clamp, plImageAddressMode::Clamp).Failed())
  {
    plLog::Error("Could not resize '{}' to {}x{}", sImageName, uiResolutionX, uiResolutionY);
    return PL_FAILURE;
  }

  inout_Image.ResetAndMove(std::move(scratch));

  if (usage == plTexConvUsage::Color && bSingleChannel)
  {
    // replicate single channel ("red" textures) into the other channels
    PL_SUCCEED_OR_RETURN(plImageUtils::CopyChannel(inout_Image, 1, inout_Image, 0));
    PL_SUCCEED_OR_RETURN(plImageUtils::CopyChannel(inout_Image, 2, inout_Image, 0));
  }

  return PL_SUCCESS;
}

plResult plTexConvProcessor::ConvertAndScaleInputImages(plUInt32 uiResolutionX, plUInt32 uiResolutionY, plEnum<plTexConvUsage> usage)
{
  PL_PROFILE_SCOPE("ConvertAndScaleInputImages");

  for (plUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];
    plStringView sName = m_Descriptor.m_InputFiles[idx];

    PL_SUCCEED_OR_RETURN(ConvertAndScaleImage(sName, img, uiResolutionX, uiResolutionY, usage));
  }

  return PL_SUCCESS;
}


