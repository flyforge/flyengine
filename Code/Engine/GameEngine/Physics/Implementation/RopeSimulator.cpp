#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/RopeSimulator.h>

plRopeSimulator::plRopeSimulator() = default;
plRopeSimulator::~plRopeSimulator() = default;

void plRopeSimulator::SimulateRope(const plTime& tDiff)
{
  m_LeftOverTimeStep += tDiff;

  constexpr plTime tStep = plTime::Seconds(1.0 / 60.0);
  const plSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());
  const plSimdFloat fAllowedError = m_fSegmentLength;

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, fAllowedError);

    m_LeftOverTimeStep -= tStep;
  }
}

void plRopeSimulator::SimulateStep(const plSimdFloat tDiffSqr, plUInt32 uiMaxIterations, plSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 2)
    return;

  UpdateNodePositions(tDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (plUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const plSimdFloat fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

void plRopeSimulator::SimulateTillEquilibrium(plSimdFloat fAllowedMovement, plUInt32 uiMaxIterations)
{
  constexpr plTime tStep = plTime::Seconds(1.0 / 60.0);
  plSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  plUInt8 uiInEquilibrium = 0;

  while (uiInEquilibrium < 100 && uiMaxIterations > 0)
  {
    --uiMaxIterations;

    SimulateStep(tStepSqr, 32, m_fSegmentLength);
    uiInEquilibrium++;

    if (!HasEquilibrium(fAllowedMovement))
    {
      uiInEquilibrium = 0;
    }
  }
}

bool plRopeSimulator::HasEquilibrium(plSimdFloat fAllowedMovement) const
{
  const plSimdFloat fErrorSqr = fAllowedMovement * fAllowedMovement;

  for (const auto& n : m_Nodes)
  {
    if ((n.m_vPosition - n.m_vPreviousPosition).GetLengthSquared<3>() > fErrorSqr)
    {
      return false;
    }
  }

  return true;
}

float plRopeSimulator::GetTotalLength() const
{
  if (m_Nodes.GetCount() <= 1)
    return 0.0f;

  float len = 0;

  plSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (plUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const plSimdVec4f cur = m_Nodes[i].m_vPosition;

    len += (cur - prev).GetLength<3>();

    prev = cur;
  }

  return len;
}

plSimdVec4f plRopeSimulator::GetPositionAtLength(float length) const
{
  if (m_Nodes.IsEmpty())
    return plSimdVec4f::ZeroVector();

  plSimdVec4f prev = m_Nodes[0].m_vPosition;
  for (plUInt32 i = 1; i < m_Nodes.GetCount(); ++i)
  {
    const plSimdVec4f cur = m_Nodes[i].m_vPosition;

    const plSimdVec4f dir = cur - prev;
    const float dist = dir.GetLength<3>();

    if (length <= dist)
    {
      const float interpolate = length / dist;
      return prev + dir * interpolate;
    }

    length -= dist;
    prev = cur;
  }

  return m_Nodes.PeekBack().m_vPosition;
}

plSimdVec4f plRopeSimulator::MoveTowards(const plSimdVec4f posThis, const plSimdVec4f posNext, plSimdFloat factor, const plSimdVec4f fallbackDir, plSimdFloat& inout_fError)
{
  plSimdVec4f vDir = (posNext - posThis);
  plSimdFloat fLen = vDir.GetLength<3>();

  if (fLen < m_fSegmentLength)
  {
    return plSimdVec4f::ZeroVector();
  }

  vDir /= fLen;
  fLen -= m_fSegmentLength;

  const plSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

plSimdFloat plRopeSimulator::EnforceDistanceConstraint()
{
  // this is the "Jakobsen method" to enforce the distance constraints in each rope node
  // just move each node half the error amount towards the left and right neighboring nodes
  // the ends are either not moved at all (when they are 'attached' to something)
  // or they are moved most of the way
  // this is applied iteratively until the overall error is pretty low

  auto& firstNode = m_Nodes[0];
  auto& lastNode = m_Nodes.PeekBack();

  plSimdFloat fError = plSimdFloat::Zero();

  if (!m_bFirstNodeIsFixed)
  {
    const plSimdVec4f posThis = m_Nodes[0].m_vPosition;
    const plSimdVec4f posNext = m_Nodes[1].m_vPosition;

    m_Nodes[0].m_vPosition += MoveTowards(posThis, posNext, 0.75f, plSimdVec4f(0, 0, 1), fError);
  }

  for (plUInt32 i = 1; i < m_Nodes.GetCount() - 1; ++i)
  {
    const plSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const plSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;
    const plSimdVec4f posNext = m_Nodes[i + 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.5f, plSimdVec4f(0, 0, 1), fError);
    m_Nodes[i].m_vPosition += MoveTowards(posThis, posNext, 0.5f, plSimdVec4f(0, 0, -1), fError);
  }

  if (!m_bLastNodeIsFixed)
  {
    const plUInt32 i = m_Nodes.GetCount() - 1;
    const plSimdVec4f posThis = m_Nodes[i].m_vPosition;
    const plSimdVec4f posPrev = m_Nodes[i - 1].m_vPosition;

    m_Nodes[i].m_vPosition += MoveTowards(posThis, posPrev, 0.75f, plSimdVec4f(0, 0, 1), fError);
  }

  return fError;
}

void plRopeSimulator::UpdateNodePositions(const plSimdFloat tDiffSqr)
{
  const plUInt32 uiFirstNode = m_bFirstNodeIsFixed ? 1 : 0;
  const plUInt32 uiNumNodes = m_bLastNodeIsFixed ? m_Nodes.GetCount() - 1 : m_Nodes.GetCount();

  const plSimdFloat damping = m_fDampingFactor;

  const plSimdVec4f acceleration = plSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (plUInt32 i = uiFirstNode; i < uiNumNodes; ++i)
  {
    // this (simple) logic is the so called 'Verlet integration' (+ damping)

    auto& n = m_Nodes[i];

    const plSimdVec4f previousPos = n.m_vPosition;

    const plSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

    // instead of using a single global acceleration, this could also use individual accelerations per node
    // this would be needed to affect the rope more localized
    n.m_vPosition += vel + acceleration;
    n.m_vPreviousPosition = previousPos;
  }

  if (m_bFirstNodeIsFixed)
  {
    m_Nodes[0].m_vPreviousPosition = m_Nodes[0].m_vPosition;
  }
  if (m_bLastNodeIsFixed)
  {
    m_Nodes.PeekBack().m_vPreviousPosition = m_Nodes.PeekBack().m_vPosition;
  }
}
