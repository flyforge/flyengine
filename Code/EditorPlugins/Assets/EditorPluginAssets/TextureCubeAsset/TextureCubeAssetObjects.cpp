#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plTextureCubeChannelMappingEnum, 1)
  PLASMA_ENUM_CONSTANTS(plTextureCubeChannelMappingEnum::RGB1, plTextureCubeChannelMappingEnum::RGBA1, plTextureCubeChannelMappingEnum::RGB1TO6, plTextureCubeChannelMappingEnum::RGBA1TO6)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetProperties, 3, plRTTIDefaultAllocator<plTextureCubeAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Usage", plTexConvUsage, m_TextureUsage),

    PLASMA_ENUM_MEMBER_PROPERTY("MipmapMode", plTexConvMipmapMode, m_MipmapMode),
    PLASMA_ENUM_MEMBER_PROPERTY("CompressionMode", plTexConvCompressionMode, m_CompressionMode),

    PLASMA_MEMBER_PROPERTY("HdrExposureBias", m_fHdrExposureBias)->AddAttributes(new plClampValueAttribute(-20.0f, 20.0f)),

    PLASMA_ENUM_MEMBER_PROPERTY("TextureFilter", plTextureFilterSetting, m_TextureFilter),

    PLASMA_ENUM_MEMBER_PROPERTY("ChannelMapping", plTextureCubeChannelMappingEnum, m_ChannelMapping),

    PLASMA_ACCESSOR_PROPERTY("Input1", GetInputFile0, SetInputFile0)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PLASMA_ACCESSOR_PROPERTY("Input2", GetInputFile1, SetInputFile1)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PLASMA_ACCESSOR_PROPERTY("Input3", GetInputFile2, SetInputFile2)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PLASMA_ACCESSOR_PROPERTY("Input4", GetInputFile3, SetInputFile3)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PLASMA_ACCESSOR_PROPERTY("Input5", GetInputFile4, SetInputFile4)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),
    PLASMA_ACCESSOR_PROPERTY("Input6", GetInputFile5, SetInputFile5)->AddAttributes(new plFileBrowserAttribute("Select Texture", plFileBrowserAttribute::ImagesLdrAndHdr)),

  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plTextureCubeAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plTextureCubeAssetProperties>())
  {
    const plInt64 mapping = e.m_pObject->GetTypeAccessor().GetValue("ChannelMapping").ConvertTo<plInt64>();
    const bool isHDR = e.m_pObject->GetTypeAccessor().GetValue("Usage").ConvertTo<plInt32>() == plTexConvUsage::Hdr;

    auto& props = *e.m_pPropertyStates;

    props["Usage"].m_Visibility = plPropertyUiState::Default;
    props["Input1"].m_Visibility = plPropertyUiState::Default;
    props["Input2"].m_Visibility = plPropertyUiState::Invisible;
    props["Input3"].m_Visibility = plPropertyUiState::Invisible;
    props["Input4"].m_Visibility = plPropertyUiState::Invisible;
    props["Input5"].m_Visibility = plPropertyUiState::Invisible;
    props["Input6"].m_Visibility = plPropertyUiState::Invisible;
    props["HdrExposureBias"].m_Visibility = plPropertyUiState::Disabled;

    if (isHDR)
    {
      props["HdrExposureBias"].m_Visibility = plPropertyUiState::Default;
    }

    if (mapping == plTextureCubeChannelMappingEnum::RGB1TO6 || mapping == plTextureCubeChannelMappingEnum::RGBA1TO6)
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::CM_Right";
      props["Input2"].m_sNewLabelText = "TextureAsset::CM_Left";
      props["Input3"].m_sNewLabelText = "TextureAsset::CM_Top";
      props["Input4"].m_sNewLabelText = "TextureAsset::CM_Bottom";
      props["Input5"].m_sNewLabelText = "TextureAsset::CM_Front";
      props["Input6"].m_sNewLabelText = "TextureAsset::CM_Back";
    }
    else
    {
      props["Input1"].m_sNewLabelText = "TextureAsset::Input1";
      props["Input2"].m_sNewLabelText = "TextureAsset::Input2";
      props["Input3"].m_sNewLabelText = "TextureAsset::Input3";
      props["Input4"].m_sNewLabelText = "TextureAsset::Input4";
      props["Input5"].m_sNewLabelText = "TextureAsset::Input5";
      props["Input6"].m_sNewLabelText = "TextureAsset::Input6";
    }

    switch (mapping)
    {
      case plTextureCubeChannelMappingEnum::RGB1TO6:
      case plTextureCubeChannelMappingEnum::RGBA1TO6:
        props["Input6"].m_Visibility = plPropertyUiState::Default;
        props["Input5"].m_Visibility = plPropertyUiState::Default;
        props["Input4"].m_Visibility = plPropertyUiState::Default;
        props["Input3"].m_Visibility = plPropertyUiState::Default;
        props["Input2"].m_Visibility = plPropertyUiState::Default;
        break;
    }
  }
}

plString plTextureCubeAssetProperties::GetAbsoluteInputFilePath(plInt32 iInput) const
{
  plStringBuilder sPath = m_Input[iInput];
  sPath.MakeCleanPath();

  if (!sPath.IsAbsolutePath())
  {
    plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
  }

  return sPath;
}

plInt32 plTextureCubeAssetProperties::GetNumInputFiles() const
{
  switch (m_ChannelMapping)
  {
    case plTextureCubeChannelMappingEnum::RGB1:
    case plTextureCubeChannelMappingEnum::RGBA1:
      return 1;

    case plTextureCubeChannelMappingEnum::RGB1TO6:
    case plTextureCubeChannelMappingEnum::RGBA1TO6:
      return 6;
  }

  PLASMA_REPORT_FAILURE("Invalid Code Path");
  return 1;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plTextureCubeAssetProperties_2_3 : public plGraphPatch
{
public:
  plTextureCubeAssetProperties_2_3()
    : plGraphPatch("plTextureCubeAssetProperties", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pUsage = pNode->FindProperty("Usage");
    if (pUsage && pUsage->m_Value.IsA<plString>())
    {
      if (pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::Unknown")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Auto);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::Other_sRGB" ||
               pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::Skybox")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Color);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::Other_Linear" ||
               pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::LookupTable")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Linear);
      }
      else if (pUsage->m_Value.Get<plString>() == "plTextureCubeUsageEnum::SkyboxHDR")
      {
        pNode->ChangeProperty("Usage", (plInt32)plTexConvUsage::Hdr);
      }
    }

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
        pNode->AddProperty("CompressionMode", (plInt32)plTexConvCompressionMode::Medium);
      else
        pNode->AddProperty("CompressionMode", (plInt32)plTexConvCompressionMode::None);
    }
  }
};

plTextureCubeAssetProperties_2_3 g_plTextureCubeAssetProperties_2_3;
