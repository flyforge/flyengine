#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <ProcGenPlugin/Declarations.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plProcGenBinaryOperator, 1)
  PL_ENUM_CONSTANTS(plProcGenBinaryOperator::Add, plProcGenBinaryOperator::Subtract, plProcGenBinaryOperator::Multiply, plProcGenBinaryOperator::Divide)
  PL_ENUM_CONSTANTS(plProcGenBinaryOperator::Max, plProcGenBinaryOperator::Min)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plProcGenBlendMode, 1)
  PL_ENUM_CONSTANTS(plProcGenBlendMode::Add, plProcGenBlendMode::Subtract, plProcGenBlendMode::Multiply, plProcGenBlendMode::Divide)
  PL_ENUM_CONSTANTS(plProcGenBlendMode::Max, plProcGenBlendMode::Min)
  PL_ENUM_CONSTANTS(plProcGenBlendMode::Set)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plProcVertexColorChannelMapping, 1)
  PL_ENUM_CONSTANTS(plProcVertexColorChannelMapping::R, plProcVertexColorChannelMapping::G, plProcVertexColorChannelMapping::B, plProcVertexColorChannelMapping::A)
  PL_ENUM_CONSTANTS(plProcVertexColorChannelMapping::Black, plProcVertexColorChannelMapping::White)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plProcVertexColorMapping, plNoBase, 1, plRTTIDefaultAllocator<plProcVertexColorMapping>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("R", plProcVertexColorChannelMapping, m_R)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::R)),
    PL_ENUM_MEMBER_PROPERTY("G", plProcVertexColorChannelMapping, m_G)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::G)),
    PL_ENUM_MEMBER_PROPERTY("B", plProcVertexColorChannelMapping, m_B)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::B)),
    PL_ENUM_MEMBER_PROPERTY("A", plProcVertexColorChannelMapping, m_A)->AddAttributes(new plDefaultValueAttribute(plProcVertexColorChannelMapping::A)),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plProcPlacementMode, 1)
  PL_ENUM_CONSTANTS(plProcPlacementMode::Raycast, plProcPlacementMode::Fixed)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plProcPlacementPattern, 1)
  PL_ENUM_CONSTANTS(plProcPlacementPattern::RegularGrid, plProcPlacementPattern::HexGrid, plProcPlacementPattern::Natural)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plProcVolumeImageMode, 1)
  PL_ENUM_CONSTANTS(plProcVolumeImageMode::ReferenceColor, plProcVolumeImageMode::ChannelR, plProcVolumeImageMode::ChannelG, plProcVolumeImageMode::ChannelB, plProcVolumeImageMode::ChannelA)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

static plTypeVersion s_ProcVertexColorMappingVersion = 1;
plResult plProcVertexColorMapping::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorMappingVersion);
  inout_stream << m_R;
  inout_stream << m_G;
  inout_stream << m_B;
  inout_stream << m_A;

  return PL_SUCCESS;
}

plResult plProcVertexColorMapping::Deserialize(plStreamReader& inout_stream)
{
  /*plTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorMappingVersion);
  inout_stream >> m_R;
  inout_stream >> m_G;
  inout_stream >> m_B;
  inout_stream >> m_A;

  return PL_SUCCESS;
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
