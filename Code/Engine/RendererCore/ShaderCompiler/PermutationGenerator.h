#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

/// \brief A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class PL_RENDERERCORE_DLL plPermutationGenerator
{
public:
  /// \brief Resets everything.
  void Clear();

  /// \brief Removes all permutations for the given variable
  void RemovePermutations(const plHashedString& sPermVarName);

  /// \brief Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const plHashedString& sName, const plHashedString& sValue);

  /// \brief Returns how many permutations are possible.
  plUInt32 GetPermutationCount() const;

  /// \brief Returns the n-th permutation.
  void GetPermutation(plUInt32 uiPerm, plHybridArray<plPermutationVar, 16>& out_permVars) const;

private:
  plMap<plHashedString, plHashSet<plHashedString>> m_Permutations;
};
