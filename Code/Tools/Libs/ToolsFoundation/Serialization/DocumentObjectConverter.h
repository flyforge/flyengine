#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plObjectAccessorBase;

/// \brief Writes the state of an plDocumentObject to an abstract graph.
///
/// This information can then be applied to another plDocument object through plDocumentObjectConverterReader,
/// or to entirely different class using plRttiConverterReader.
class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectConverterWriter
{
public:
  using FilterFunction = plDelegate<bool(const plDocumentObject*, const plAbstractProperty*)>;
  plDocumentObjectConverterWriter(plAbstractObjectGraph* pGraph, const plDocumentObjectManager* pManager, FilterFunction filter = FilterFunction())
  {
    m_pGraph = pGraph;
    m_pManager = pManager;
    m_Filter = filter;
  }

  plAbstractObjectNode* AddObjectToGraph(const plDocumentObject* pObject, const char* szNodeName = nullptr);

private:
  void AddProperty(plAbstractObjectNode* pNode, const plAbstractProperty* pProp, const plDocumentObject* pObject);
  void AddProperties(plAbstractObjectNode* pNode, const plDocumentObject* pObject);

  plAbstractObjectNode* AddSubObjectToGraph(const plDocumentObject* pObject, const char* szNodeName);

  const plDocumentObjectManager* m_pManager;
  plAbstractObjectGraph* m_pGraph;
  FilterFunction m_Filter;
  plSet<const plDocumentObject*> m_QueuedObjects;
};


class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectConverterReader
{
public:
  enum class Mode
  {
    CreateOnly,
    CreateAndAddToDocument,
  };
  plDocumentObjectConverterReader(const plAbstractObjectGraph* pGraph, plDocumentObjectManager* pManager, Mode mode);

  plDocumentObject* CreateObjectFromNode(const plAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const plAbstractObjectNode* pNode, plDocumentObject* pObject);

  plUInt32 GetNumUnknownObjectCreations() const { return m_uiUnknownTypeInstances; }
  const plSet<plString>& GetUnknownObjectTypes() const { return m_UnknownTypes; }

  static void ApplyDiffToObject(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pObject, plDeque<plAbstractGraphDiffOperation>& diff);

private:
  void AddObject(plDocumentObject* pObject, plDocumentObject* pParent, const char* szParentProperty, plVariant index);
  void ApplyProperty(plDocumentObject* pObject, const plAbstractProperty* pProp, const plAbstractObjectNode::Property* pSource);
  static void ApplyDiff(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp,
    plAbstractGraphDiffOperation& op, plDeque<plAbstractGraphDiffOperation>& diff);

  Mode m_Mode;
  plDocumentObjectManager* m_pManager;
  const plAbstractObjectGraph* m_pGraph;
  plSet<plString> m_UnknownTypes;
  plUInt32 m_uiUnknownTypeInstances;
};
