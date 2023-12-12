#include <Foundation/Logging/Log.h>

template <typename T>
T plObjectAccessorBase::Get(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  plVariant value;
  plStatus res = GetValue(pObject, pProp, value, index);
  if (res.m_Result.Failed())
    plLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

template <typename T>
T plObjectAccessorBase::Get(const plDocumentObject* pObject, const char* szProp, plVariant index /*= plVariant()*/)
{
  plVariant value;
  plStatus res = GetValue(pObject, szProp, value, index);
  if (res.m_Result.Failed())
    plLog::Error("GetValue failed: {0}", res.m_sMessage);
  return value.ConvertTo<T>();
}

inline plInt32 plObjectAccessorBase::GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  plInt32 iCount = 0;
  plStatus res = GetCount(pObject, pProp, iCount);
  if (res.m_Result.Failed())
    plLog::Error("GetCount failed: {0}", res.m_sMessage);
  return iCount;
}
