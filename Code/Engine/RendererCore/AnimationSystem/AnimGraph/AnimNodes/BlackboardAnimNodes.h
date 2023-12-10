#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PLASMA_RENDERERCORE_DLL plSetBlackboardNumberAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSetBlackboardNumberAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fNumber = 0.0f; // [ property ]

private:
  plAnimGraphTriggerInputPin m_InActivate; // [ property ]
  plAnimGraphNumberInputPin m_InNumber;    // [ property ]
  plHashedString m_sBlackboardEntry;       // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plGetBlackboardNumberAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGetBlackboardNumberAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plGetBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  plHashedString m_sBlackboardEntry;      // [ property ]
  plAnimGraphNumberOutputPin m_OutNumber; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plCompareBlackboardNumberAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCompareBlackboardNumberAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plCompareBlackboardNumberAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  double m_fReferenceValue = 0.0;            // [ property ]
  plEnum<plComparisonOperator> m_Comparison; // [ property ]

private:
  plHashedString m_sBlackboardEntry;        // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnTrue;  // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFalse; // [ property ]
  plAnimGraphBoolOutputPin m_OutIsTrue;     // [ property ]

  struct InstanceData
  {
    plInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plCheckBlackboardBoolAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCheckBlackboardBoolAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plCheckBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  plHashedString m_sBlackboardEntry;        // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnTrue;  // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnFalse; // [ property ]
  plAnimGraphBoolOutputPin m_OutBool;       // [ property ]

  struct InstanceData
  {
    plInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plSetBlackboardBoolAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSetBlackboardBoolAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plSetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

  bool m_bBool = false; // [ property ]

private:
  plHashedString m_sBlackboardEntry;       // [ property ]
  plAnimGraphTriggerInputPin m_InActivate; // [ property ]
  plAnimGraphBoolInputPin m_InBool;        // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plGetBlackboardBoolAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGetBlackboardBoolAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // plGetBlackboardBoolAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  plHashedString m_sBlackboardEntry;  // [ property ]
  plAnimGraphBoolOutputPin m_OutBool; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_RENDERERCORE_DLL plOnBlackboardValueChangedAnimNode : public plAnimGraphNode
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plOnBlackboardValueChangedAnimNode, plAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // plAnimGraphNode

protected:
  virtual plResult SerializeNode(plStreamWriter& stream) const override;
  virtual plResult DeserializeNode(plStreamReader& stream) override;

  virtual void Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // plOnBlackboardValuechangedAnimNode

public:
  void SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;       // [ property ]

private:
  plHashedString m_sBlackboardEntry;               // [ property ]
  plAnimGraphTriggerOutputPin m_OutOnValueChanged; // [ property ]

  struct InstanceData
  {
    plUInt32 m_uiChangeCounter = plInvalidIndex;
  };
};
