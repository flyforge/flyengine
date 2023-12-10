#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <ProcGenPlugin/Declarations.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcGenBinaryOperator, 1)
  PLASMA_ENUM_CONSTANTS(plProcGenBinaryOperator::Add, plProcGenBinaryOperator::Subtract, plProcGenBinaryOperator::Multiply, plProcGenBinaryOperator::Divide)
  PLASMA_ENUM_CONSTANTS(plProcGenBinaryOperator::Max, plProcGenBinaryOperator::Min)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcGenBlendMode, 1)
  PLASMA_ENUM_CONSTANTS(plProcGenBlendMode::Add, plProcGenBlendMode::Subtract, plProcGenBlendMode::Multiply, plProcGenBlendMode::Divide)
  PLASMA_ENUM_CONSTANTS(plProcGenBlendMode::Max, plProcGenBlendMode::Min)
  PLASMA_ENUM_CONSTANTS(plProcGenBlendMode::Set)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcVertexColorChannelMapping, 1)
  PLASMA_ENUM_CONSTANTS(plProcVertexColorChannelMapping::R, plProcVertexColorChannelMapping::G, plProcVertexColorChannelMapping::B, plProcVertexColorChannelMapping::A)
  PLASMA_ENUM_CONSTANTS(plProcVertexColorChannelMapping::Black, plProcVertexColorChannelMapping::White)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plProcVertexColorMapping, plNoBase, 1, plRTTIDefaultAllocator<plProcVertexColorMapping>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("R", plProcVertexColorChannelMapping, m_R)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::R)),
    PLASMA_ENUM_MEMBER_PROPERTY("G", plProcVertexColorChannelMapping, m_G)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::G)),
    PLASMA_ENUM_MEMBER_PROPERTY("B", plProcVertexColorChannelMapping, m_B)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::B)),
    PLASMA_ENUM_MEMBER_PROPERTY("A", plProcVertexColorChannelMapping, m_A)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::A)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcPlacementMode, 1)
  PLASMA_ENUM_CONSTANTS(plProcPlacementMode::Raycast, plProcPlacementMode::Fixed)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcPlacementPattern, 1)
  PLASMA_ENUM_CONSTANTS(plProcPlacementPattern::RegularGrid, plProcPlacementPattern::HexGrid, plProcPlacementPattern::Natural)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plProcVolumeImageMode, 1)
  PLASMA_ENUM_CONSTANTS(plProcVolumeImageMode::ReferenceColor, plProcVolumeImageMode::ChannelR, plProcVolumeImageMode::ChannelG, plProcVolumeImageMode::ChannelB, plProcVolumeImageMode::ChannelA)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

static plTypeVersion s_ProcVertexColorMappingVersion = 1;
plResult plProcVertexColorMapping::Serialize(plStreamWriter& stream) const
{
  stream.WriteVersion(s_ProcVertexColorMappingVersion);
  stream << m_R;
  stream << m_G;
  stream << m_B;
  stream << m_A;

  return PLASMA_SUCCESS;
}

plResult plProcVertexColorMapping::Deserialize(plStreamReader& stream)
{
  /*plTypeVersion version =*/stream.ReadVersion(s_ProcVertexColorMappingVersion);
  stream >> m_R;
  stream >> m_G;
  stream >> m_B;
  stream >> m_A;

  return PLASMA_SUCCESS;
}

namespace plProcGenInternal
{
  GraphSharedDataBase::~GraphSharedDataBase() = default;
  Output::~Output() = default;

  plHashedString ExpressionInputs::s_sPosition = plMakeHashedString("position");
  plHashedString ExpressionInputs::s_sPositionX = plMakeHashedString("position.x");
  plHashedString ExpressionInputs::s_sPositionY = plMakeHashedString("position.y");
  plHashedString ExpressionInputs::s_sPositionZ = plMakeHashedString("position.z");
  plHashedString ExpressionInputs::s_sNormal = plMakeHashedString("normal");
  plHashedString ExpressionInputs::s_sNormalX = plMakeHashedString("normal.x");
  plHashedString ExpressionInputs::s_sNormalY = plMakeHashedString("normal.y");
  plHashedString ExpressionInputs::s_sNormalZ = plMakeHashedString("normal.z");
  plHashedString ExpressionInputs::s_sColor = plMakeHashedString("color");
  plHashedString ExpressionInputs::s_sColorR = plMakeHashedString("color.x");
  plHashedString ExpressionInputs::s_sColorG = plMakeHashedString("color.y");
  plHashedString ExpressionInputs::s_sColorB = plMakeHashedString("color.z");
  plHashedString ExpressionInputs::s_sColorA = plMakeHashedString("color.w");
  plHashedString ExpressionInputs::s_sPointIndex = plMakeHashedString("pointIndex");

  plHashedString ExpressionOutputs::s_sOutDensity = plMakeHashedString("outDensity");
  plHashedString ExpressionOutputs::s_sOutScale = plMakeHashedString("outScale");
  plHashedString ExpressionOutputs::s_sOutColorIndex = plMakeHashedString("outColorIndex");
  plHashedString ExpressionOutputs::s_sOutObjectIndex = plMakeHashedString("outObjectIndex");

  plHashedString ExpressionOutputs::s_sOutColor = plMakeHashedString("outColor");
  plHashedString ExpressionOutputs::s_sOutColorR = plMakeHashedString("outColor.x");
  plHashedString ExpressionOutputs::s_sOutColorG = plMakeHashedString("outColor.y");
  plHashedString ExpressionOutputs::s_sOutColorB = plMakeHashedString("outColor.z");
  plHashedString ExpressionOutputs::s_sOutColorA = plMakeHashedString("outColor.w");
} // namespace plProcGenInternal