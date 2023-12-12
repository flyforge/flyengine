#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocumentObjectManager;
class plDocument;

// Prevent conflicts with windows.h
#ifdef GetObject
#  undef GetObject
#endif

/// \brief Standard root object for most documents.
/// m_RootObjects stores what is in the document and m_TempObjects stores transient data used during editing which is not part of the document.
class PLASMA_TOOLSFOUNDATION_DLL plDocumentRoot : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentRoot, plReflectedClass);

  plHybridArray<plReflectedClass*, 1> m_RootObjects;
  plHybridArray<plReflectedClass*, 1> m_TempObjects;
};

/// \brief Implementation detail of plDocumentObjectManager.
class plDocumentRootObject : public plDocumentStorageObject
{
public:
  plDocumentRootObject(const plRTTI* pRootType)
    : plDocumentStorageObject(pRootType)
  {
    m_Guid = plUuid::StableUuidForString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(plDocumentObject* pObject, const char* szProperty, const plVariant& index) override;
  virtual void RemoveSubObject(plDocumentObject* pObject) override;
};

/// \brief Used by plDocumentObjectManager::m_StructureEvents.
struct plDocumentObjectStructureEvent
{
  plDocumentObjectStructureEvent()
    : m_pObject(nullptr)
    , m_pPreviousParent(nullptr)
    , m_pNewParent(nullptr)
  {
  }

  const plAbstractProperty* GetProperty() const;
  plVariant getInsertIndex() const;
  enum class Type
  {
    BeforeReset,
    AfterReset,
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
    AfterObjectMoved2,
  };

  Type m_EventType;
  const plDocument* m_pDocument = nullptr;
  const plDocumentObject* m_pObject = nullptr;
  const plDocumentObject* m_pPreviousParent = nullptr;
  const plDocumentObject* m_pNewParent = nullptr;
  plString m_sParentProperty;
  plVariant m_OldPropertyIndex;
  plVariant m_NewPropertyIndex;
};

/// \brief Used by plDocumentObjectManager::m_PropertyEvents.
struct plDocumentObjectPropertyEvent
{
  plDocumentObjectPropertyEvent() { m_pObject = nullptr; }
  plVariant getInsertIndex() const;

  enum class Type
  {
    PropertySet,
    PropertyInserted,
    PropertyRemoved,
    PropertyMoved,
  };

  Type m_EventType;
  const plDocumentObject* m_pObject;
  plVariant m_OldValue;
  plVariant m_NewValue;
  plString m_sProperty;
  plVariant m_OldIndex;
  plVariant m_NewIndex;
};

/// \brief Used by plDocumentObjectManager::m_ObjectEvents.
struct plDocumentObjectEvent
{
  plDocumentObjectEvent() { m_pObject = nullptr; }

  enum class Type
  {
    BeforeObjectDestroyed,
    AfterObjectCreated,
  };

  Type m_EventType;
  const plDocumentObject* m_pObject;
};

/// \brief Represents to content of a document. Every document has exactly one root object under which all objects need to be parented. The default root object is plDocumentRoot.
class PLASMA_TOOLSFOUNDATION_DLL plDocumentObjectManager
{
public:
  // \brief Storage for the object manager so it can be swapped when using multiple sub documents.
  class Storage : public plRefCounted
  {
  public:
    Storage(const plRTTI* pRootType);

    plDocument* m_pDocument = nullptr;
    plDocumentRootObject m_RootObject;

    plHashTable<plUuid, const plDocumentObject*> m_GuidToObject;

    mutable plCopyOnBroadcastEvent<const plDocumentObjectStructureEvent&> m_StructureEvents;
    mutable plCopyOnBroadcastEvent<const plDocumentObjectPropertyEvent&> m_PropertyEvents;
    plEvent<const plDocumentObjectEvent&> m_ObjectEvents;
  };

public:
  mutable plCopyOnBroadcastEvent<const plDocumentObjectStructureEvent&> m_StructureEvents;
  mutable plCopyOnBroadcastEvent<const plDocumentObjectPropertyEvent&> m_PropertyEvents;
  plEvent<const plDocumentObjectEvent&> m_ObjectEvents;

  plDocumentObjectManager(const plRTTI* pRootType = plDocumentRoot::GetStaticRTTI());
  virtual ~plDocumentObjectManager();
  void SetDocument(plDocument* pDocument) { m_pObjectStorage->m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  plDocumentObject* CreateObject(const plRTTI* pRtti, plUuid guid = plUuid());

  void DestroyObject(plDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const {};

  /// \brief Allows to annotate types with a category (group), such that things like creator menus can use this to present the types in a more user
  /// friendly way
  virtual const char* GetTypeCategory(const plRTTI* pRtti) const { return nullptr; }
  void PatchEmbeddedClassObjects(const plDocumentObject* pObject) const;

  const plDocumentObject* GetRootObject() const { return &m_pObjectStorage->m_RootObject; }
  plDocumentObject* GetRootObject() { return &m_pObjectStorage->m_RootObject; }
  const plDocumentObject* GetObject(const plUuid& guid) const;
  plDocumentObject* GetObject(const plUuid& guid);
  const plDocument* GetDocument() const { return m_pObjectStorage->m_pDocument; }
  plDocument* GetDocument() { return m_pObjectStorage->m_pDocument; }

  // Property Change
  plStatus SetValue(plDocumentObject* pObject, const char* szProperty, const plVariant& newValue, plVariant index = plVariant());
  plStatus InsertValue(plDocumentObject* pObject, const char* szProperty, const plVariant& newValue, plVariant index = plVariant());
  plStatus RemoveValue(plDocumentObject* pObject, const char* szProperty, plVariant index = plVariant());
  plStatus MoveValue(plDocumentObject* pObject, const char* szProperty, const plVariant& oldIndex, const plVariant& newIndex);

  // Structure Change
  void AddObject(plDocumentObject* pObject, plDocumentObject* pParent, const char* szParentProperty, plVariant index);
  void RemoveObject(plDocumentObject* pObject);
  void MoveObject(plDocumentObject* pObject, plDocumentObject* pNewParent, const char* szParentProperty, plVariant index);

  // Structure Change Test
  plStatus CanAdd(const plRTTI* pRtti, const plDocumentObject* pParent, const char* szParentProperty, const plVariant& index) const;
  plStatus CanRemove(const plDocumentObject* pObject) const;
  plStatus CanMove(const plDocumentObject* pObject, const plDocumentObject* pNewParent, const char* szParentProperty, const plVariant& index) const;
  plStatus CanSelect(const plDocumentObject* pObject) const;

  bool IsUnderRootProperty(const char* szRootProperty, const plDocumentObject* pObject) const;
  bool IsUnderRootProperty(const char* szRootProperty, const plDocumentObject* pParent, const char* szParentProperty) const;

  bool IsTemporary(const plDocumentObject* pObject) const;
  bool IsTemporary(const plDocumentObject* pParent, const char* szParentProperty) const;

  plSharedPtr<plDocumentObjectManager::Storage> SwapStorage(plSharedPtr<plDocumentObjectManager::Storage> pNewStorage);
  plSharedPtr<plDocumentObjectManager::Storage> GetStorage() { return m_pObjectStorage; }

private:
  virtual plDocumentObject* InternalCreateObject(const plRTTI* pRtti) { return PLASMA_DEFAULT_NEW(plDocumentStorageObject, pRtti); }
  virtual void InternalDestroyObject(plDocumentObject* pObject) { PLASMA_DEFAULT_DELETE(pObject); }

  void InternalAddObject(plDocumentObject* pObject, plDocumentObject* pParent, const char* szParentProperty, plVariant index);
  void InternalRemoveObject(plDocumentObject* pObject);
  void InternalMoveObject(plDocumentObject* pNewParent, plDocumentObject* pObject, const char* szParentProperty, plVariant index);

  virtual plStatus InternalCanAdd(const plRTTI* pRtti, const plDocumentObject* pParent, const char* szParentProperty, const plVariant& index) const
  {
    return plStatus(PLASMA_SUCCESS);
  };
  virtual plStatus InternalCanRemove(const plDocumentObject* pObject) const { return plStatus(PLASMA_SUCCESS); };
  virtual plStatus InternalCanMove(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, const char* szParentProperty, const plVariant& index) const
  {
    return plStatus(PLASMA_SUCCESS);
  };
  virtual plStatus InternalCanSelect(const plDocumentObject* pObject) const { return plStatus(PLASMA_SUCCESS); };

  void RecursiveAddGuids(plDocumentObject* pObject);
  void RecursiveRemoveGuids(plDocumentObject* pObject);
  void PatchEmbeddedClassObjectsInternal(plDocumentObject* pObject, const plRTTI* pType, bool addToDoc);

private:
  friend class plObjectAccessorBase;

  plSharedPtr<plDocumentObjectManager::Storage> m_pObjectStorage;

  plCopyOnBroadcastEvent<const plDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventsUnsubscriber;
  plCopyOnBroadcastEvent<const plDocumentObjectPropertyEvent&>::Unsubscriber m_PropertyEventsUnsubscriber;
  plEvent<const plDocumentObjectEvent&>::Unsubscriber m_ObjectEventsUnsubscriber;
};
