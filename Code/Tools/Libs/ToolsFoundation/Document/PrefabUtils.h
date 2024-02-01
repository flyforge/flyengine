#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocumentObject;

class PL_TOOLSFOUNDATION_DLL plPrefabUtils
{
public:
  /// \brief
  static void LoadGraph(plAbstractObjectGraph& out_graph, plStringView sGraph);

  static plAbstractObjectNode* GetFirstRootNode(plAbstractObjectGraph& ref_graph);

  static void GetRootNodes(plAbstractObjectGraph& ref_graph, plHybridArray<plAbstractObjectNode*, 4>& out_nodes);

  static plUuid GetPrefabRoot(const plDocumentObject* pObject, const plObjectMetaData<plUuid, plDocumentObjectMetaData>& documentObjectMetaData, plInt32* pDepth = nullptr);

  static plVariant GetDefaultValue(
    const plAbstractObjectGraph& graph, const plUuid& objectGuid, plStringView sProperty, plVariant index = plVariant(), bool* pValueFound = nullptr);

  static void WriteDiff(const plDeque<plAbstractGraphDiffOperation>& mergedDiff, plStringBuilder& out_sText);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph.
  static void Merge(const plAbstractObjectGraph& baseGraph, const plAbstractObjectGraph& leftGraph, const plAbstractObjectGraph& rightGraph,
    plDeque<plAbstractGraphDiffOperation>& out_mergedDiff);

  /// \brief Merges diffs of left and right graphs relative to their base graph. Conflicts prefer the right graph. Base and left are provided as
  /// serialized DDL graphs and the right graph is build directly from pRight and its PrefabSeed.
  static void Merge(plStringView sBase, plStringView sLeft, plDocumentObject* pRight, bool bRightIsNotPartOfPrefab, const plUuid& prefabSeed,
    plStringBuilder& out_sNewGraph);

  static plString ReadDocumentAsString(plStringView sFile);
};
