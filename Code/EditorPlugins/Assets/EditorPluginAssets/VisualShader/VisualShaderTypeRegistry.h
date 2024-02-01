#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plOpenDdlReaderElement;

struct plVisualShaderPinDescriptor
{
  plString m_sName;
  const plRTTI* m_pDataType = nullptr;
  plReflectedPropertyDescriptor m_PropertyDesc;
  plColorGammaUB m_Color = plColorScheme::DarkUI(plColorScheme::Gray);
  bool m_bExposeAsProperty = false;
  plString m_sDefaultValue;
  plDynamicArray<plString> m_sDefinesWhenUsingDefaultValue;
  plString m_sShaderCodeInline;
  plString m_sTooltip;
};

struct plVisualShaderNodeType
{
  using StorageType = plUInt8;

  enum Enum
  {
    Generic,
    Main,
    Texture,

    Default = Generic
  };
};

struct plVisualShaderNodeDescriptor
{
  plEnum<plVisualShaderNodeType> m_NodeType;
  plString m_sCfgFile; ///< from which config file this node type was loaded
  plString m_sName;
  plHashedString m_sCategory;
  plString m_sCheckPermutations;
  plColorGammaUB m_Color = plColorScheme::DarkUI(plColorScheme::Gray);
  plString m_sShaderCodePixelDefines;
  plString m_sShaderCodePixelIncludes;
  plString m_sShaderCodePixelSamplers;
  plString m_sShaderCodePixelConstants;
  plString m_sShaderCodePixelBody;
  plString m_sShaderCodePermutations;
  plString m_sShaderCodeMaterialParams;
  plString m_sShaderCodeMaterialCB;
  plString m_sShaderCodeRenderState;
  plString m_sShaderCodeVertexShader;
  plString m_sShaderCodeGeometryShader;

  plHybridArray<plVisualShaderPinDescriptor, 4> m_InputPins;
  plHybridArray<plVisualShaderPinDescriptor, 4> m_OutputPins;
  plHybridArray<plReflectedPropertyDescriptor, 4> m_Properties;
  plHybridArray<plInt8, 4> m_UniquePropertyValueGroups; // no property in the same group may share the same value, -1 for disabled
};


class plVisualShaderTypeRegistry
{
  PL_DECLARE_SINGLETON(plVisualShaderTypeRegistry);

public:
  plVisualShaderTypeRegistry();

  const plVisualShaderNodeDescriptor* GetDescriptorForType(const plRTTI* pRtti) const;

  const plRTTI* GetNodeBaseType() const { return m_pBaseType; }

  const plRTTI* GetPinSamplerType() const { return m_pSamplerPinType; }

  void UpdateNodeData();

  void UpdateNodeData(plStringView sCfgFileRelative);

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(EditorPluginAssets, VisualShader);

  void LoadNodeData();
  const plRTTI* GenerateTypeFromDesc(const plVisualShaderNodeDescriptor& desc);
  void LoadConfigFile(const char* szFile);

  void ExtractNodePins(
    const plOpenDdlReaderElement* pNode, const char* szPinType, plHybridArray<plVisualShaderPinDescriptor, 4>& pinArray, bool bOutput);
  void ExtractNodeProperties(const plOpenDdlReaderElement* pNode, plVisualShaderNodeDescriptor& nd);
  void ExtractNodeConfig(const plOpenDdlReaderElement* pNode, plVisualShaderNodeDescriptor& nd);


  plMap<const plRTTI*, plVisualShaderNodeDescriptor> m_NodeDescriptors;

  const plRTTI* m_pBaseType;
  const plRTTI* m_pSamplerPinType;
};
