#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocument;
struct plDocumentObjectStructureEvent;

struct plSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
  };

  Type m_Type;
  const plDocument* m_pDocument;
  const plDocumentObject* m_pObject;
};

/// \brief Selection Manager stores a set of selected document objects.
class PLASMA_TOOLSFOUNDATION_DLL plSelectionManager
{
public:
  plCopyOnBroadcastEvent<const plSelectionManagerEvent&> m_Events;

  // \brief Storage for the selection so it can be swapped when using multiple sub documents.
  class Storage : public plRefCounted
  {
  public:
    plDeque<const plDocumentObject*> m_SelectionList;
    plSet<plUuid> m_SelectionSet;
    const plDocumentObjectManager* m_pObjectManager = nullptr;
    plCopyOnBroadcastEvent<const plSelectionManagerEvent&> m_Events;
  };

public:
  plSelectionManager(const plDocumentObjectManager* pObjectManager);
  ~plSelectionManager();

  void Clear();
  void AddObject(const plDocumentObject* pObject);
  void RemoveObject(const plDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const plDocumentObject* pSingleObject);
  void SetSelection(const plDeque<const plDocumentObject*>& Selection);
  void ToggleObject(const plDocumentObject* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const plDocumentObject* GetCurrentObject() const;

  /// \brief Returns the selection in the same order the objects were added to the list.
  const plDeque<const plDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected. I.e. if an object is selected and one of its ancestors is selected, it is culled from the list. Items are returned in the order of appearance in an expanded scene tree.
  const plDeque<const plDocumentObject*> GetTopLevelSelection() const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  const plDeque<const plDocumentObject*> GetTopLevelSelection(const plRTTI* pBase) const;

  bool IsSelected(const plDocumentObject* pObject) const;
  bool IsParentSelected(const plDocumentObject* pObject) const;

  const plDocument* GetDocument() const;

  plSharedPtr<plSelectionManager::Storage> SwapStorage(plSharedPtr<plSelectionManager::Storage> pNewStorage);
  plSharedPtr<plSelectionManager::Storage> GetStorage() { return m_pSelectionStorage; }

private:
  void TreeEventHandler(const plDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const plDocumentObject* pObject);

  friend class plDocument;

  plSharedPtr<plSelectionManager::Storage> m_pSelectionStorage;

  plCopyOnBroadcastEvent<const plDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  plCopyOnBroadcastEvent<const plSelectionManagerEvent&>::Unsubscriber m_EventsUnsubscriber;
};
