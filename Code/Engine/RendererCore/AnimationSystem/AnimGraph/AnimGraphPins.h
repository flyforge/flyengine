#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class plAnimGraphInstance;
class plAnimController;
class plStreamWriter;
class plStreamReader;
struct plAnimGraphPinDataBoneWeights;
struct plAnimGraphPinDataLocalTransforms;
struct plAnimGraphPinDataModelTransforms;

struct plAnimGraphSharedBoneWeights : public plRefCounted
{
  plDynamicArray<ozz::math::SimdFloat4, plAlignedAllocatorWrapper> m_Weights;
};

using plAnimPoseGeneratorLocalPoseID = plUInt32;
using plAnimPoseGeneratorModelPoseID = plUInt32;
using plAnimPoseGeneratorCommandID = plUInt32;

class PL_RENDERERCORE_DLL plAnimGraphPin : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphPin, plReflectedClass);

public:
  enum Type : plUInt8
  {
    Invalid,
    Trigger,
    Number,
    Bool,
    BoneWeights,
    LocalPose,
    ModelPose,
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  virtual plAnimGraphPin::Type GetPinType() const = 0;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

protected:
  friend class plAnimGraph;

  plInt16 m_iPinIndex = -1;
  plUInt8 m_uiNumConnections = 0;
};

class PL_RENDERERCORE_DLL plAnimGraphInputPin : public plAnimGraphPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphInputPin, plAnimGraphPin);

public:
};

class PL_RENDERERCORE_DLL plAnimGraphOutputPin : public plAnimGraphPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphOutputPin, plAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphTriggerInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphTriggerInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Trigger; }

  bool IsTriggered(plAnimGraphInstance& ref_graph) const;
  bool AreAllTriggered(plAnimGraphInstance& ref_graph) const;
};

class PL_RENDERERCORE_DLL plAnimGraphTriggerOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphTriggerOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Trigger; }

  /// \brief Sets this output pin to the triggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(plAnimGraphInstance& ref_graph) const;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphNumberInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphNumberInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Number; }

  double GetNumber(plAnimGraphInstance& ref_graph, double fFallback = 0.0) const;
};

class PL_RENDERERCORE_DLL plAnimGraphNumberOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphNumberOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Number; }

  void SetNumber(plAnimGraphInstance& ref_graph, double value) const;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphBoolInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphBoolInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Bool; }

  bool GetBool(plAnimGraphInstance& ref_graph, bool bFallback = false) const;
};

class PL_RENDERERCORE_DLL plAnimGraphBoolOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphBoolOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::Bool; }

  void SetBool(plAnimGraphInstance& ref_graph, bool bValue) const;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphBoneWeightsInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphBoneWeightsInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::BoneWeights; }

  plAnimGraphPinDataBoneWeights* GetWeights(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const;
};

class PL_RENDERERCORE_DLL plAnimGraphBoneWeightsOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphBoneWeightsOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::BoneWeights; }

  void SetWeights(plAnimGraphInstance& ref_graph, plAnimGraphPinDataBoneWeights* pWeights) const;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphLocalPoseInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphLocalPoseInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::LocalPose; }

  plAnimGraphPinDataLocalTransforms* GetPose(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const;
};

class PL_RENDERERCORE_DLL plAnimGraphLocalPoseMultiInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphLocalPoseMultiInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::LocalPose; }

  void GetPoses(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plDynamicArray<plAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class PL_RENDERERCORE_DLL plAnimGraphLocalPoseOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphLocalPoseOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::LocalPose; }

  void SetPose(plAnimGraphInstance& ref_graph, plAnimGraphPinDataLocalTransforms* pPose) const;
};

//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plAnimGraphModelPoseInputPin : public plAnimGraphInputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphModelPoseInputPin, plAnimGraphInputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::ModelPose; }

  plAnimGraphPinDataModelTransforms* GetPose(plAnimController& ref_controller, plAnimGraphInstance& ref_graph) const;
};

class PL_RENDERERCORE_DLL plAnimGraphModelPoseOutputPin : public plAnimGraphOutputPin
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimGraphModelPoseOutputPin, plAnimGraphOutputPin);

public:
  virtual plAnimGraphPin::Type GetPinType() const override { return plAnimGraphPin::ModelPose; }

  void SetPose(plAnimGraphInstance& ref_graph, plAnimGraphPinDataModelTransforms* pPose) const;
};
