#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plAbstractObjectGraph;
class plAbstractObjectNode;

struct PL_FOUNDATION_DLL plRttiConverterObject
{
  plRttiConverterObject()
    : m_pType(nullptr)
    , m_pObject(nullptr)
  {
  }
  plRttiConverterObject(const plRTTI* pType, void* pObject)
    : m_pType(pType)
    , m_pObject(pObject)
  {
  }

  PL_DECLARE_POD_TYPE();

  const plRTTI* m_pType;
  void* m_pObject;
};


class PL_FOUNDATION_DLL plRttiConverterContext
{
public:
  virtual void Clear();

  /// \brief Generates a guid for a new object. Default implementation generates stable guids derived from
  /// parentGuid + property name + index and ignores the address of pObject.
  virtual plUuid GenerateObjectGuid(const plUuid& parentGuid, const plAbstractProperty* pProp, plVariant index, void* pObject) const;

  virtual plInternal::NewInstance<void> CreateObject(const plUuid& guid, const plRTTI* pRtti);
  virtual void DeleteObject(const plUuid& guid);

  virtual void RegisterObject(const plUuid& guid, const plRTTI* pRtti, void* pObject);
  virtual void UnregisterObject(const plUuid& guid);

  virtual plRttiConverterObject GetObjectByGUID(const plUuid& guid) const;
  virtual plUuid GetObjectGUID(const plRTTI* pRtti, const void* pObject) const;

  virtual const plRTTI* FindTypeByName(plStringView sName) const;

  template <typename T>
  void GetObjectsByType(plDynamicArray<T*>& out_objects, plDynamicArray<plUuid>* out_pUuids = nullptr)
  {
    for (auto it : m_GuidToObject)
    {
      if (it.Value().m_pType->IsDerivedFrom(plGetStaticRTTI<T>()))
      {
        out_objects.PushBack(static_cast<T*>(it.Value().m_pObject));
        if (out_pUuids)
        {
          out_pUuids->PushBack(it.Key());
        }
      }
    }
  }

  virtual plUuid EnqueObject(const plUuid& guid, const plRTTI* pRtti, void* pObject);
  virtual plRttiConverterObject DequeueObject();

  virtual void OnUnknownTypeError(plStringView sTypeName);

protected:
  plHashTable<plUuid, plRttiConverterObject> m_GuidToObject;
  mutable plHashTable<const void*, plUuid> m_ObjectToGuid;
  plSet<plUuid> m_QueuedObjects;
};


class PL_FOUNDATION_DLL plRttiConverterWriter
{
public:
  using FilterFunction = plDelegate<bool(const void* pObject, const plAbstractProperty* pProp)>;

  plRttiConverterWriter(plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs);
  plRttiConverterWriter(plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext, FilterFunction filter);

  plAbstractObjectNode* AddObjectToGraph(plReflectedClass* pObject, const char* szNodeName = nullptr)
  {
    return AddObjectToGraph(pObject->GetDynamicRTTI(), pObject, szNodeName);
  }
  plAbstractObjectNode* AddObjectToGraph(const plRTTI* pRtti, const void* pObject, const char* szNodeName = nullptr);

  void AddProperty(plAbstractObjectNode* pNode, const plAbstractProperty* pProp, const void* pObject);
  void AddProperties(plAbstractObjectNode* pNode, const plRTTI* pRtti, const void* pObject);

  plAbstractObjectNode* AddSubObjectToGraph(const plRTTI* pRtti, const void* pObject, const plUuid& guid, const char* szNodeName);

private:
  plRttiConverterContext* m_pContext = nullptr;
  plAbstractObjectGraph* m_pGraph = nullptr;
  FilterFunction m_Filter;
};

class PL_FOUNDATION_DLL plRttiConverterReader
{
public:
  plRttiConverterReader(const plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext);

  plInternal::NewInstance<void> CreateObjectFromNode(const plAbstractObjectNode* pNode);
  void ApplyPropertiesToObject(const plAbstractObjectNode* pNode, const plRTTI* pRtti, void* pObject);

private:
  void ApplyProperty(void* pObject, const plAbstractProperty* pProperty, const plAbstractObjectNode::Property* pSource);
  void CallOnObjectCreated(const plAbstractObjectNode* pNode, const plRTTI* pRtti, void* pObject);

  plRttiConverterContext* m_pContext = nullptr;
  const plAbstractObjectGraph* m_pGraph = nullptr;
};
