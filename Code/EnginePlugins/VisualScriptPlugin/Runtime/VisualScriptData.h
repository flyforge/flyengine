#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptDataDescription : public plRefCounted
{
  struct DataOffset
  {
    PLASMA_DECLARE_POD_TYPE();

    struct PLASMA_VISUALSCRIPTPLUGIN_DLL Source
    {
      enum Enum
      {
        Local,
        Instance,
        Constant,

        Count
      };

      static const char* GetName(Enum source);
    };

    enum
    {
      BYTE_OFFSET_BITS = 24,
      TYPE_BITS = 6,
      SOURCE_BITS = 2,
      INVALID_BYTE_OFFSET = PLASMA_BIT(BYTE_OFFSET_BITS) - 1
    };

    PLASMA_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = INVALID_BYTE_OFFSET;
      m_uiType = plVisualScriptDataType::Invalid;
      m_uiSource = Source::Local;
    }

    PLASMA_ALWAYS_INLINE DataOffset(plUInt32 uiOffset, plVisualScriptDataType::Enum dataType, Source::Enum source)
    {
      m_uiByteOffset = uiOffset;
      m_uiType = dataType;
      m_uiSource = source;
    }

    PLASMA_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != INVALID_BYTE_OFFSET &&
             m_uiType != plVisualScriptDataType::Invalid;
    }

    PLASMA_ALWAYS_INLINE plVisualScriptDataType::Enum GetType() const { return static_cast<plVisualScriptDataType::Enum>(m_uiType); }
    PLASMA_ALWAYS_INLINE Source::Enum GetSource() const { return static_cast<Source::Enum>(m_uiSource); }
    PLASMA_ALWAYS_INLINE bool IsLocal() const { return m_uiSource == Source::Local; }
    PLASMA_ALWAYS_INLINE bool IsInstance() const { return m_uiSource == Source::Instance; }
    PLASMA_ALWAYS_INLINE bool IsConstant() const { return m_uiSource == Source::Constant; }

    PLASMA_ALWAYS_INLINE plResult Serialize(plStreamWriter& inout_stream) const { return inout_stream.WriteDWordValue(this); }
    PLASMA_ALWAYS_INLINE plResult Deserialize(plStreamReader& inout_stream) { return inout_stream.ReadDWordValue(this); }

    plUInt32 m_uiByteOffset : BYTE_OFFSET_BITS;
    plUInt32 m_uiType : TYPE_BITS;
    plUInt32 m_uiSource : SOURCE_BITS;
  };

  struct OffsetAndCount
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiStartOffset = 0;
    plUInt32 m_uiCount = 0;
  };

  OffsetAndCount m_PerTypeInfo[plVisualScriptDataType::Count];
  plUInt32 m_uiStorageSizeNeeded = 0;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  void Clear();
  void CalculatePerTypeStartOffsets();
  void CheckOffset(DataOffset dataOffset, const plRTTI* pType) const;

  DataOffset GetOffset(plVisualScriptDataType::Enum dataType, plUInt32 uiIndex, DataOffset::Source::Enum source) const;
};

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptDataStorage : public plRefCounted
{
public:
  plVisualScriptDataStorage(const plSharedPtr<const plVisualScriptDataDescription>& pDesc);
  ~plVisualScriptDataStorage();

  bool IsAllocated() const;
  void AllocateStorage();
  void DeallocateStorage();

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  using DataOffset = plVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  plTypedPointer GetPointerData(DataOffset dataOffset, plUInt32 uiExecutionCounter) const;

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const plRTTI* pType, plUInt32 uiExecutionCounter);

  plVariant GetDataAsVariant(DataOffset dataOffset, const plRTTI* pExpectedType, plUInt32 uiExecutionCounter) const;
  void SetDataFromVariant(DataOffset dataOffset, const plVariant& value, plUInt32 uiExecutionCounter);

private:
  plSharedPtr<const plVisualScriptDataDescription> m_pDesc;
  plBlob m_Storage;
};

struct plVisualScriptInstanceData
{
  plVisualScriptDataDescription::DataOffset m_DataOffset;
  plVariant m_DefaultValue;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

using plVisualScriptInstanceDataMapping = plRefCountedContainer<plHashTable<plHashedString, plVisualScriptInstanceData>>;

#include <VisualScriptPlugin/Runtime/VisualScriptData_inl.h>
