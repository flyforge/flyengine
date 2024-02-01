#pragma once

#include <Core/World/GameObject.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

/// \brief Data types that are available in visual script. These are a subset of plVariantType.
///
/// Like with plVariantType, the order of these types is important as they are used to determine
/// if a type is "bigger" during type deduction. Also the enum values are serialized in visual script files.
struct PL_VISUALSCRIPTPLUGIN_DLL plVisualScriptDataType
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    Invalid = 0,

    Bool,
    Byte,
    Int,
    Int64,
    Float,
    Double,
    Color,
    Vector3,
    Quaternion,
    Transform,
    Time,
    Angle,
    String,
    HashedString,
    GameObject,
    Component,
    TypedPointer,
    Variant,
    Array,
    Map,
    Coroutine,

    Count,

    EnumValue,

    ExtendedCount,

    AnyPointer = 0xFE,
    Any = 0xFF,

    Default = Invalid,
  };

  PL_ALWAYS_INLINE static bool IsNumber(Enum dataType) { return dataType >= Bool && dataType <= Double; }
  PL_ALWAYS_INLINE static bool IsPointer(Enum dataType) { return (dataType >= GameObject && dataType <= TypedPointer) || dataType == Coroutine; }

  static plVariantType::Enum GetVariantType(Enum dataType);
  static Enum FromVariantType(plVariantType::Enum variantType);

  static plProcessingStream::DataType GetStreamDataType(Enum dataType);

  static const plRTTI* GetRtti(Enum dataType);
  static Enum FromRtti(const plRTTI* pRtti);

  static plUInt32 GetStorageSize(Enum dataType);
  static plUInt32 GetStorageAlignment(Enum dataType);

  static const char* GetName(Enum dataType);

  static bool CanConvertTo(Enum sourceDataType, Enum targetDataType);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_VISUALSCRIPTPLUGIN_DLL, plVisualScriptDataType);

struct PL_VISUALSCRIPTPLUGIN_DLL plVisualScriptGameObjectHandle
{
  plGameObjectHandle m_Handle;
  mutable plGameObject* m_Ptr;
  mutable plUInt32 m_uiExecutionCounter;

  void AssignHandle(const plGameObjectHandle& hObject)
  {
    m_Handle = hObject;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(plGameObject* pObject, plUInt32 uiExecutionCounter)
  {
    m_Handle = pObject != nullptr ? pObject->GetHandle() : plGameObjectHandle();
    m_Ptr = pObject;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  plGameObject* GetPtr(plUInt32 uiExecutionCounter) const;
};

struct PL_VISUALSCRIPTPLUGIN_DLL plVisualScriptComponentHandle
{
  plComponentHandle m_Handle;
  mutable plComponent* m_Ptr;
  mutable plUInt32 m_uiExecutionCounter;

  void AssignHandle(const plComponentHandle& hComponent)
  {
    m_Handle = hComponent;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(plComponent* pComponent, plUInt32 uiExecutionCounter)
  {
    m_Handle = pComponent != nullptr ? pComponent->GetHandle() : plComponentHandle();
    m_Ptr = pComponent;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  plComponent* GetPtr(plUInt32 uiExecutionCounter) const;
};
