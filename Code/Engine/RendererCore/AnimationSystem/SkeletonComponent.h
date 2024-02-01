#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct plMsgQueryAnimationSkeleton;

using plVisualizeSkeletonComponentManager = plComponentManagerSimple<class plSkeletonComponent, plComponentUpdateType::Always, plBlockStorageType::Compact>;

/// \brief Uses debug rendering to visualize various aspects of an animation skeleton.
///
/// This is meant for visually inspecting skeletons. It is used by the main skeleton editor,
/// but can also be added to a scene or added to an animated mesh on-demand.
///
/// There are different options what to visualize and also to highlight certain bones.
class PL_RENDERERCORE_DLL plSkeletonComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSkeletonComponent, plRenderComponent, plVisualizeSkeletonComponentManager);

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

  /// \brief Sets a semicolon-separated list of bone names that should be highlighted.
  ///
  /// Set it to "*" to highlight all bones.
  /// Set it to empty to not highlight any bone.
  /// Set it to "BoneA;BoneB" to highlight the bones with name "BoneA" and "BoneB".
  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  bool m_bVisualizeBones = true;        // [ property ]
  bool m_bVisualizeColliders = false;   // [ property ]
  bool m_bVisualizeJoints = false;      // [ property ]
  bool m_bVisualizeSwingLimits = false; // [ property ]
  bool m_bVisualizeTwistLimits = false; // [ property ]

protected:
  void Update();
  void VisualizeSkeletonDefaultState();
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
