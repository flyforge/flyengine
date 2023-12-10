#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class plDocumentObject;
class plObjectAccessorBase;
struct plPropertyReference;
class plStringBuilder;
class plAbstractProperty;

struct PLASMA_EDITORFRAMEWORK_DLL plPropertyReference
{
  bool operator==(const plPropertyReference& rhs) const
  {
    return m_Object == rhs.m_Object && m_pProperty == rhs.m_pProperty && m_Index == rhs.m_Index;
  }
  plUuid m_Object;
  const plAbstractProperty* m_pProperty;
  plVariant m_Index;
};

struct PLASMA_EDITORFRAMEWORK_DLL plObjectPropertyPathContext
{
  const plDocumentObject* m_pContextObject; ///< Paths start at this object.
  plObjectAccessorBase* m_pAccessor;        ///< Accessor used to traverse hierarchy and query properties.
  plString m_sRootProperty;                 ///< In case m_pContextObject points to the root object, this is the property to follow.
};

class PLASMA_EDITORFRAMEWORK_DLL plObjectPropertyPath
{
public:
  static plStatus CreatePath(const plObjectPropertyPathContext& context, const plPropertyReference& prop, plStringBuilder& out_sObjectSearchSequence,
    plStringBuilder& out_sComponentType, plStringBuilder& out_sPropertyPath);
  static plStatus CreatePropertyPath(const plObjectPropertyPathContext& context, const plPropertyReference& prop, plStringBuilder& out_sPropertyPath);

  static plStatus ResolvePath(const plObjectPropertyPathContext& context, plDynamicArray<plPropertyReference>& out_keys,
    const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath);
  static plStatus ResolvePropertyPath(const plObjectPropertyPathContext& context, const char* szPropertyPath, plPropertyReference& out_key);

  static const plDocumentObject* FindParentNodeComponent(const plDocumentObject* pObject);

private:
  static plStatus PrependProperty(
    const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index, plStringBuilder& out_sPropertyPath);
};
