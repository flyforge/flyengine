#pragma once

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  error "uwp util header should only be included in UWP builds!"
#endif

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <asyncinfo.h>
#include <guiddef.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.h>
#include <windows.foundation.numerics.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/implements.h>
#include <wrl/wrappers/corewrappers.h>

class plUuid;

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

namespace plUwpUtils
{
  /// Helper function to iterate over all elements of a ABI::Windows::Foundation::Collections::IVectorView
  /// \param callback
  ///   Callable of signature bool(UINT index, const ComPtr<Interface>& pElement). Return value of false means discontinue.
  template <typename ElementQueryType, typename ElementType, typename Callback>
  HRESULT plWinRtIterateIVectorView(
    const ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ElementType>>& pVectorView, const Callback& callback)
  {
    UINT numElements = 0;
    HRESULT result = pVectorView->get_Size(&numElements);
    if (FAILED(result))
      return result;

    for (UINT i = 0; i < numElements; ++i)
    {
      ElementQueryType pElement;
      result = pVectorView->GetAt(i, &pElement);
      if (FAILED(result))
        return result;

      if (!callback(i, pElement))
        return S_OK;
    }

    return result;
  }

  /// Helper function to iterate over all elements of a ABI::Windows::Foundation::Collections::IMapView
  /// \param callback
  ///   Callable of signature bool(const KeyType& key, const ValueInterfaceType& value). Return value of false means discontinue.
  template <typename ValueInterfaceType, typename KeyType, typename ValueType, typename Callback>
  void plWinRtIterateIMapView(const ComPtr<ABI::Windows::Foundation::Collections::IMapView<KeyType, ValueType>>& pMapView, const Callback& callback)
  {
    using namespace ABI::Windows::Foundation::Collections;
    using namespace ABI::Windows::Foundation::Internal;

    using MapInterface = IIterable<IKeyValuePair<KeyType, GetLogicalType<ValueType>::type>*>;

    ComPtr<MapInterface> pIterable;
    pMapView->QueryInterface<MapInterface>(&pIterable);

    PLASMA_ASSERT_DEBUG(pIterable != nullptr, "Invalid map iteration interface");

    ComPtr<IIterator<IKeyValuePair<KeyType, ValueType>*>> iterator;
    pIterable->First(&iterator);

    boolean hasCurrent = FALSE;
    iterator->get_HasCurrent(&hasCurrent);

    while (hasCurrent == TRUE)
    {
      IKeyValuePair<KeyType, ValueType>* pair;
      iterator->get_Current(&pair);

      KeyType key;
      pair->get_Key(&key);

      ValueInterfaceType value;
      pair->get_Value(&value);

      if (!callback(key, value))
        return;

      iterator->MoveNext(&hasCurrent);
    }
  }

  /// \brief Helper function to correctly execute 'put_Completed' on 'object'.
  ///
  /// \param callback: A lambda of type void func(AsyncResultType param1);
  /// The callback gets the final result of the completed async function. Currently only when the async operation succeeds.
  ///
  /// AsyncCompletedType is the type of the async function parameter. Unfortunately it cannot be deduced and therefore
  /// has to be passed in manually.
  ///
  /// Search the code for usage examples.
  template <typename AsyncCompletedType, typename AsyncResultType, typename Callback, typename ObjectType>
  HRESULT plWinRtPutCompleted(ObjectType& object, Callback callback)
  {
    object->put_Completed(Microsoft::WRL::Callback<ABI::Windows::Foundation::IAsyncOperationCompletedHandler<AsyncCompletedType>>(
      [callback](ABI::Windows::Foundation::IAsyncOperation<AsyncCompletedType>* pCompletion, AsyncStatus status) {
        if (status != ABI::Windows::Foundation::AsyncStatus::Completed)
          return S_OK;

        AsyncResultType pResult;
        pCompletion->GetResults(&pResult);

        callback(pResult);

        return S_OK;
      }).Get());

    return S_OK;
  }

  /// \brief Helper functions to get the statics / activation factory for a runtime class.
  template <typename T>
  void RetrieveStatics(const WCHAR* szRuntimeClassName, ComPtr<T>& out_Ptr)
  {
    if (FAILED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(szRuntimeClassName).Get(), &out_Ptr)))
    {
      PLASMA_REPORT_FAILURE("Failed to retrieve activation factory (statics) for '{0}'", plStringUtf8(szRuntimeClassName).GetData());
      out_Ptr = nullptr;
    }
  }

  /// \brief Helper function to create an instance of a runtime class type that does not have a factory function.
  ///
  /// \note Only use this function when there is no other way to create an instance of a type.
  /// Most types can be instantiated through the statics / activation factory.
  template <typename T>
  void CreateInstance(const WCHAR* szRuntimeClassName, ComPtr<T>& out_Ptr)
  {
    ::RoActivateInstance(HStringReference(szRuntimeClassName).Get(), &out_Ptr);
  }

  plMat4 PLASMA_FOUNDATION_DLL ConvertMat4(const ABI::Windows::Foundation::Numerics::Matrix4x4& in);

  plVec3 PLASMA_FOUNDATION_DLL ConvertVec3(const ABI::Windows::Foundation::Numerics::Vector3& in);
  void PLASMA_FOUNDATION_DLL ConvertVec3(const plVec3& in, ABI::Windows::Foundation::Numerics::Vector3& out);

  plQuat PLASMA_FOUNDATION_DLL ConvertQuat(const ABI::Windows::Foundation::Numerics::Quaternion& in);
  void PLASMA_FOUNDATION_DLL ConvertQuat(const plQuat& in, ABI::Windows::Foundation::Numerics::Quaternion& out);

  plUuid PLASMA_FOUNDATION_DLL ConvertGuid(const GUID& in);
  void PLASMA_FOUNDATION_DLL ConvertGuid(const plUuid& in, GUID& out);
} // namespace plUwpUtils
