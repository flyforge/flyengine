#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>

class plProcGenNodeBase : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGenNodeBase, plReflectedClass);

public:
  struct GraphContext
  {
    enum OutputType
    {
      Unknown,
      Placement,
      Color,
    };

    plProcGenInternal::GraphSharedData m_SharedData;
    plHybridArray<plUInt8, 4> m_VolumeTagSetIndices;
    OutputType m_OutputType = OutputType::Unknown;
  };

  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) = 0;
};

//////////////////////////////////////////////////////////////////////////

class plProcGenOutput : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGenOutput, plProcGenNodeBase);

public:
  bool m_bActive = true;

  void Save(plStreamWriter& stream);

  plString m_sName;

  plHybridArray<plUInt8, 4> m_VolumeTagSetIndices;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_PlacementOutput : public plProcGenOutput
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_PlacementOutput, plProcGenOutput);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  void Save(plStreamWriter& stream);

  plHybridArray<plString, 4> m_ObjectsToPlace;

  float m_fFootprint = 1.0f;

  plVec3 m_vMinOffset = plVec3(0);
  plVec3 m_vMaxOffset = plVec3(0);

  plAngle m_YawRotationSnap = plAngle::Radian(0.0f);
  float m_fAlignToNormal = 1.0f;

  plVec3 m_vMinScale = plVec3(1);
  plVec3 m_vMaxScale = plVec3(1);

  float m_fCullDistance = 30.0f;

  plUInt32 m_uiCollisionLayer = 0;

  plString m_sSurface;

  plString m_sColorGradient;

  plEnum<plProcPlacementMode> m_PlacementMode;

  plRenderPipelineNodeInputPin m_DensityPin;
  plRenderPipelineNodeInputPin m_ScalePin;
  plRenderPipelineNodeInputPin m_ColorIndexPin;
  plRenderPipelineNodeInputPin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_VertexColorOutput : public plProcGenOutput
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_VertexColorOutput, plProcGenOutput);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  void Save(plStreamWriter& stream);

  plRenderPipelineNodeInputPin m_RPin;
  plRenderPipelineNodeInputPin m_GPin;
  plRenderPipelineNodeInputPin m_BPin;
  plRenderPipelineNodeInputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Random : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_Random, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  plInt32 m_iSeed = -1;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  plRenderPipelineNodeOutputPin m_OutputValuePin;

private:
  void OnObjectCreated(const plAbstractObjectNode& node);

  plUInt32 m_uiAutoSeed;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_PerlinNoise : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_PerlinNoise, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  plVec3 m_Scale = plVec3(10);
  plVec3 m_Offset = plVec3::ZeroVector();
  plUInt32 m_uiNumOctaves = 3;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  plRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Blend : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_Blend, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  plEnum<plProcGenBinaryOperator> m_Operator;
  float m_fInputValueA = 1.0f;
  float m_fInputValueB = 1.0f;
  bool m_bClampOutput = false;

  plRenderPipelineNodeInputPin m_InputValueAPin;
  plRenderPipelineNodeInputPin m_InputValueBPin;
  plRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Height : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_Height, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  float m_fMinHeight = 0.0f;
  float m_fMaxHeight = 1000.0f;
  float m_fLowerFade = 0.2f;
  float m_fUpperFade = 0.2f;

  plRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Slope : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_Slope, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  plAngle m_MinSlope = plAngle::Degree(0.0f);
  plAngle m_MaxSlope = plAngle::Degree(30.0f);
  float m_fLowerFade = 0.0f;
  float m_fUpperFade = 0.2f;

  plRenderPipelineNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_MeshVertexColor : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_MeshVertexColor, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  plRenderPipelineNodeOutputPin m_RPin;
  plRenderPipelineNodeOutputPin m_GPin;
  plRenderPipelineNodeOutputPin m_BPin;
  plRenderPipelineNodeOutputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_ApplyVolumes : public plProcGenNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcGen_ApplyVolumes, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_Ast, GraphContext& context) override;

  float m_fInputValue = 0.0f;

  plTagSet m_IncludeTags;

  plEnum<plProcVolumeImageMode> m_ImageVolumeMode;
  plColorGammaUB m_RefColor;

  plRenderPipelineNodeInputPin m_InputValuePin;
  plRenderPipelineNodeOutputPin m_OutputValuePin;
};