#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plTexture2DChannelMappingEnum, 1)
  PL_ENUM_CONSTANTS(plTexture2DChannelMappingEnum::R1)
  PL_ENUM_CONSTANTS(plTexture2DChannelMappingEnum::RG1, plTexture2DChannelMappingEnum::R1_G2)
  PL_ENUM_CONSTANTS(plTexture2DChannelMappingEnum::RGB1, plTexture2DChannelMappingEnum::RGB1_ABLACK, plTexture2DChannelMappingEnum::R1_G2_B3)
  PL_ENUM_CONSTANTS(plTexture2DChannelMappingEnum::RGBA1, plTexture2DChannelMappingEnum::RGB1_A2, plTexture2DChannelMappingEnum::R1_G2_B3_A4)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plTexture2DResolution, 1)
  PL_ENUM_CONSTANTS(plTexture2DResolution::Fixed64x64, plTexture2DResolution::Fixed128x128, plTexture2DResolution::Fixed256x256, plTexture2DResolution::Fixed512x512, plTexture2DResolution::Fixed1024x1024, plTexture2DResolution::Fixed2048x2048)
  PL_ENUM_CONSTANTS(plTexture2DResolution::CVarRtResolution1, plTexture2DResolution::CVarRtResolution2)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plRenderTargetFormat, 1)
  PL_ENUM_CONSTANTS(plRenderTargetFormat::RGBA8sRgb, plRenderTargetFormat::RGBA8, plRenderTargetFormat::RGB10, plRenderTargetFormat::RGBA16)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureAssetProperties, 5, plRTTIDefaultAllocator<plTextureAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("IsRenderTarget", m_bIsRenderTarget)->AddAttributes(new plHiddenAttribute),
    PL_ENUM_MEMBER_PROPERTY("Usage", plTexConvUsage, m_TextureUsage),

    PL_ENUM_MEMBER_PROPERTY("Format", plRenderTargetFormat, m_RtFormat),
    PL_ENUM_MEMBER_PROPERTY("Resolution", plTexture2DResolution, m_Resolution),
    PL_MEMBER_PROPERTY("CVarResScale", m_fCVarResolutionScale)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, 10.0f)),

    PL_ENUM_MEMBER_PROPERTY("MipmapMode", plTexConvMipmapMode, m_MipmapMode),
    PL_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    PL_MEMBER_PROPERTY("AlphaThreshold", m_fAlphaThreshold)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_ENUM_MEMBER_PROPERTY("CompressionMode", plTexConvCompressionMode, m_CompressionMode),
    PL_MEMBER_PROPERTY("PremultipliedAlpha", m_bPremultipliedAlpha),
    PL_MEMBER_PROPERTY("DilateColor", m_bDilateColor)->AddAttributes(new plDefaultValueAttribute(false)),
    PL_MEMBER_PROPERTY("FlipHorizontal", m_bFlipHorizontal),
    PL_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new plClampValueAttribute(-20.0f, 20.0f)),

    PL_ENUM_MEMBER_PROPERTY("TextureFilter", plTextureFilterSetting, m_TextureFilter),
    PL_ENUM_MEMBER_PROPERTY("AddressModeU", plImageAddressMode, m_AddressModeU),
    PL_ENUM_MEMBER_PROPERTY("AddressModeV", plImageAddressMode, m_AddressModeV),
    PL_ENUM_MEMBER_PROPERTY("AddressModeW", plImageAddressMode, m_AddressModeW),

    PL_ENUM_MEMBER_PROPERTY("ChannelMapping", plTexture2DChannelMappingEnum, m_ChannelMapping),

    PL_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PL_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PL_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PL_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),

  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plTextureAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plTextureAssetProperties>())
  {
    auto& props = *e.m_pPropertyStates;

    const bool isRenderTarget = e.m_pObject->GetTypeAccessor().GetValue("IsRenderTarget").ConvertTo<bool>();

    props["AddressModeW"].m_Visibility = plPropertyUiState::Invisible;

    if (isRenderTarget)
    {
      const plInt32 resMode = e.m_pObject->GetTypeAccessor().GetValue("Resolution").ConvertTo<plInt32>();
      const bool resIsCVar = resMode == plTexture2DResolution::CVarRtResolution1 || resMode == plTexture2DResolution::CVarRtResolution2;

      props["CVarResScale"].m_Visibility = resIsCVar ? plPropertyUiState::Default : plPropertyUiState::Disabled;
      props["Usage"].m_Visibility = plPropertyUiState::Invisible;
      props["MipmapMode"].m_Visibility = plPropertyUiState::Invisible;
      props["CompressionMode"].m_Visibility = plPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = plPropertyUiState::Invisible;
      props["FlipHorizontal"].m_Visibility = plPropertyUiState::Invisible;
      props["ChannelMapping"].m_Visibility = plPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = plPropertyUiState::Invisible;
      props["AlphaThreshold"].m_Visibility = plPropertyUiState::Invisible;
      props["PremultipliedAlpha"].m_Visibility = plPropertyUiState::Invisible;
      props["HdrExposureBias"].m_Visibility = plPropertyUiState::Invisible;
      props["DilateColor"].m_Visibility = plPropertyUiState::Invisible;

      props["Input1"].m_Visibility = plPropertyUiState::Invisible;
      props["Input2"].m_Visibility = plPropertyUiState::Invisible;
      props["Input3"].m_Visibility = plPropertyUiState::Invisible;
      props["Input4"].m_Visibility = plPropertyUiState::Invisible;

      props["Format"].m_Visibility = plPropertyUiState::Default;
      props["Resolution"].m_Visibility = plPropertyUiState::Default;
    }
    else
    {
      const bool hasMips = e.m_pObject->GetTypeAccessor().GetValue("MipmapMode").ConvertTo<plInt32>() != plTexConvMipmapMode::None;
      const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<plInt32>() == plTexConvUsage::Hdr;

      props["CVarResScale"].m_Visibility = plPropertyUiState::Invisible;
      props["Usage"].m_Visibility = plPropertyUiState::Default;
      props["Mipmaps"].m_Visibility = plPropertyUiState::Default;
      props["Compression"].m_Visibility = plPropertyUiState::Default;
      props["PremultipliedAlpha"].m_Visibility = plPropertyUiState::Disabled;
      props["FlipHorizontal"].m_Visibility = plPropertyUiState::Default;
      props["ChannelMapping"].m_Visibility = plPropertyUiState::Default;
      props["Format"].m_Visibility = plPropertyUiState::Invisible;
      props["Resolution"].m_Visibility = plPropertyUiState::Invisible;
      props["PreserveAlphaCoverage"].m_Visibility = plPropertyUiState::Disabled;
      props["AlphaThreshold"].m_Visibility = plPropertyUiState::Disabled;
      props["HdrExposureBias"].m_Visibility = plPropertyUiState::Disabled;
      props["DilateColor"].m_Visibility = plPropertyUiState::Disabled;

      const plInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<plInt64>();

      props["Usage"].m_Visibility = plPropertyUiState::Default;
      props["Input1"].m_Visibility = plPropertyUiState::Default;
      props["Input2"].m_Visibility = plPropertyUiState::Invisible;
      props["Input3"].m_Visibility = plPropertyUiState::Invisible;
      props["Input4"].m_Visibility = plPropertyUiState::Invisible;

      {
        props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
        props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
        props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
        props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      }

      switch (mapping)
      {
        case plTexture2DChannelMappingEnum::R1_G2_B3_A4:
          props["Input4"].m_Visibility = plPropertyUiState::Default;
          // fall through

        case plTexture2DChannelMappingEnum::R1_G2_B3:
          props["Input3"].m_Visibility = plPropertyUiState::Default;
          // fall through

        case plTexture2DChannelMappingEnum::RGB1_A2:
        case plTexture2DChannelMappingEnum::R1_G2:
          props["Input2"].m_Visibility = plPropertyUiState::Default;
          break;
      }

      if (mapping == plTexture2DChannelMappingEnum::R1 || mapping == plTexture2DChannelMappingEnum::RGBA1 ||
          mapping == plTexture2DChannelMappingEnum::R1_G2_B3_A4 || mapping == plTexture2DChannelMappingEnum::RGB1_A2 ||
          mapping == plTexture2DChannelMappingEnum::R1_G2_B3_A4)
      {
        if (mapping != plTexture2DChannelMappingEnum::R1)
        {
          props["PremultipliedAlpha"].m_Visibility = plPropertyUiState::Default;
          props["DilateColor"].m_Visibility = plPropertyUiState::Default;
        }

        if (hasMips)
        {
          props["PreserveAlphaCoverage"].m_Visibility = plPropertyUiState::Default;
          props["AlphaThreshold"].m_Visibility = plPropertyUiState::Default;
        }
      }

      if (isHDR)
      {
        props["HdrExposureBias"].m_Visibility = plPropertyUiState::Default;
      }
    }

    // always hide this, feature may be removed at some point
    props["PremultipliedAlpha"].m_Visibility = plPropertyUiState::Invisible;
  }
}

plString plTextureAssetProperties::GetAbsoluteInputFilePath(plInt32 iInput) const
{
  plStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

plInt32 plTextureAssetProperties::GetNumInputFiles() const
{
  if (m_bIsRenderTarget)
    return 0;

  switch (m_ChannelMapping)
  {
    case plTexture2DChannelMappingEnum::R1:
    case plTexture2DChannelMappingEnum::RG1:
    case plTexture2DChannelMappingEnum::RGB1:
    case plTexture2DChannelMappingEnum::RGB1_ABLACK:
    case plTexture2DChannelMappingEnum::RGBA1:
      return 1;

    case plTexture2DChannelMappingEnum::R1_G2:
    case plTexture2DChannelMappingEnum::RGB1_A2:
      return 2;

    case plTexture2DChannelMappingEnum::R1_G2_B3:
      return 3;

    case plTexture2DChannelMappingEnum::R1_G2_B3_A4:
      return 4;
  }

  PL_REPORT_FAILURE("Invalid Code Path");
  return 1;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plTextureAssetPropertiesPatch_2_3 : public plGraphPatch
{
public:
  plTextureAssetPropertiesPatch_2_3()
    : plGraphPatch("plTextureAssetProperties", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pMipmaps = pNode->FindProperty("Mipmaps");
    if (pMipmaps && pMipmaps->m_Value.IsA<bool>())
    {
      if (pMipmaps->m_Value.Get<bool>())
        pNode->AddProperty("MipmapMode", (plInt32)plTexConvMipmapMode::Kaiser);
      else
        pNode->AddProperty("MipmapMode", (plInt32)plTexConvMipmapMode::None);
    }

    auto* pCompression = pNode->FindProperty("Compression");
    if (pCompression && pCompression->m_Value.IsA<bool>())
    {
      if (pCompression->m_Value.Get<bool>())
        pNode->AddProperty("CompressionMode", (plInt32)plTexConvCompressionMode::High);
      else
        pNode->AddProperty("CompressionMode", (plInt32)plTexConvCompressionMode::None);
    }
  }
};

plTextureAssetPropertiesPatch_2_3 g_plTextureAssetPropertiesPatch_2_3;

//////////////////////////////////////////////////////////////////////////

class plTextureAssetPropertiesPatch_3_4 : public plGraphPatch
{
public:
  plTextureAssetPropertiesPatch_3_4()
    : plGraphPatch("plTextureAssetProperties", 4)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    const char* szAddressModes[] = {"AddressModeU", "AddressModeV", "AddressModeW"};

    for (plUInt32 i = 0; i < 3; ++i)
    {
      auto* pAddress = pNode->FindProperty(szAddressModes[i]);
      if (pAddress && pAddress->m_Value.IsA<plString>())
      {
        if (pAddress->m_Value.Get<plString>() == "plTexture2DAddressMode::Wrap")
        {
          pNode->ChangeProperty(szAddressModes[i], (plInt32)plImageAddressMode::Repeat);
        }
        else if (pAddress->m_Value.Get<plString>() == "plTexture2DAddressMode::Clamp")
        {
          pNode->ChangeProperty(szAddressModes[i], (plInt32)plImageAddressMode::Clamp);
        }
        else if (pAddress->m_Value.Get<plString>() == "plTexture2DAddressMode::Mirror")
        {
          pNode->ChangeProperty(szAddressModes[i], (plInt32)plImageAddressMode::Mirror);
        }
      }
    }
  }
};

plTextureAssetPropertiesPatch_3_4 g_plTextureAssetPropertiesPatch_3_4;

//////////////////////////////////////////////////////////////////////////

class plTextureAssetPropertiesPatch_4_5 : public plGraphPatch
{
public:
  plTextureAssetPropertiesPatch_4_5()
    : plGraphPatch("plTextureAssetProperties", 5)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<plString>())
    {
      if (pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Diffuse" ||
               pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::EmissiveColor")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Height" || pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Mask" ||
               pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::LookupTable" ||
               pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::EmissiveMask")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::NormalMap")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::NormalMap);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTexture2DUsageEnum::HDR")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Hdr);
      }
    }
  }
};

plTextureAssetPropertiesPatch_4_5 g_plTextureAssetPropertiesPatch_4_5;
