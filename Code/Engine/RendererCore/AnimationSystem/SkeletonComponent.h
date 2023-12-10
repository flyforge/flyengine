#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct plMsgQueryAnimationSkeleton;

using plVisualizeSkeletonComponentManager = plComponentManagerSimple<class plSkeletonComponent, plComponentUpdateType::Always, plBlockStorageType::Compact>;

class PLASMA_RENDERERCORE_DLL plSkeletonComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSkeletonComponent, plRenderComponent, plVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plSkeletonComponent

public:
  plSkeletonComponent();
  ~plSkeletonComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const plSkeletonResourceHandle& hResource);
  const plSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  void VisualizeSkeletonDefaultState();

  bool m_bVisualizeBones = true;
  bool m_bVisualizeColliders = false;
  bool m_bVisualizeJoints = false;
  bool m_bVisualizeSwingLimits = false;
  bool m_bVisualizeTwistLimits = false;

protected:
  void Update();
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(plMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(plMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(plMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(plMsgQueryAnimationSkeleton& msg);
  plDebugRenderer::Line& AddLine(const plVec3& vStart, const plVec3& vEnd, const plColor& color);

  plSkeletonResourceHandle m_hSkeleton;
  plTransform m_RootTransform = plTransform::MakeIdentity();
  plUInt32 m_uiSkeletonChangeCounter = 0;
  plString m_sBonesToHighlight;

  plBoundingBox m_MaxBounds;
  plDynamicArray<plDebugRenderer::Line> m_LinesSkeleton;

  struct SphereShape
  {
    plTransform m_Transform;
    plBoundingSphere m_Shape;
    plColor m_Color;
  };

  struct BoxShape
  {
    plTransform m_Transform;
    plBoundingBox m_Shape;
    plColor m_Color;
  };

  struct CapsuleShape
  {
    plTransform m_Transform;
    float m_fLength;
    float m_fRadius;
    plColor m_Color;
  };

  struct AngleShape
  {
    plTransform m_Transform;
    plColor m_Color;
    plAngle m_StartAngle;
    plAngle m_EndAngle;
  };

  struct ConeLimitShape
  {
    plTransform m_Transform;
    plColor m_Color;
    plAngle m_Angle1;
    plAngle m_Angle2;
  };

  struct CylinderShape
  {
    plTransform m_Transform;
    plColor m_Color;
    float m_fRadius1;
    float m_fRadius2;
    float m_fLength;
  };

  plDynamicArray<SphereShape> m_SpheresShapes;
  plDynamicArray<BoxShape> m_BoxShapes;
  plDynamicArray<CapsuleShape> m_CapsuleShapes;
  plDynamicArray<AngleShape> m_AngleShapes;
  plDynamicArray<ConeLimitShape> m_ConeLimitShapes;
  plDynamicArray<CylinderShape> m_CylinderShapes;
};
