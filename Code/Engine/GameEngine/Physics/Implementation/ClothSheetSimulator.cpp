#include <GameEngine/GameEnginePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>

void plClothSimulator::SimulateCloth(const plTime& diff)
{
  m_LeftOverTimeStep += diff;

  constexpr plTime tStep = plTime::MakeFromSeconds(1.0 / 60.0);
  const plSimdFloat tStepSqr = static_cast<float>(tStep.GetSeconds() * tStep.GetSeconds());

  while (m_LeftOverTimeStep >= tStep)
  {
    SimulateStep(tStepSqr, 32, m_vSegmentLength.x);

    m_LeftOverTimeStep -= tStep;
  }
}

void plClothSimulator::SimulateStep(const plSimdFloat fDiffSqr, plUInt32 uiMaxIterations, plSimdFloat fAllowedError)
{
  if (m_Nodes.GetCount() < 4)
    return;

  UpdateNodePositions(fDiffSqr);

  // repeatedly apply the distance constraint, until the overall error is low enough
  for (plUInt32 i = 0; i < uiMaxIterations; ++i)
  {
    const plSimdFloat fError = EnforceDistanceConstraint();

    if (fError < fAllowedError)
      return;
  }
}

plSimdFloat plClothSimulator::EnforceDistanceConstraint()
{
  plSimdFloat fError = plSimdFloat::MakeZero();

  for (plUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (plUInt32 x = 0; x < m_uiWidth; ++x)
    {
      const plUInt32 idx = (y * m_uiWidth) + x;

      auto& n = m_Nodes[idx];

      if (n.m_bFixed)
        continue;

      const plSimdVec4f posThis = n.m_vPosition;

      if (x > 0)
      {
        const plSimdVec4f pos = m_Nodes[idx - 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, plSimdVec4f(-1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (x + 1 < m_uiWidth)
      {
        const plSimdVec4f pos = m_Nodes[idx + 1].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, plSimdVec4f(1, 0, 0), fError, m_vSegmentLength.x);
      }

      if (y > 0)
      {
        const plSimdVec4f pos = m_Nodes[idx - m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, plSimdVec4f(0, -1, 0), fError, m_vSegmentLength.y);
      }

      if (y + 1 < m_uiHeight)
      {
        const plSimdVec4f pos = m_Nodes[idx + m_uiWidth].m_vPosition;
        n.m_vPosition += MoveTowards(posThis, pos, 0.5f, plSimdVec4f(0, 1, 0), fError, m_vSegmentLength.y);
      }
    }
  }

  return fError;
}

plSimdVec4f plClothSimulator::MoveTowards(const plSimdVec4f posThis, const plSimdVec4f posNext, plSimdFloat factor, const plSimdVec4f fallbackDir, plSimdFloat& inout_fError, plSimdFloat fSegLen)
{
  plSimdVec4f vDir = (posNext - posThis);
  plSimdFloat fLen = vDir.GetLength<3>();

  if (fLen.IsEqual(plSimdFloat::MakeZero(), 0.001f))
  {
    vDir = fallbackDir;
    fLen = 1;
  }

  vDir /= fLen;
  fLen -= fSegLen;

  const plSimdFloat fLocalError = fLen * factor;

  vDir *= fLocalError;

  // keep track of how much the rope had to be moved to fulfill the constraint
  inout_fError += fLocalError.Abs();

  return vDir;
}

void plClothSimulator::UpdateNodePositions(const plSimdFloat tDiffSqr)
{
  const plSimdFloat damping = m_fDampingFactor;
  const plSimdVec4f acceleration = plSimdConversion::ToVec3(m_vAcceleration) * tDiffSqr;

  for (auto& n : m_Nodes)
  {
    if (n.m_bFixed)
    {
      n.m_vPreviousPosition = n.m_vPosition;
    }
    else
    {
      // this (simple) logic is the so called 'Verlet integration' (+ damping)

      const plSimdVec4f previousPos = n.m_vPosition;

      const plSimdVec4f vel = (n.m_vPosition - n.m_vPreviousPosition) * damping;

      // instead of using a single global acceleration, this could also use individual accelerations per node
      // this would be needed to affect the rope more localized
      n.m_vPosition += vel + acceleration;
      n.m_vPreviousPosition = previousPos;
    }
  }
}

bool plClothSimulator::HasEquilibrium(plSimdFloat fAllowedMovement) const
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


