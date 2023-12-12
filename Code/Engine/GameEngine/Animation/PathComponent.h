#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Types/Bitflags.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgTransformChanged;
struct plMsgParentChanged;

//////////////////////////////////////////////////////////////////////////

PLASMA_DECLARE_FLAGS(plUInt32, plPathComponentFlags, VisualizePath, VisualizeUpDir);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plPathComponentFlags);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_GAMEENGINE_DLL plMsgPathChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgPathChanged, plEventMessage);
};

//////////////////////////////////////////////////////////////////////////

class plPathComponentManager : public plComponentManager<class plPathComponent, plBlockStorageType::FreeList>
{
public:
  plPathComponentManager(plWorld* pWorld);

  void SetEnableUpdate(plPathComponent* pThis, bool bEnable);

protected:
  void Initialize() override;
  void Update(const plWorldModule::UpdateContext& context);

  plHybridArray<plPathComponent*, 32> m_NeedUpdate;
};

/// \brief Describes a path shape.
///
/// This can be used for moving things along the path (see plFollowPathComponent) or to describe the (complex) shape of an object, for example a rope.
///
/// The plPathComponent stores the shape as nodes with positions and tangents.
/// It can be asked to provide a 'linearized' representation, e.g. one that is made up of many short segments whose linear interpolation
/// is still reasonably close to the curved shape.
///
/// To set up the shape, attach child objects and attach an plPathNodeComponent to each. Also give each child object a distinct name.
/// Then reference these child objects by name through the "Nodes" property on the path shape.
///
/// During scene export, typically the child objects are automatically deleted (if they have no children and no other components).
/// Instead, the plPathComponent stores all necessary information in a more compact representation.
class PLASMA_GAMEENGINE_DLL plPathComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plPathComponent, plComponent, plPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(plWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plPathComponent

public:
  plPathComponent();
  ~plPathComponent();

  /// \brief Informs the path component, that its shape has changed. Sent by path nodes when they are modified.
  void OnMsgPathChanged(plMsgPathChanged& ref_msg); // [ message handler ]

  /// \brief Whether the path end connects to the beginning.
  void SetClosed(bool bClosed);                // [ property ]
  bool GetClosed() const { return m_bClosed; } // [ property ]

  void SetPathFlags(plBitflags<plPathComponentFlags> flags);                    // [ property ]
  plBitflags<plPathComponentFlags> GetPathFlags() const { return m_PathFlags; } // [ property ]


  /// \brief The 'raw' data for a single path control point
  struct ControlPoint
  {
    plVec3 m_vPosition = plVec3::ZeroVector();
    plVec3 m_vTangentIn = plVec3::ZeroVector();
    plVec3 m_vTangentOut = plVec3::ZeroVector();
    plAngle m_Roll;

    plResult Serialize(plStreamWriter& ref_writer) const;
    plResult Deserialize(plStreamReader& ref_reader);
  };

  /// \brief If the control points changed recently, this makes sure the local representation is synchronized. Call this before GetControlPointRepresentation(), if necessary.
  void EnsureControlPointRepresentationIsUpToDate();

  /// \brief Grants access to the control points that define the path's shape.
  const plArrayPtr<const plPathComponent::ControlPoint> GetControlPointRepresentation() const { return m_ControlPointRepresentation; }


  /// \brief If the path is linearized, this represents a single sample point
  struct LinearizedElement
  {
    plVec3 m_vPosition = plVec3::ZeroVector();
    plVec3 m_vUpDirection = plVec3::UnitZAxis();
  };

  /// \brief If the control points changed recently, this makes sure the linearized representation gets recreated. Call this before GetLinearizedRepresentation(), if necessary.
  void EnsureLinearizedRepresentationIsUpToDate();

  /// \brief Grants access to the linearized representation that define the path's shape.
  const plArrayPtr<const plPathComponent::LinearizedElement> GetLinearizedRepresentation() const { return m_LinearizedRepresentation; }

  /// \brief Returns the total length of the linearized path representation.
  float GetLinearizedRepresentationLength() const { return m_fLinearizedLength; }

  /// \brief Forces that the current control point state is never updated in the future. Used as a work-around during serialization.
  void SetDisableControlPointUpdates(bool bDisable) { m_bDisableControlPointUpdates = bDisable; }

  /// \brief An object that keeps track of where one is sampling the path component.
  ///
  /// If you want to follow a path, keep this object alive and only advance its position,
  /// to not waste performance.
  struct LinearSampler
  {
    /// \brief Resets the sampler to point to the beginning of the path.
    void SetToStart();

  private:
    friend class plPathComponent;

    float m_fSegmentFraction = 0.0f;
    plUInt32 m_uiSegmentNode = 0;
  };

  /// \brief Sets the sampler to the desired distance along the path.
  ///
  /// For a long distance, this is a slow operation, because it has to follow the path
  /// from the beginning.
  void SetLinearSamplerTo(LinearSampler& ref_sampler, float fDistance) const;

  /// \brief Moves the sampler along the path by the desired distance.
  ///
  /// Prefer this over SetLinearSamplerTo().
  bool AdvanceLinearSamplerBy(LinearSampler& ref_sampler, float& inout_fAddDistance) const;

  /// \brief Samples the linearized path representation at the desired location and returns the interpolated values.
  plPathComponent::LinearizedElement SampleLinearizedRepresentation(const LinearSampler& sampler) const;

  /// \brief Specifies how large the error of the linearized path representation is allowed to be.
  ///
  /// The lower the allowed error, the more detailed the linearized path will be, to match the
  /// Bplier representation as closely as possible.
  ///
  /// The error is a distance measure. Thus a value of 0.01 means that the linearized representation
  /// may at most deviate a centimeter from the real curve.
  void SetLinearizationError(float fError);                             // [ property ]
  float GetLinearizationError() const { return m_fLinearizationError; } // [ property ]

protected:
  plUInt32 Nodes_GetCount() const { return m_Nodes.GetCount(); }         // [ property ]
  const plString& Nodes_GetNode(plUInt32 i) const { return m_Nodes[i]; } // [ property ]
  void Nodes_SetNode(plUInt32 i, const plString& node);                  // [ property ]
  void Nodes_Insert(plUInt32 uiIndex, const plString& node);             // [ property ]
  void Nodes_Remove(plUInt32 uiIndex);                                   // [ property ]

  void FindControlPoints(plDynamicArray<ControlPoint>& out_ControlPoints) const;
  void CreateLinearizedPathRepresentation(const plDynamicArray<ControlPoint>& points);

  void DrawDebugVisualizations();

  plBitflags<plPathComponentFlags> m_PathFlags;                 // [ property ]
  float m_fLinearizationError = 0.05f;                          // [ property ]
  float m_fLinearizedLength = 0.0f;                             //
  bool m_bDisableControlPointUpdates = false;                   //
  bool m_bControlPointsChanged = true;                          //
  bool m_bLinearizedRepresentationChanged = true;               //
  bool m_bClosed = false;                                       // [ property ]
  plDynamicArray<plString> m_Nodes;                             // [ property ]
  plDynamicArray<LinearizedElement> m_LinearizedRepresentation; //
  plDynamicArray<ControlPoint> m_ControlPointRepresentation;    //
};

//////////////////////////////////////////////////////////////////////////

using plPathNodeComponentManager = plComponentManager<class plPathNodeComponent, plBlockStorageType::Compact>;

/// \brief The different modes that tangents may use in a path node.
struct plPathNodeTangentMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the node is automatically computed to be smooth.
    Linear, ///< There is no curvature through this node/tangent. Creates sharp corners.

    Default = Auto
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plPathNodeTangentMode);


/// \brief Attach this to child object of an plPathComponent to turn them into viable path nodes.
///
/// See plPathComponent for details on how to create a path.
///
/// This component allows to specify the mode of the tangents (linear, curved),
/// and also to adjust the 'roll' that the path will have at this location (rotation around the forward axis).
class PLASMA_GAMEENGINE_DLL plPathNodeComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plPathNodeComponent, plComponent, plPathNodeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plPathNodeComponent

public:
  plPathNodeComponent();
  ~plPathNodeComponent();

  /// \brief Sets the rotation along the forward axis, that the path shall have at this location.
  void SetRoll(plAngle roll);                                                      // [ property ]
  plAngle GetRoll() const { return m_Roll; }                                       // [ property ]
                                                                                   //
  void SetTangentMode1(plEnum<plPathNodeTangentMode> mode);                        // [ property ]
  plEnum<plPathNodeTangentMode> GetTangentMode1() const { return m_TangentMode1; } // [ property ]
                                                                                   //
  void SetTangentMode2(plEnum<plPathNodeTangentMode> mode);                        // [ property ]
  plEnum<plPathNodeTangentMode> GetTangentMode2() const { return m_TangentMode2; } // [ property ]

protected:
  void OnMsgTransformChanged(plMsgTransformChanged& msg);
  void OnMsgParentChanged(plMsgParentChanged& msg);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void PathChanged();

  plAngle m_Roll;
  plEnum<plPathNodeTangentMode> m_TangentMode1;
  plEnum<plPathNodeTangentMode> m_TangentMode2;
};
