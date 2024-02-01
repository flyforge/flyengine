#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

void plPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}


void plPermutationGenerator::RemovePermutations(const plHashedString& sPermVarName)
{
  m_Permutations.Remove(sPermVarName);
}

void plPermutationGenerator::AddPermutation(const plHashedString& sName, const plHashedString& sValue)
{
  PL_ASSERT_DEV(!sName.IsEmpty(), "");
  PL_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
}

plUInt32 plPermutationGenerator::GetPermutationCount() const
{
  plUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void plPermutationGenerator::GetPermutation(plUInt32 uiPerm, plHybridArray<plPermutationVar, 16>& out_permVars) const
{
  out_permVars.Clear();

  for (auto itVariable = m_Permutations.GetIterator(); itVariable.IsValid(); ++itVariable)
  {
    const plUInt32 uiValues = itVariable.Value().GetCount();
    plUInt32 uiUseValue = uiPerm % uiValues;

    uiPerm /= uiValues;

    auto itValue = itVariable.Value().GetIterator();

    for (; uiUseValue > 0; --uiUseValue)
    {
      ++itValue;
    }

    plPermutationVar& pv = out_permVars.ExpandAndGetRef();
    pv.m_sName = itVariable.Key();
    pv.m_sValue = itValue.Key();
  }
}


