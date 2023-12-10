#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging cloth.
///
/// Uses Verlet Integration to update the cloth positions from velocities, and the "Jakobsen method" to enforce distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class PLASMA_GAMEENGINE_DLL plClothSimulator
{
public:
  struct Node
  {
    /// Whether this node can swing freely or will remain fixed in place.
    bool m_bFixed = false;
    plSimdVec4f m_vPosition = plSimdVec4f::MakeZero();
    plSimdVec4f m_vPreviousPosition = plSimdVec4f::MakeZero();
  };

  /// Resolution of the cloth along X
  plUInt8 m_uiWidth = 32;

  /// Resolution of the cloth along Y
  plUInt8 m_uiHeight = 32;

  /// Overall force acting equally upon all cloth nodes.
  plVec3 m_vAcceleration;

  /// Factor with which all node velocities are damped to reduce swinging.
  float m_fDampingFactor = 0.995f;

  /// The distance along x and y between each neighboring node.
  plVec2 m_vSegmentLength = plVec2(0.1f);

  /// All cloth nodes.
  plDynamicArray<Node, plAlignedAllocatorWrapper> m_Nodes;

  void SimulateCloth(const plTime& tDiff);
  void SimulateStep(const plSimdFloat tDiffSqr, plUInt32 uiMaxIterations, plSimdFloat fAllowedError);
  bool HasEquilibrium(plSimdFloat fAllowedMovement) const;

private:
  plSimdFloat EnforceDistanceConstraint();
  void UpdateNodePositions(const plSimdFloat tDiffSqr);
  plSimdVec4f MoveTowards(const plSimdVec4f posThis, const plSimdVec4f posNext, plSimdFloat factor, const plSimdVec4f fallbackDir, plSimdFloat& inout_fError, plSimdFloat fSegLen);

  plTime m_LeftOverTimeStep;
};
