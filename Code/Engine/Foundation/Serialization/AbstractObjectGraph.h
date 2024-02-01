#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class plAbstractObjectGraph;

class PL_FOUNDATION_DLL plAbstractObjectNode
{
public:
  struct Property
  {
    plStringView m_sPropertyName;
    plVariant m_Value;
  };

  plAbstractObjectNode()

    = default;

  const plHybridArray<Property, 16>& GetProperties() const { return m_Properties; }

  void AddProperty(plStringView sName, const plVariant& value);

  void RemoveProperty(plStringView sName);

  void ChangeProperty(plStringView sName, const plVariant& value);

  void RenameProperty(plStringView sOldName, plStringView sNewName);

  void ClearProperties();

  // \brief Inlines a custom variant type. Use to patch properties that have been turned into custom variant type.
  // \sa PL_DEFINE_CUSTOM_VARIANT_TYPE, PL_DECLARE_CUSTOM_VARIANT_TYPE
  plResult InlineProperty(plStringView sName);

  const plAbstractObjectGraph* GetOwner() const { return m_pOwner; }
  const plUuid& GetGuid() const { return m_Guid; }
  plUInt32 GetTypeVersion() const { return m_uiTypeVersion; }
  void SetTypeVersion(plUInt32 uiTypeVersion) { m_uiTypeVersion = uiTypeVersion; }
  plStringView GetType() const { return m_sType; }
  void SetType(plStringView sType);

  const Property* FindProperty(plStringView sName) const;
  Property* FindProperty(plStringView sName);

  plStringView GetNodeName() const { return m_sNodeName; }

private:
  friend class plAbstractObjectGraph;

  plAbstractObjectGraph* m_pOwner = nullptr;

  plUuid m_Guid;
  plUInt32 m_uiTypeVersion = 0;
  plStringView m_sType;
  plStringView m_sNodeName;

  plHybridArray<Property, 16> m_Properties;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plAbstractObjectNode);

struct PL_FOUNDATION_DLL plAbstractGraphDiffOperation
{
  enum class Op
  {
    NodeAdded,
    NodeRemoved,
    PropertyChanged
  };

  Op m_Operation;
  plUuid m_Node;            // prop parent or added / deleted node
  plString m_sProperty;     // prop name or type
  plUInt32 m_uiTypeVersion; // only used for NodeAdded
  plVariant m_Value;
};

struct PL_FOUNDATION_DLL plObjectChangeType
{
  using StorageType = plInt8;

  enum Enum : plInt8
  {
    NodeAdded,
    NodeRemoved,
    PropertySet,
    PropertyInserted,
    PropertyRemoved,

    Default = NodeAdded
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plObjectChangeType);


struct PL_FOUNDATION_DLL plDiffOperation
{
  plEnum<plObjectChangeType> m_Operation;
  plUuid m_Node;        // owner of m_sProperty
  plString m_sProperty; // property
  plVariant m_Index;
  plVariant m_Value;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plDiffOperation);


class PL_FOUNDATION_DLL plAbstractObjectGraph
{
public:
  plAbstractObjectGraph() = default;
  ~plAbstractObjectGraph();

  void Clear();

  using FilterFunction = plDelegate<bool(const plAbstractObjectNode*, const plAbstractObjectNode::Property*)>;
  plAbstractObjectNode* Clone(plAbstractObjectGraph& ref_cloneTarget, const plAbstractObjectNode* pRootNode = nullptr, FilterFunction filter = FilterFunction()) const;

  plStringView RegisterString(plStringView sString);

  const plAbstractObjectNode* GetNode(const plUuid& guid) const;
  plAbstractObjectNode* GetNode(const plUuid& guid);

  const plAbstractObjectNode* GetNodeByName(plStringView sName) const;
  plAbstractObjectNode* GetNodeByName(plStringView sName);

  plAbstractObjectNode* AddNode(const plUuid& guid, plStringView sType, plUInt32 uiTypeVersion, plStringView sNodeName = {});
  void RemoveNode(const plUuid& guid);

  const plMap<plUuid, plAbstractObjectNode*>& GetAllNodes() const { return m_Nodes; }
  plMap<plUuid, plAbstractObjectNode*>& GetAllNodes() { return m_Nodes; }

  /// \brief Remaps all node guids by adding the given seed, or if bRemapInverse is true, by subtracting it/
  ///   This is mostly used to remap prefab instance graphs to their prefab template graph.
  void ReMapNodeGuids(const plUuid& seedGuid, bool bRemapInverse = false);

  /// \brief Tries to remap the guids of this graph to those in rhsGraph by walking in both down the hierarchy, starting at root and
  /// rhsRoot.
  ///
  ///  Note that in case of array properties the remapping assumes element indices to be equal
  ///  on both sides which will cause all moves inside the arrays to be lost as there is no way of recovering this information without an
  ///  equality criteria. This function is mostly used to remap a graph from a native object to a graph from plDocumentObjects to allow
  ///  applying native side changes to the original plDocumentObject hierarchy using diffs.
  void ReMapNodeGuidsToMatchGraph(plAbstractObjectNode* pRoot, const plAbstractObjectGraph& rhsGraph, const plAbstractObjectNode* pRhsRoot);

  /// \brief Finds everything accessible by the given root node.
  void FindTransitiveHull(const plUuid& rootGuid, plSet<plUuid>& out_reachableNodes) const;
  /// \brief Deletes everything not accessible by the given root node.
  void PruneGraph(const plUuid& rootGuid);

  /// \brief Allows for a given node to be modified as a native object.
  /// Once the callback exits any changes to the sub-hierarchy of the given root node will be written back to the node objects.
  void ModifyNodeViaNativeCounterpart(plAbstractObjectNode* pRootNode, plDelegate<void(void*, const plRTTI*)> callback);

  /// \brief Allows to copy a node from another graph into this graph.
  plAbstractObjectNode* CopyNodeIntoGraph(const plAbstractObjectNode* pNode);

  plAbstractObjectNode* CopyNodeIntoGraph(const plAbstractObjectNode* pNode, FilterFunction& ref_filter);

  void CreateDiffWithBaseGraph(const plAbstractObjectGraph& base, plDeque<plAbstractGraphDiffOperation>& out_diffResult) const;

  void ApplyDiff(plDeque<plAbstractGraphDiffOperation>& ref_diff);

  void MergeDiffs(const plDeque<plAbstractGraphDiffOperation>& lhs, const plDeque<plAbstractGraphDiffOperation>& rhs, plDeque<plAbstractGraphDiffOperation>& ref_out) const;

private:
  PL_DISALLOW_COPY_AND_ASSIGN(plAbstractObjectGraph);

  void RemapVariant(plVariant& value, const plHashTable<plUuid, plUuid>& guidMap);
  void MergeArrays(const plVariantArray& baseArray, const plVariantArray& leftArray, const plVariantArray& rightArray, plVariantArray& out) const;
  void ReMapNodeGuidsToMatchGraphRecursive(plHashTable<plUuid, plUuid>& guidMap, plAbstractObjectNode* lhs, const plAbstractObjectGraph& rhsGraph, const plAbstractObjectNode* rhs);

  plSet<plString> m_Strings;
  plMap<plUuid, plAbstractObjectNode*> m_Nodes;
  plMap<plStringView, plAbstractObjectNode*> m_NodesByName;
};
