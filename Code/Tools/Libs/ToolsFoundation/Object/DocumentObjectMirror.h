#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>


/// \brief An object change starts at the heap object m_Root (because we can only safely store pointers to those).
///  From this object we follow m_Steps (member arrays, structs) to execute m_Change at the end target.
///
/// In case of an NodeAdded operation, m_GraphData contains the entire subgraph of this node.
class PLASMA_TOOLSFOUNDATION_DLL plObjectChange
{
public:
  plObjectChange() {}
  plObjectChange(const plObjectChange&);
  plObjectChange(plObjectChange&& rhs);
  void operator=(plObjectChange&& rhs);
  void operator=(plObjectChange& rhs);
  void GetGraph(plAbstractObjectGraph& graph) const;
  void SetGraph(plAbstractObjectGraph& graph);

  plUuid m_Root;                                //< The object that is the parent of the op, namely the parent heap object we can store a pointer to.
  plHybridArray<plPropertyPathStep, 2> m_Steps; //< Path from root to target of change.
  plDiffOperation m_Change;                     //< Change at the target.
  plDataBuffer m_GraphData;                     //< In case of ObjectAdded, this holds the binary serialized object graph.
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plObjectChange);


class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectMirror
{
public:
  plDocumentObjectMirror();
  virtual ~plDocumentObjectMirror();

  void InitSender(const plDocumentObjectManager* pManager);
  void InitReceiver(plRttiConverterContext* pContext);
  void DeInit();

  typedef plDelegate<bool(const plDocumentObject* pObject, const char* szProperty)> FilterFunction;
  /// \brief
  ///
  /// \param filter
  ///   Filter that defines whether an object property should be mirrored or not.
  void SetFilterFunction(FilterFunction filter);

  void SendDocument();
  void Clear();

  void TreeStructureEventHandler(const plDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e);

  void* GetNativeObjectPointer(const plDocumentObject* pObject);
  const void* GetNativeObjectPointer(const plDocumentObject* pObject) const;

protected:
  bool IsRootObject(const plDocumentObject* pParent);
  bool IsHeapAllocated(const plDocumentObject* pParent, const char* szParentProperty);
  bool IsDiscardedByFilter(const plDocumentObject* pObject, const char* szProperty) const;
  static void CreatePath(plObjectChange& out_change, const plDocumentObject* pRoot, const char* szProperty);
  static plUuid FindRootOpObject(const plDocumentObject* pObject, plHybridArray<const plDocumentObject*, 8>& path);
  static void FlattenSteps(const plArrayPtr<const plDocumentObject* const> path, plHybridArray<plPropertyPathStep, 2>& out_steps);

  virtual void ApplyOp(plObjectChange& change);
  void ApplyOp(plRttiConverterObject object, const plObjectChange& change);

protected:
  plRttiConverterContext* m_pContext;
  const plDocumentObjectManager* m_pManager;
  FilterFunction m_Filter;
};
