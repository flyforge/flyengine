#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

class plAbstractProperty;


///\brief Reflected property step that can be used to init an plPropertyPath
struct PL_FOUNDATION_DLL plPropertyPathStep
{
  plString m_sProperty;
  plVariant m_Index;
};
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plPropertyPathStep);

///\brief Stores a path from an object of a given type to a property inside of it.
/// Once initialized to a specific path, the target property/object of the path can be read or written on
/// multiple root objects.
/// An empty path is allowed in which case WriteToLeafObject/ReadFromLeafObject will return pRootObject directly.
///
/// TODO: read/write methods and ResolvePath should return a failure state.
class PL_FOUNDATION_DLL plPropertyPath
{
public:
  plPropertyPath();
  ~plPropertyPath();

  /// \brief Returns true if InitializeFromPath() has been successfully called and it is therefore possible to use the other functions.
  bool IsValid() const;

  ///\brief Resolves a path in the syntax 'propertyName[index]/propertyName[index]/...' into steps.
  /// The '[index]' part is only added for properties that require indices (arrays and maps).
  plResult InitializeFromPath(const plRTTI& rootObjectRtti, const char* szPath);
  ///\brief Resolves a path provided as an array of plPropertyPathStep.
  plResult InitializeFromPath(const plRTTI* pRootObjectRtti, const plArrayPtr<const plPropertyPathStep> path);

  ///\brief Applies the entire path and allows writing to the target object.
  plResult WriteToLeafObject(void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeaf, const plRTTI& pType)> func) const;
  ///\brief Applies the entire path and allows reading from the target object.
  plResult ReadFromLeafObject(void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeaf, const plRTTI& pType)> func) const;

  ///\brief Applies the path up to the last step and allows a functor to write to the final property.
  plResult WriteProperty(
    void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeafObject, const plRTTI& pLeafType, const plAbstractProperty* pProp, const plVariant& index)> func) const;
  ///\brief Applies the path up to the last step and allows a functor to read from the final property.
  plResult ReadProperty(
    void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeafObject, const plRTTI& pLeafType, const plAbstractProperty* pProp, const plVariant& index)> func) const;

  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  void SetValue(void* pRootObject, const plRTTI& type, const plVariant& value) const;
  ///\brief Convenience function that writes 'value' to the 'pRootObject' at the current path.
  template <typename T>
  PL_ALWAYS_INLINE void SetValue(T* pRootObject, const plVariant& value) const
  {
    SetValue(pRootObject, *plGetStaticRTTI<T>(), value);
  }

  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  void GetValue(void* pRootObject, const plRTTI& type, plVariant& out_value) const;
  ///\brief Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  template <typename T>
  PL_ALWAYS_INLINE void GetValue(T* pRootObject, plVariant& out_value) const
  {
    GetValue(pRootObject, *plGetStaticRTTI<T>(), out_value);
  }

private:
  struct ResolvedStep
  {
    const plAbstractProperty* m_pProperty = nullptr;
    plVariant m_Index;
  };

  static plResult ResolvePath(void* pCurrentObject, const plRTTI* pType, const plArrayPtr<const ResolvedStep> path, bool bWriteToObject,
    const plDelegate<void(void* pLeaf, const plRTTI& pType)>& func);

  bool m_bIsValid = false;
  plHybridArray<ResolvedStep, 2> m_PathSteps;
};
