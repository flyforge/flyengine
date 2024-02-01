#pragma once

#include <Foundation/Math/Math.h>
#include <Utilities/PathFinding/PathState.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Implements a directed breadth-first search through a graph (A*).
///
/// You can search for a path to a specific location using FindPath() or to the closest node that fulfills some arbitrary criteria
/// using FindClosest().
///
/// PathStateType must be derived from plPathState and can be used for keeping track of certain state along a path and to modify
/// the path search dynamically.
template <typename PathStateType>
class plPathSearch
{
public:
  /// \brief Used by FindClosest() to query whether the currently visited node fulfills the termination criteria.
  using IsSearchedObjectCallback = bool (*)(plInt64 iStartNodeIndex, const PathStateType& StartState);

  /// \brief FindPath() and FindClosest() return an array of these objects as the path result.
  struct PathResultData
  {
    PL_DECLARE_POD_TYPE();

    /// \brief The index of the node that was visited.
    plInt64 m_iNodeIndex;

    /// \brief Pointer to the path state that was active at that step along the path.
    const PathStateType* m_pPathState;
  };

  /// \brief Sets the plPathStateGenerator that should be used by this plPathSearch object.
  void SetPathStateGenerator(plPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when the graph node \a iTargetNodeIndex was reached.
  ///
  /// Returns PL_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  plResult FindPath(plInt64 iStartNodeIndex, const PathStateType& StartState, plInt64 iTargetNodeIndex, plDeque<PathResultData>& out_Path,
    float fMaxPathCost = plMath::Infinity<float>());

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when a graph node is reached for which \a Callback return true.
  ///
  /// Returns PL_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  plResult FindClosest(plInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, plDeque<PathResultData>& out_Path,
    float fMaxPathCost = plMath::Infinity<float>());

  /// \brief Needs to be called by the used plPathStateGenerator to add nodes to evaluate.
  void AddPathNode(plInt64 iNodeIndex, const PathStateType& NewState);

private:
  void ClearPathStates();
  plInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void FillOutPathResult(plInt64 iEndNodeIndex, plDeque<PathResultData>& out_Path);

  plPathStateGenerator<PathStateType>* m_pStateGenerator;

  plHashTable<plInt64, PathStateType> m_PathStates;

  plDeque<plInt64> m_StateQueue;

  plInt64 m_iCurNodeIndex;
  PathStateType m_CurState;
};



#include <Utilities/PathFinding/Implementation/GraphSearch_inl.h>
