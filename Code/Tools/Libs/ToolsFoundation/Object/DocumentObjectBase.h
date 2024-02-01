#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocumentObjectManager;

class PL_TOOLSFOUNDATION_DLL plDocumentObject
{
public:
  plDocumentObject()

    = default;
  virtual ~plDocumentObject() = default;

  // Accessors
  const plUuid& GetGuid() const { return m_Guid; }
  const plRTTI* GetType() const { return GetTypeAccessor().GetType(); }

  const plDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  plDocumentObjectManager* GetDocumentObjectManager() { return m_pDocumentObjectManager; }

  virtual const plIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  plIReflectedTypeAccessor& GetTypeAccessor();

  // Ownership
  const plDocumentObject* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(plDocumentObject* pObject, plStringView sProperty, const plVariant& index);
  virtual void RemoveSubObject(plDocumentObject* pObject);

  // Helper
  void ComputeObjectHash(plUInt64& ref_uiHash) const;
  const plHybridArray<plDocumentObject*, 8>& GetChildren() const { return m_Children; }
  plDocumentObject* GetChild(const plUuid& guid);
  const plDocumentObject* GetChild(const plUuid& guid) const;
  plStringView GetParentProperty() const { return m_sParentProperty; }
  const plAbstractProperty* GetParentPropertyType() const;
  plVariant GetPropertyIndex() const;
  bool IsOnHeap() const;
  plUInt32 GetChildIndex(const plDocumentObject* pChild) const;

private:
  friend class plDocumentObjectManager;
  void HashPropertiesRecursive(const plIReflectedTypeAccessor& acc, plUInt64& uiHash, const plRTTI* pType) const;

protected:
  plUuid m_Guid;
  plDocumentObjectManager* m_pDocumentObjectManager = nullptr;

  plDocumentObject* m_pParent = nullptr;
  plHybridArray<plDocumentObject*, 8> m_Children;

  // Sub object data
  plString m_sParentProperty;
};

class PL_TOOLSFOUNDATION_DLL plDocumentStorageObject : public plDocumentObject
{
public:
  plDocumentStorageObject(const plRTTI* pType)
    : plDocumentObject()
    , m_ObjectPropertiesAccessor(pType, this)
  {
  }

  virtual ~plDocumentStorageObject() = default;

  virtual const plIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }

protected:
  plReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};
