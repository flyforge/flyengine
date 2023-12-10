#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

class plDebugRendererContext;

/// \brief Aggregated data by plAiNavigation that should be sufficient to implement a steering behavior.
struct plAiSteeringInfo
{
  plVec3 m_vNextWaypoint;
  float m_fDistanceToWaypoint = 0;
  float m_fArrivalDistance = plMath::HighValue<float>();
  plVec2 m_vDirectionTowardsWaypoint = plVec2::MakeZero();
  plAngle m_AbsRotationTowardsWaypoint = plAngle();
  plAngle m_MaxAbsRotationAfterWaypoint = plAngle();
  // float m_fWaypointCorridorWidth = plMath::HighValue<float>();
};

/// \brief Computes a path through a navigation mesh.
///
/// First call SetNavmesh() and SetQueryFilter().
///
/// When you need a path, call SetCurrentPosition() and SetTargetPosition() to inform the
/// system of the current position and desired target location.
/// Then call Update() once per frame to have it compute the path.
/// Call GetState() to figure out whether a path exists.
/// Use ComputeAllWaypoints() to get an entire path, e.g. for visualization.
/// For steering this is not necessary. Instead use ComputeSteeringInfo() to plan the next step.
/// Apply your steering behavior to your character as desired.
/// Keep calling SetCurrentPosition() and SetTargetPosition() to inform the plAiNavigation of the
/// new state, and keep calling ComputeSteeringInfo() every frame for the updated path.
///
/// If the destination was reached, a completely different path should be computed, or the current
/// path should be canceled, call CancelNavigation().
/// To start a new path search, call SetTargetPosition() again (and Update() every frame).
class PLASMA_AIPLUGIN_DLL plAiNavigation final
{
public:
  plAiNavigation();
  ~plAiNavigation();

  enum class State
  {
    Idle,
    StartNewSearch,
    InvalidCurrentPosition,
    InvalidTargetPosition,
    NoPathFound,
    PartialPathFound,
    FullPathFound,
    Searching,
  };

  static constexpr plUInt32 MaxPathNodes = 64;
  static constexpr plUInt32 MaxSearchNodes = MaxPathNodes * 8;

  State GetState() const { return m_State; }

  void Update();

  void CancelNavigation();

  void SetCurrentPosition(const plVec3& vPosition);
  void SetTargetPosition(const plVec3& vPosition);
  const plVec3& GetTargetPosition() const;
  void SetNavmesh(plAiNavMesh& ref_navmesh);
  void SetQueryFilter(const dtQueryFilter& filter);

  void ComputeAllWaypoints(plDynamicArray<plVec3>& out_waypoints) const;

  void DebugDraw(const plDebugRendererContext& context, plColor tilesColor, plColor straightLineColor, float fPolyRenderOffsetZ = 0.1f, float fLineRenderOffsetZ = 0.2f);

  /// \brief Returns the height of the navmesh at the current position.
  float GetCurrentElevation() const;

  void ComputeSteeringInfo(plAiSteeringInfo& out_info, const plVec2& vForwardDir, float fMaxLookAhead = 5.0f);

  // in what radius / up / down distance navigation mesh polygons should be searched around a given position
  // this should relate to the character size, ie at least the character radius
  // otherwise a character that barely left the navmesh area may not know where it is, anymore
  float m_fPolySearchRadius = 0.5f;
  float m_fPolySearchUp = 1.5f;
  float m_fPolySearchDown = 1.5f;

  // when a path search is started, all tiles in a rectangle around the start and end point are loaded first
  // this is the amount to increase that rectangle size, to overestimate which sectors may be needed during the path search
  constexpr static float c_fPathSearchBoundary = 10.0f;

private:
  State m_State = State::Idle;

  plVec3 m_vCurrentPosition = plVec3::MakeZero();
  plVec3 m_vTargetPosition = plVec3::MakeZero();

  plUInt8 m_uiCurrentPositionChangedBit : 1;
  plUInt8 m_uiTargetPositionChangedBit : 1;
  plUInt8 m_uiEnvironmentChangedBit : 1;
  plUInt8 m_uiReinitQueryBit : 1;

  plAiNavMesh* m_pNavmesh = nullptr;
  dtNavMeshQuery m_Query;
  const dtQueryFilter* m_pFilter = nullptr;
  dtPathCorridor m_PathCorridor;

  dtPolyRef m_PathSearchTargetPoly;
  plVec3 m_vPathSearchTargetPos;

  plUInt8 m_uiOptimizeTopologyCounter = 0;
  plUInt8 m_uiOptimizeVisibilityCounter = 0;

  bool UpdatePathSearch();
};
