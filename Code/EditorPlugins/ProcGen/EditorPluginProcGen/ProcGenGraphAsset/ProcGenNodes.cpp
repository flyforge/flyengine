#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  plExpressionAST::NodeType::Enum GetOperator(plProcGenBinaryOperator::Enum blendMode)
  {
    switch (blendMode)
    {
      case plProcGenBinaryOperator::Add:
        return plExpressionAST::NodeType::Add;
      case plProcGenBinaryOperator::Subtract:
        return plExpressionAST::NodeType::Subtract;
      case plProcGenBinaryOperator::Multiply:
        return plExpressionAST::NodeType::Multiply;
      case plProcGenBinaryOperator::Divide:
        return plExpressionAST::NodeType::Divide;
      case plProcGenBinaryOperator::Max:
        return plExpressionAST::NodeType::Max;
      case plProcGenBinaryOperator::Min:
        return plExpressionAST::NodeType::Min;

        PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return plExpressionAST::NodeType::Invalid;
  }

  plExpressionAST::Node* CreateRandom(plUInt32 uiSeed, plExpressionAST& out_Ast, const plProcGenNodeBase::GraphContext& context)
  {
    PLASMA_ASSERT_DEV(context.m_OutputType != plProcGenNodeBase::GraphContext::Unknown, "Unkown output type");

    auto pointIndexDataType = context.m_OutputType == plProcGenNodeBase::GraphContext::Placement ? plProcessingStream::DataType::Short : plProcessingStream::DataType::Int;
    plExpressionAST::Node* pPointIndex = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPointIndex, pointIndexDataType});

    plExpressionAST::Node* pSeed = out_Ast.CreateFunctionCall(plProcGenExpressionFunctions::s_GetInstanceSeedFunc.m_Desc, plArrayPtr<plExpressionAST::Node*>());
    pSeed = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pSeed, out_Ast.CreateConstant(uiSeed, plExpressionAST::DataType::Int));

    plExpressionAST::Node* arguments[] = {pPointIndex, pSeed};
    return out_Ast.CreateFunctionCall(plDefaultExpressionFunctions::s_RandomFunc.m_Desc, arguments);
  }

  plExpressionAST::Node* CreateRemapFrom01(plExpressionAST::Node* pInput, float fMin, float fMax, plExpressionAST& out_Ast)
  {
    auto pOffset = out_Ast.CreateConstant(fMin);
    auto pScale = out_Ast.CreateConstant(fMax - fMin);

    auto pValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  plExpressionAST::Node* CreateRemapTo01WithFadeout(plExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, plExpressionAST& out_Ast)
  {
    // Note that we need to clamp the scale if it is below eps or we would end up with a division by 0.
    // To counter the clamp we move the lower and upper bounds by eps.
    // If no fade out is specified we would get a value of 0 for inputs that are exactly on the bounds otherwise which is not the expected behavior.

    const float eps = plMath::DefaultEpsilon<float>();
    const float fLowerScale = plMath::Max((fMax - fMin), 0.0f) * fLowerFade;
    const float fUpperScale = plMath::Max((fMax - fMin), 0.0f) * fUpperFade;

    if (fLowerScale < eps)
      fMin = fMin - eps;
    if (fUpperScale < eps)
      fMax = fMax + eps;

    auto pLowerOffset = out_Ast.CreateConstant(fMin);
    auto pLowerValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_Ast.CreateConstant(plMath::Max(fLowerScale, eps));
    pLowerValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_Ast.CreateConstant(fMax);
    auto pUpperValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_Ast.CreateConstant(plMath::Max(fUpperScale, eps));
    pUpperValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_Ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pValue);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenNodeBase, 1, plRTTINoAllocator)
{
  flags.Add(plTypeFlags::Abstract);
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenOutput, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
  }
  PLASMA_END_PROPERTIES;

  flags.Add(plTypeFlags::Abstract);
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plProcGenOutput::Save(plStreamWriter& stream)
{
  stream << m_sName;
  stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_PlacementOutput, 1, plRTTIDefaultAllocator<plProcGen_PlacementOutput>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    PLASMA_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    PLASMA_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new plClampValueAttribute(plAngle::Radian(0.0f), plVariant())),
    PLASMA_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f)), new plClampValueAttribute(plVec3(0.0f), plVariant())),
    PLASMA_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f)), new plClampValueAttribute(plVec3(0.0f), plVariant())),
    PLASMA_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    PLASMA_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new plDefaultValueAttribute(30.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ENUM_MEMBER_PROPERTY("PlacementMode", plProcPlacementMode, m_PlacementMode),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),

    PLASMA_MEMBER_PROPERTY("Density", m_DensityPin),
    PLASMA_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Pink))),
    PLASMA_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Violet))),
    PLASMA_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Cyan)))
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Active} Placement Output: {Name}"),
    new plCategoryAttribute("Output"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_PlacementOutput::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  out_Ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_Ast.CreateConstant(1.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutDensity, plProcessingStream::DataType::Float}, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_Ast, context);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutScale, plProcessingStream::DataType::Float}, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_Ast, context);
    }

    pColorIndex = out_Ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pColorIndex);
    pColorIndex = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pColorIndex, out_Ast.CreateConstant(255.0f));
    pColorIndex = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pColorIndex, out_Ast.CreateConstant(0.5f));

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutColorIndex, plProcessingStream::DataType::Byte}, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_Ast, context);
    }

    pObjectIndex = out_Ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pObjectIndex);
    pObjectIndex = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pObjectIndex, out_Ast.CreateConstant(m_ObjectsToPlace.GetCount() - 1));
    pObjectIndex = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pObjectIndex, out_Ast.CreateConstant(0.5f));

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutObjectIndex, plProcessingStream::DataType::Byte}, pObjectIndex));
  }

  return nullptr;
}

void plProcGen_PlacementOutput::Save(plStreamWriter& stream)
{
  SUPER::Save(stream);

  stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  stream << m_fFootprint;

  stream << m_vMinOffset;
  stream << m_vMaxOffset;

  // chunk version 6
  stream << m_YawRotationSnap;
  stream << m_fAlignToNormal;

  stream << m_vMinScale;
  stream << m_vMaxScale;

  stream << m_fCullDistance;

  stream << m_uiCollisionLayer;

  stream << m_sColorGradient;

  // chunk version 3
  stream << m_sSurface;

  // chunk version 5
  stream << m_PlacementMode;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_VertexColorOutput, 1, plRTTIDefaultAllocator<plProcGen_VertexColorOutput>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PLASMA_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PLASMA_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
    PLASMA_MEMBER_PROPERTY("A", m_APin),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Active} Vertex Color Output: {Name}"),
    new plCategoryAttribute("Output"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_VertexColorOutput::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  out_Ast.m_OutputNodes.Clear();

  plHashedString sOutputNames[4] = {
    plProcGenInternal::ExpressionOutputs::s_sOutColorR,
    plProcGenInternal::ExpressionOutputs::s_sOutColorG,
    plProcGenInternal::ExpressionOutputs::s_sOutColorB,
    plProcGenInternal::ExpressionOutputs::s_sOutColorA,
  };

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_Ast.CreateConstant(0.0f);
    }

    out_Ast.m_OutputNodes.PushBack(out_Ast.CreateOutput({sOutputNames[i], plProcessingStream::DataType::Float}, pInput));
  }

  return nullptr;
}

void plProcGen_VertexColorOutput::Save(plStreamWriter& stream)
{
  SUPER::Save(stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Random, 1, plRTTIDefaultAllocator<plProcGen_Random>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new plClampValueAttribute(-1, plVariant()), new plDefaultValueAttribute(-1), new plMinValueTextAttribute("Auto")),
    PLASMA_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    PLASMA_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),

    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_FUNCTION_PROPERTY(OnObjectCreated),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Random: {Seed}"),
    new plCategoryAttribute("Math"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Random::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_Ast, context);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_Ast);
}

void plProcGen_Random::OnObjectCreated(const plAbstractObjectNode& node)
{
  m_uiAutoSeed = plHashHelper<plUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_PerlinNoise, 1, plRTTIDefaultAllocator<plProcGen_PerlinNoise>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Scale", m_Scale)->AddAttributes(new plDefaultValueAttribute(plVec3(10))),
    PLASMA_MEMBER_PROPERTY("Offset", m_Offset),
    PLASMA_MEMBER_PROPERTY("NumOctaves", m_uiNumOctaves)->AddAttributes(new plClampValueAttribute(1, 6), new plDefaultValueAttribute(3)),
    PLASMA_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    PLASMA_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),

    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Perlin Noise"),
    new plCategoryAttribute("Math"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_PerlinNoise::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  plExpressionAST::Node* pPos = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPosition, plProcessingStream::DataType::Float3});
  pPos = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pPos, out_Ast.CreateConstant(m_Scale, plExpressionAST::DataType::Float3));
  pPos = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pPos, out_Ast.CreateConstant(m_Offset, plExpressionAST::DataType::Float3));

  auto pPosX = out_Ast.CreateSwizzle(plExpressionAST::VectorComponent::X, pPos);
  auto pPosY = out_Ast.CreateSwizzle(plExpressionAST::VectorComponent::Y, pPos);
  auto pPosZ = out_Ast.CreateSwizzle(plExpressionAST::VectorComponent::Z, pPos);

  auto pNumOctaves = out_Ast.CreateConstant(m_uiNumOctaves, plExpressionAST::DataType::Int);

  plExpressionAST::Node* arguments[] = {pPosX, pPosY, pPosZ, pNumOctaves};

  auto pNoiseFunc = out_Ast.CreateFunctionCall(plDefaultExpressionFunctions::s_PerlinNoiseFunc.m_Desc, arguments);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Blend, 2, plRTTIDefaultAllocator<plProcGen_Blend>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Operator", plProcGenBinaryOperator, m_Operator),
    PLASMA_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("ClampOutput", m_bClampOutput),

    PLASMA_MEMBER_PROPERTY("A", m_InputValueAPin),
    PLASMA_MEMBER_PROPERTY("B", m_InputValueBPin),
    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Operator}({A}, {B})"),
    new plCategoryAttribute("Math"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Blend::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_Ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_Ast.CreateConstant(m_fInputValueB);
  }

  plExpressionAST::Node* pBlend = out_Ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_Ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pBlend);
  }

  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Height, 1, plRTTIDefaultAllocator<plProcGen_Height>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PLASMA_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new plDefaultValueAttribute(1000.0f)),
    PLASMA_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),

    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Height: [{MinHeight}, {MaxHeight}]"),
    new plCategoryAttribute("Input"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Height::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float});
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Slope, 1, plRTTIDefaultAllocator<plProcGen_Slope>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(0.0f))),
    PLASMA_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(60.0f))),
    PLASMA_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),


    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Slope: [{MinSlope}, {MaxSlope}]"),
    new plCategoryAttribute("Input"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Slope::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalZ, plProcessingStream::DataType::Float});
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_Ast.CreateBinaryOperator(plExpressionAST::NodeType::Min, out_Ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle = out_Ast.CreateUnaryOperator(plExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_Ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_MeshVertexColor, 1, plRTTIDefaultAllocator<plProcGen_MeshVertexColor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PLASMA_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PLASMA_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
    PLASMA_MEMBER_PROPERTY("A", m_APin),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Mesh Vertex Color"),
    new plCategoryAttribute("Input"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_MeshVertexColor::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  if (sOutputName == "R")
  {
    return out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorR, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "G")
  {
    return out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorG, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "B")
  {
    return out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorB, plProcessingStream::DataType::Float});
  }
  else
  {
    PLASMA_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorA, plProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_ApplyVolumes, 1, plRTTIDefaultAllocator<plProcGen_ApplyVolumes>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new plTagSetWidgetAttribute("Default")),

    PLASMA_MEMBER_PROPERTY("InputValue", m_fInputValue),

    PLASMA_ENUM_MEMBER_PROPERTY("ImageVolumeMode", plProcVolumeImageMode, m_ImageVolumeMode),
    PLASMA_MEMBER_PROPERTY("RefColor", m_RefColor)->AddAttributes(new plExposeColorAlphaAttribute()),

    PLASMA_MEMBER_PROPERTY("In", m_InputValuePin),
    PLASMA_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Volumes: {IncludeTags}"),
    new plCategoryAttribute("Modifiers"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_ApplyVolumes::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context)
{
  PLASMA_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  plUInt32 tagSetIndex = context.m_SharedData.AddTagSet(m_IncludeTags);
  PLASMA_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  auto pPosX = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionX, plProcessingStream::DataType::Float});
  auto pPosY = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionY, plProcessingStream::DataType::Float});
  auto pPosZ = out_Ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float});

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_Ast.CreateConstant(m_fInputValue);
  }

  plExpressionAST::Node* arguments[] = {
    pPosX,
    pPosY,
    pPosZ,
    pInput,
    out_Ast.CreateConstant(tagSetIndex, plExpressionAST::DataType::Int),
    out_Ast.CreateConstant(m_ImageVolumeMode.GetValue(), plExpressionAST::DataType::Int),
    out_Ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.r)),
    out_Ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.g)),
    out_Ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.b)),
    out_Ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.a)),
  };

  return out_Ast.CreateFunctionCall(plProcGenExpressionFunctions::s_ApplyVolumesFunc.m_Desc, arguments);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plProcGen_Blend_1_2 : public plGraphPatch
{
public:
  plProcGen_Blend_1_2()
    : plGraphPatch("plProcGen_Blend", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pMode = pNode->FindProperty("Mode");
    if (pMode && pMode->m_Value.IsA<plString>())
    {
      plStringBuilder val = pMode->m_Value.Get<plString>();
      val.ReplaceAll("plProcGenBlendMode", "plProcGenBinaryOperator");

      pNode->AddProperty("Operator", val.GetData());
    }
  }
};

plProcGen_Blend_1_2 g_plProcGen_Blend_1_2;