#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging ropes.
///
/// Can be used both for interactive rope simulation, as well as to just pre-compute the shape of hanging wires, cables, etc.
/// Uses Verlet Integration to update the rope positions from velocities, and the "Jakobsen method" to enforce
/// rope distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class PLASMA_GAMEENGINE_DLL plRopeSimulator
{
public:
  struct Node
  {
    plSimdVec4f m_vPosition = plSimdVec4f::MakeZero();
    plSimdVec4f m_vPreviousPosition = plSimdVec4f::MakeZero();

    // could add per node acceleration
    // could add per node mass
  };

public:
  plRopeSimulator();
  ~plRopeSimulator();

  /// \brief External acceleration, typically gravity or a combination of gravity and wind.
  /// Applied to all rope nodes equally.
  plVec3 m_vAcceleration = plVec3(0, 0, -10);

  /// \brief All the nodes in the rope
  plDynamicArray<Node, plAlignedAllocatorWrapper> m_Nodes;

  /// \brief A factor to dampen velocities to make the rope stop swinging.
  /// Should be between 0.97 (strong damping) and 1.0 (no damping).
  float m_fDampingFactor = 0.995f;

  /// \brief How long each rope segment (between two nodes) should be.
  float m_fSegmentLength = 0.1f;

  bool m_bFirstNodeIsFixed = true;
  bool m_bLastNodeIsFixed = true;

  void SimulateRope(const plTime& tDiff);
  void SimulateStep(const plSimdFloat tDiffSqr, plUInt32 uiMaxIterations, plSimdFloat fAllowedError);
  void SimulateTillEquilibrium(plSimdFloat fAllowedMovement = 0.005f, plUInt32 uiMaxIterations = 1000);
  bool HasEquilibrium(plSimdFloat fAllowedMovement) const;
  float GetTotalLength() const;
  plSimdVec4f GetPositionAtLength(float length) const;

private:
  plSimdFloat EnforceDistanceConstraint();
  void UpdateNodePositions(const plSimdFloat tDiffSqr);
  plSimdVec4f MoveTowards(const plSimdVec4f posThis, const plSimdVec4f posNext, plSimdFloat factor, const plSimdVec4f fallbackDir, plSimdFloat& inout_fError);

  plTime m_LeftOverTimeStep;
};
