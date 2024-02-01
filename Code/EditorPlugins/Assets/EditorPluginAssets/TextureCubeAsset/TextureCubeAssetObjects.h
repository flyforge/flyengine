#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plPropertyMetaStateEvent;

struct plTextureCubeChannelMappingEnum
{
  using StorageType = plInt8;

  enum Enum
  {
    RGB1,
    RGBA1,

    RGB1TO6,
    RGBA1TO6,

    Default = RGB1,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plTextureCubeChannelMappingEnum);


class plTextureCubeAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plTextureCubeAssetProperties, plReflectedClass);

public:
  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  const char* GetInputFile(plInt32 iInput) const { return m_Input[iInput]; }

  void SetInputFile0(const char* szFile) { m_Input[0] = szFile; }
  const char* GetInputFile0() const { return m_Input[0]; }
  void SetInputFile1(const char* szFile) { m_Input[1] = szFile; }
  const char* GetInputFile1() const { return m_Input[1]; }
  void SetInputFile2(const char* szFile) { m_Input[2] = szFile; }
  const char* GetInputFile2() const { return m_Input[2]; }
  void SetInputFile3(const char* szFile) { m_Input[3] = szFile; }
  const char* GetInputFile3() const { return m_Input[3]; }
  void SetInputFile4(const char* szFile) { m_Input[4] = szFile; }
  const char* GetInputFile4() const { return m_Input[4]; }
  void SetInputFile5(const char* szFile) { m_Input[5] = szFile; }
  const char* GetInputFile5() const { return m_Input[5]; }

  plString GetAbsoluteInputFilePath(plInt32 iInput) const;
  plInt32 GetNumInputFiles() const;

  plEnum<plTexConvCompressionMode> m_CompressionMode;
  plEnum<plTexConvMipmapMode> m_MipmapMode;

  plEnum<plTextureFilterSetting> m_TextureFilter;
  plEnum<plTexConvUsage> m_TextureUsage;
  plEnum<plTextureCubeChannelMappingEnum> m_ChannelMapping;

  float m_fHdrExposureBias = 0;

private:
  plString m_Input[6];
};
