#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PLASMA_IMPLEMENT_SINGLETON(plVisualShaderTypeRegistry);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, VisualShader)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plVisualShaderTypeRegistry);

    plVisualShaderTypeRegistry::GetSingleton()->LoadNodeData();
    const plRTTI* pBaseType = plVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    plQtNodeScene::GetPinFactory().RegisterCreator(plGetStaticRTTI<plVisualShaderPin>(), [](const plRTTI* pRtti)->plQtPin* { return new plQtVisualShaderPin(); });
    plQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const plRTTI* pRtti)->plQtNode* { return new plQtVisualShaderNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const plRTTI* pBaseType = plVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

    plQtNodeScene::GetPinFactory().UnregisterCreator(plGetStaticRTTI<plVisualShaderPin>());
    plQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    plVisualShaderTypeRegistry* pDummy = plVisualShaderTypeRegistry::GetSingleton();
    PLASMA_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  static const char* s_szColorNames[] = {
    "Red",
    "Pink",
    "Grape",
    "Violet",
    "Indigo",
    "Blue",
    "Cyan",
    "Teal",
    "Green",
    "Lime",
    "Yellow",
    "Orange",
    "Gray",
    "PlasmaBranding",
    "Black"
  };
  static_assert(PLASMA_ARRAY_SIZE(s_szColorNames) == plColorScheme::Count);

  static void GetColorFromDdl(const plOpenDdlReaderElement* pElement, plColorGammaUB& out_color)
  {
    if (pElement->GetPrimitivesType() == plOpenDdlPrimitiveType::String)
    {
      plColorScheme::Enum color = plColorScheme::Gray;
      const plStringView* pValue = pElement->GetPrimitivesString();
      for (plUInt32 i = 0; i < plColorScheme::Count; ++i)
      {
        if (pValue->IsEqual_NoCase(s_szColorNames[i]))
        {
          color = static_cast<plColorScheme::Enum>(i);
          break;
        }
      }

      out_color = plColorScheme::DarkUI(color);
    }
    else
    {
      plOpenDdlUtils::ConvertToColorGamma(pElement, out_color).IgnoreResult();
    }
  }
} // namespace

plVisualShaderTypeRegistry::plVisualShaderTypeRegistry()
  : m_SingletonRegistrar(this)
{
  m_pBaseType = nullptr;
  m_pSamplerPinType = nullptr;
}

const plVisualShaderNodeDescriptor* plVisualShaderTypeRegistry::GetDescriptorForType(const plRTTI* pRtti) const
{
  auto it = m_NodeDescriptors.Find(pRtti);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}


void plVisualShaderTypeRegistry::UpdateNodeData()
{
  plStringBuilder sSearchDir = plApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("VisualShader/*.ddl");

  plFileSystemIterator it;
  for (it.StartSearch(sSearchDir, plFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    UpdateNodeData(it.GetStats().m_sName);
  }
}


void plVisualShaderTypeRegistry::UpdateNodeData(plStringView sCfgFileRelative)
{
  plStringBuilder sPath(":app/VisualShader/", sCfgFileRelative);

  LoadConfigFile(sPath);
}

void plVisualShaderTypeRegistry::LoadNodeData()
{
  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    plReflectedTypeDescriptor desc;
    desc.m_sTypeName = "plVisualShaderNodeBase";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
    desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Abstract | plTypeFlags::Class;
    desc.m_uiTypeVersion = 1;

    m_pBaseType = plPhantomRttiManager::RegisterType(desc);
  }

  if (m_pSamplerPinType == nullptr)
  {
    plReflectedTypeDescriptor desc;
    desc.m_sTypeName = "plVisualShaderSamplerPin";
    desc.m_sPluginName = "VisualShaderTypes";
    desc.m_sParentTypeName = plGetStaticRTTI<plReflectedClass>()->GetTypeName();
    desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Class;
    desc.m_uiTypeVersion = 1;

    m_pSamplerPinType = plPhantomRttiManager::RegisterType(desc);
  }

  UpdateNodeData();
}

const plRTTI* plVisualShaderTypeRegistry::GenerateTypeFromDesc(const plVisualShaderNodeDescriptor& nd)
{
  plStringBuilder temp;
  temp.Set("ShaderNode::", nd.m_sName);

  plReflectedTypeDescriptor desc;
  desc.m_sTypeName = temp;
  desc.m_sPluginName = "VisualShaderTypes";
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = plTypeFlags::Phantom | plTypeFlags::Class;
  desc.m_uiTypeVersion = 1;
  desc.m_Properties = nd.m_Properties;

  for (const auto& pin : nd.m_InputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  for (const auto& pin : nd.m_OutputPins)
  {
    if (pin.m_PropertyDesc.m_sName.IsEmpty())
      continue;

    desc.m_Properties.PushBack(pin.m_PropertyDesc);
  }

  return plPhantomRttiManager::RegisterType(desc);
}

void plVisualShaderTypeRegistry::LoadConfigFile(const char* szFile)
{
  PLASMA_LOG_BLOCK("Loading Visual Shader Config", szFile);

  plLog::Debug("Loading VSE node config '{0}'", szFile);

  plFileReader file;
  if (file.Open(szFile).Failed())
  {
    plLog::Error("Failed to open Visual Shader config file '{0}'", szFile);
    return;
  }

  if (plPathUtils::HasExtension(szFile, "ddl"))
  {
    plOpenDdlReader ddl;
    if (ddl.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
    {
      plLog::Error("Failed to parse Visual Shader config file '{0}'", szFile);
      return;
    }

    const plOpenDdlReaderElement* pRoot = ddl.GetRootElement();
    const plOpenDdlReaderElement* pNode = pRoot->GetFirstChild();

    while (pNode != nullptr)
    {
      if (!pNode->IsCustomType() || pNode->GetCustomType() != "Node")
      {
        plLog::Error("Top-Level object is not a 'Node' type");
        continue;
      }

      plVisualShaderNodeDescriptor nd;
      nd.m_sCfgFile = szFile;
      nd.m_sName = pNode->GetName();

      ExtractNodeConfig(pNode, nd);
      ExtractNodeProperties(pNode, nd);
      ExtractNodePins(pNode, "InputPin", nd.m_InputPins, false);
      ExtractNodePins(pNode, "OutputPin", nd.m_OutputPins, true);

      m_NodeDescriptors.Insert(GenerateTypeFromDesc(nd), nd);

      pNode = pNode->GetSibling();
    }
  }
}

static plVariant ExtractDefaultValue(const plRTTI* pType, const char* szDefault)
{
  if (pType == plGetStaticRTTI<plString>())
  {
    return plVariant(szDefault);
  }

  if (pType == plGetStaticRTTI<bool>())
  {
    bool res = false;
    plConversionUtils::StringToBool(szDefault, res).IgnoreResult();
    return plVariant(res);
  }

  float values[4] = {0, 0, 0, 0};
  plConversionUtils::ExtractFloatsFromString(szDefault, 4, values);

  if (pType == plGetStaticRTTI<float>())
  {
    return plVariant(values[0]);
  }

  if (pType == plGetStaticRTTI<int>())
  {
    return plVariant((int)values[0]);
  }

  if (pType == plGetStaticRTTI<plVec2>())
  {
    return plVariant(plVec2(values[0], values[1]));
  }

  if (pType == plGetStaticRTTI<plVec3>())
  {
    return plVariant(plVec3(values[0], values[1], values[2]));
  }

  if (pType == plGetStaticRTTI<plVec4>())
  {
    return plVariant(plVec4(values[0], values[1], values[2], values[3]));
  }

  if (pType == plGetStaticRTTI<plColor>())
  {
    return plVariant(plColorGammaUB(values[0], values[1], values[2], values[3]));
  }

  return plVariant();
}

void plVisualShaderTypeRegistry::ExtractNodePins(const plOpenDdlReaderElement* pNode, const char* szPinType, plHybridArray<plVisualShaderPinDescriptor, 4>& pinArray, bool bOutput)
{
  for (const plOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == szPinType)
    {
      plVisualShaderPinDescriptor pin;

      if (!pElement->HasName())
      {
        plLog::Error("Missing or invalid name for pin");
        continue;
      }

      pin.m_sName = pElement->GetName();

      auto pType = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "Type");

      if (!pType)
      {
        plLog::Error("Missing or invalid pin type");
        continue;
      }

      {
        const plString& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
          pin.m_pDataType = plGetStaticRTTI<plColor>();
        else if (sType == "float4")
          pin.m_pDataType = plGetStaticRTTI<plVec4>();
        else if (sType == "float3")
          pin.m_pDataType = plGetStaticRTTI<plVec3>();
        else if (sType == "float2")
          pin.m_pDataType = plGetStaticRTTI<plVec2>();
        else if (sType == "float")
          pin.m_pDataType = plGetStaticRTTI<float>();
        else if (sType == "string")
          pin.m_pDataType = plGetStaticRTTI<plString>();
        else if (sType == "sampler")
          pin.m_pDataType = m_pSamplerPinType;
        else
        {
          plLog::Error("Invalid pin type '{0}'", sType);
          continue;
        }
      }

      if (auto pInline = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "Inline"))
      {
        pin.m_sShaderCodeInline = pInline->GetPrimitivesString()[0];
      }
      else if (bOutput)
      {
        plLog::Error("Output pin '{0}' has no inline code specified", pin.m_sName);
        continue;
      }

      // this is optional
      if (auto pColor = pElement->FindChild("Color"))
      {
        GetColorFromDdl(pColor, pin.m_Color);
      }

      // this is optional
      if (auto pTooltip = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "Tooltip"))
      {
        pin.m_sTooltip = pTooltip->GetPrimitivesString()[0];
      }

      // this is optional
      if (auto pDefaultValue = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "DefaultValue"))
      {
        pin.m_sDefaultValue = pDefaultValue->GetPrimitivesString()[0];
      }

      if (auto pDefineWhenUsingDefaultValue = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "DefineWhenUsingDefaultValue"))
      {
        const plUInt32 numElements = pDefineWhenUsingDefaultValue->GetNumPrimitives();
        pin.m_sDefinesWhenUsingDefaultValue.Reserve(numElements);

        for (plUInt32 i = 0; i < numElements; ++i)
        {
          pin.m_sDefinesWhenUsingDefaultValue.PushBack(pDefineWhenUsingDefaultValue->GetPrimitivesString()[i]);
        }
      }

      // this is optional
      if (auto pExpose = pElement->FindChildOfType(plOpenDdlPrimitiveType::Bool, "Expose"))
      {
        pin.m_bExposeAsProperty = pExpose->GetPrimitivesBool()[0];
      }

      if (pin.m_bExposeAsProperty)
      {
        pin.m_PropertyDesc.m_sName = pin.m_sName;
        pin.m_PropertyDesc.m_Category = plPropertyCategory::Member;
        pin.m_PropertyDesc.m_Flags.SetValue((plUInt16)plPropertyFlags::Phantom | (plUInt16)plPropertyFlags::StandardType);
        pin.m_PropertyDesc.m_sType = pin.m_pDataType->GetTypeName();

        const plVariant def = ExtractDefaultValue(pin.m_pDataType, pin.m_sDefaultValue);

        if (def.IsValid())
        {
          pin.m_PropertyDesc.m_Attributes.PushBack(PLASMA_DEFAULT_NEW(plDefaultValueAttribute, def));
        }
      }

      pinArray.PushBack(pin);
    }
  }
}

void plVisualShaderTypeRegistry::ExtractNodeProperties(const plOpenDdlReaderElement* pNode, plVisualShaderNodeDescriptor& nd)
{
  for (const plOpenDdlReaderElement* pElement = pNode->GetFirstChild(); pElement != nullptr; pElement = pElement->GetSibling())
  {
    if (pElement->GetCustomType() == "Property")
    {
      plInt8 iValueGroup = -1;

      plReflectedPropertyDescriptor prop;
      prop.m_Category = plPropertyCategory::Member;
      prop.m_Flags.SetValue((plUInt16)plPropertyFlags::Phantom | (plUInt16)plPropertyFlags::StandardType);

      if (!pElement->HasName())
      {
        plLog::Error("Property doesn't have a name");
        continue;
      }

      prop.m_sName = pElement->GetName();

      const plOpenDdlReaderElement* pType = pElement->FindChildOfType(plOpenDdlPrimitiveType::String, "Type");
      if (!pType)
      {
        plLog::Error("Property doesn't have a type");
        continue;
      }

      const plRTTI* pRtti = nullptr;

      {
        const plStringView& sType = pType->GetPrimitivesString()[0];

        if (sType == "color")
        {
          pRtti = plGetStaticRTTI<plColor>();

          // always expose the alpha channel for color properties
          plExposeColorAlphaAttribute* pAttr = plExposeColorAlphaAttribute::GetStaticRTTI()->GetAllocator()->Allocate<plExposeColorAlphaAttribute>();
          prop.m_Attributes.PushBack(pAttr);
        }
        else if (sType == "float4")
        {
          pRtti = plGetStaticRTTI<plVec4>();
        }
        else if (sType == "float3")
        {
          pRtti = plGetStaticRTTI<plVec3>();
        }
        else if (sType == "float2")
        {
          pRtti = plGetStaticRTTI<plVec2>();
        }
        else if (sType == "float")
        {
          pRtti = plGetStaticRTTI<float>();
        }
        else if (sType == "int")
        {
          pRtti = plGetStaticRTTI<int>();
        }
        else if (sType == "bool")
        {
          pRtti = plGetStaticRTTI<bool>();
        }
        else if (sType == "string")
        {
          pRtti = plGetStaticRTTI<plString>();
        }
        else if (sType == "identifier")
        {
          pRtti = plGetStaticRTTI<plString>();

          iValueGroup = 1; // currently no way to specify the group
        }
        else if (sType == "Texture2D")
        {
          pRtti = plGetStaticRTTI<plString>();

          // apparently the attributes are deallocated using the type allocator, so we must allocate them here through RTTI as well
          plAssetBrowserAttribute* pAttr = plAssetBrowserAttribute::GetStaticRTTI()->GetAllocator()->Allocate<plAssetBrowserAttribute>();
          pAttr->SetTypeFilter("CompatibleAsset_Texture_2D");
          prop.m_Attributes.PushBack(pAttr);
        }
        else
        {
          plLog::Error("Invalid property type '{0}'", sType);
          continue;
        }
      }

      prop.m_sType = pRtti->GetTypeName();

      const plOpenDdlReaderElement* pValue = pElement->FindChild("DefaultValue");
      if (pValue && pRtti != nullptr && pValue->HasPrimitives(plOpenDdlPrimitiveType::String))
      {
        plStringBuilder tmp = pValue->GetPrimitivesString()[0];
        const plVariant def = ExtractDefaultValue(pRtti, tmp);

        if (def.IsValid())
        {
          prop.m_Attributes.PushBack(PLASMA_DEFAULT_NEW(plDefaultValueAttribute, def));
        }
      }

      nd.m_Properties.PushBack(prop);
      nd.m_UniquePropertyValueGroups.PushBack(iValueGroup);
    }
  }
}

void plVisualShaderTypeRegistry::ExtractNodeConfig(const plOpenDdlReaderElement* pNode, plVisualShaderNodeDescriptor& nd)
{
  plStringBuilder temp;

  const plOpenDdlReaderElement* pElement = pNode->GetFirstChild();

  while (pElement)
  {
    if (pElement->GetName() == "Color")
    {
      GetColorFromDdl(pElement, nd.m_Color);
    }
    else if (pElement->HasPrimitives(plOpenDdlPrimitiveType::String))
    {
      if (pElement->GetName() == "NodeType")
      {
        if (pElement->GetPrimitivesString()[0] == "Main")
          nd.m_NodeType = plVisualShaderNodeType::Main;
        else if (pElement->GetPrimitivesString()[0] == "Texture")
          nd.m_NodeType = plVisualShaderNodeType::Texture;
        else
          nd.m_NodeType = plVisualShaderNodeType::Generic;
      }
      else if (pElement->GetName() == "Category")
      {
        nd.m_sCategory = pElement->GetPrimitivesString()[0];
      }
      else if (pElement->GetName() == "CheckPermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        temp.ReplaceAll(" ", "");
        temp.ReplaceAll("\r", "");
        temp.ReplaceAll("\t", "");
        temp.Trim("\n");
        nd.m_sCheckPermutations = temp;
      }
      else if (pElement->GetName() == "CodePermutations")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePermutations = temp;
      }
      else if (pElement->GetName() == "CodeRenderStates")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeRenderState = temp;
      }
      else if (pElement->GetName() == "CodeVertexShader")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeVertexShader = temp;
      }
      else if (pElement->GetName() == "CodeGeometryShader")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeGeometryShader = temp;
      }
      else if (pElement->GetName() == "CodeMaterialParams")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodeMaterialParams = temp;
      }
      else if (pElement->GetName() == "CodeMaterialCB")
      {
        temp = pElement->GetPrimitivesString()[0];
        nd.m_sShaderCodeMaterialCB = temp;
      }
      else if (pElement->GetName() == "CodePixelDefines")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelDefines = temp;
      }
      else if (pElement->GetName() == "CodePixelIncludes")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelIncludes = temp;
      }
      else if (pElement->GetName() == "CodePixelSamplers")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelSamplers = temp;
      }
      else if (pElement->GetName() == "CodePixelConstants")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelConstants = temp;
      }
      else if (pElement->GetName() == "CodePixelBody")
      {
        temp = pElement->GetPrimitivesString()[0];
        if (!temp.IsEmpty() && !temp.EndsWith("\n"))
          temp.Append("\n");
        nd.m_sShaderCodePixelBody = temp;
      }
    }

    pElement = pElement->GetSibling();
  }
}
