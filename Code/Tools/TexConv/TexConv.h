#pragma once

#include <Foundation/Application/Application.h>

class plStreamWriter;

class plTexConv : public plApplication
{
public:
  typedef plApplication SUPER;

  struct KeyEnumValuePair
  {
    KeyEnumValuePair(plStringView sKey, plInt32 iVal)
      : m_sKey(sKey)
      , m_iEnumValue(iVal)
    {
    }

    plStringView m_sKey;
    plInt32 m_iEnumValue = -1;
  };

  plTexConv();

public:
  virtual Execution Run() override;
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  plResult ParseCommandLine();
  plResult ParseOutputType();
  plResult DetectOutputFormat();
  plResult ParseInputFiles();
  plResult ParseOutputFiles();
  plResult ParseChannelMappings();
  plResult ParseChannelSliceMapping(plInt32 iSlice);
  plResult ParseChannelMappingConfig(plTexConvChannelMapping& out_mapping, plStringView sCfg, plInt32 iChannelIndex, bool bSingleChannel);
  plResult ParseUsage();
  plResult ParseMipmapMode();
  plResult ParseTargetPlatform();
  plResult ParseCompressionMode();
  plResult ParseWrapModes();
  plResult ParseFilterModes();
  plResult ParseResolutionModifiers();
  plResult ParseMiscOptions();
  plResult ParseAssetHeader();
  plResult ParseBumpMapFilter();

  plResult ParseUIntOption(plStringView sOption, plInt32 iMinValue, plInt32 iMaxValue, plUInt32& ref_uiResult) const;
  plResult ParseStringOption(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed, plInt32& ref_iResult) const;
  void PrintOptionValues(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed) const;
  void PrintOptionValuesHelp(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed) const;
  bool ParseFile(plStringView sOption, plString& ref_sResult) const;


  bool IsTexFormat() const;
  plResult WriteTexFile(plStreamWriter& stream, const plImage& image);
  plResult WriteOutputFile(plStringView sFile, const plImage& image);

private:
  plString m_sOutputFile;
  plString m_sOutputThumbnailFile;
  plString m_sOutputLowResFile;

  bool m_bOutputSupports2D = false;
  bool m_bOutputSupports3D = false;
  bool m_bOutputSupportsCube = false;
  bool m_bOutputSupportsAtlas = false;
  bool m_bOutputSupportsMipmaps = false;
  bool m_bOutputSupportsFiltering = false;
  bool m_bOutputSupportsCompression = false;

  plTexConvProcessor m_Processor;
};
