#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Logging/Log.h>

/// \brief Represents a single 'object' in a DDL document, e.g. either a custom type or a primitives list.
class PLASMA_FOUNDATION_DLL plOpenDdlReaderElement
{
public:
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Whether this is a custom object type that typically contains sub-elements.
  PLASMA_ALWAYS_INLINE bool IsCustomType() const { return m_PrimitiveType == plOpenDdlPrimitiveType::Custom; } // [tested]

  /// \brief Whether this is a custom object type of the requested type.
  PLASMA_ALWAYS_INLINE bool IsCustomType(plStringView sTypeName) const
  {
    return m_PrimitiveType == plOpenDdlPrimitiveType::Custom && m_sCustomType == sTypeName;
  }

  /// \brief Returns the string for the custom type name.
  PLASMA_ALWAYS_INLINE plStringView GetCustomType() const { return m_sCustomType; } // [tested]

  /// \brief Whether the name of the object is non-empty.
  PLASMA_ALWAYS_INLINE bool HasName() const { return !m_sName.IsEmpty(); } // [tested]

  /// \brief Returns the name of the object.
  PLASMA_ALWAYS_INLINE plStringView GetName() const { return m_sName; } // [tested]

  /// \brief Returns whether the element name is a global or a local name.
  PLASMA_ALWAYS_INLINE bool IsNameGlobal() const { return (m_uiNumChildElements & PLASMA_BIT(31)) != 0; } // [tested]

  /// \brief How many sub-elements the object has.
  plUInt32 GetNumChildObjects() const; // [tested]

  /// \brief If this is a custom type element, the returned pointer is to the first child element.
  PLASMA_ALWAYS_INLINE const plOpenDdlReaderElement* GetFirstChild() const
  {
    return reinterpret_cast<const plOpenDdlReaderElement*>(m_pFirstChild);
  } // [tested]

  /// \brief If the parent is a custom type element, the next child after this is returned.
  PLASMA_ALWAYS_INLINE const plOpenDdlReaderElement* GetSibling() const { return m_pSiblingElement; } // [tested]

  /// \brief For non-custom types this returns how many primitives are stored at this element.
  plUInt32 GetNumPrimitives() const; // [tested]

  /// \brief For non-custom types this returns the type of primitive that is stored at this element.
  PLASMA_ALWAYS_INLINE plOpenDdlPrimitiveType GetPrimitivesType() const { return m_PrimitiveType; } // [tested]

  /// \brief Returns true if the element stores the requested type of primitives AND has at least the desired amount of them, so that accessing the
  /// data array at certain indices is safe.
  bool HasPrimitives(plOpenDdlPrimitiveType type, plUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const bool* GetPrimitivesBool() const { return reinterpret_cast<const bool*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plInt8* GetPrimitivesInt8() const { return reinterpret_cast<const plInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plInt16* GetPrimitivesInt16() const { return reinterpret_cast<const plInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plInt32* GetPrimitivesInt32() const { return reinterpret_cast<const plInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plInt64* GetPrimitivesInt64() const { return reinterpret_cast<const plInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plUInt8* GetPrimitivesUInt8() const { return reinterpret_cast<const plUInt8*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plUInt16* GetPrimitivesUInt16() const { return reinterpret_cast<const plUInt16*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plUInt32* GetPrimitivesUInt32() const { return reinterpret_cast<const plUInt32*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plUInt64* GetPrimitivesUInt64() const { return reinterpret_cast<const plUInt64*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const float* GetPrimitivesFloat() const { return reinterpret_cast<const float*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const double* GetPrimitivesDouble() const { return reinterpret_cast<const double*>(m_pFirstChild); } // [tested]

  /// \brief Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  PLASMA_ALWAYS_INLINE const plStringView* GetPrimitivesString() const { return reinterpret_cast<const plStringView*>(m_pFirstChild); } // [tested]

  /// \brief Searches for a child with the given name. It does not matter whether the object's name is 'local' or 'global'.
  /// \a szName is case-sensitive.
  const plOpenDdlReaderElement* FindChild(plStringView sName) const; // [tested]

  /// \brief Searches for a child element that has the given type, name and if it is a primitives list, at least the desired number of primitives.
  const plOpenDdlReaderElement* FindChildOfType(plOpenDdlPrimitiveType type, plStringView sName, plUInt32 uiMinNumberOfPrimitives = 1) const;

  /// \brief Searches for a child element with the given type and optionally also a certain name.
  const plOpenDdlReaderElement* FindChildOfType(plStringView sType, plStringView sName = nullptr) const;

private:
  friend class plOpenDdlReader;

  plOpenDdlPrimitiveType m_PrimitiveType = plOpenDdlPrimitiveType::Custom;
  plUInt32 m_uiNumChildElements = 0;
  const void* m_pFirstChild = nullptr;
  const plOpenDdlReaderElement* m_pLastChild = nullptr;
  plStringView m_sCustomType;
  plStringView m_sName;
  const plOpenDdlReaderElement* m_pSiblingElement = nullptr;
};

/// \brief An OpenDDL reader parses an entire DDL document and creates an in-memory representation of the document structure.
class PLASMA_FOUNDATION_DLL plOpenDdlReader : public plOpenDdlParser
{
public:
  plOpenDdlReader();
  ~plOpenDdlReader();

  /// \brief Parses the given document, returns PLASMA_FAILURE if an unrecoverable parsing error was encountered.
  ///
  /// \param stream is the input data.
  /// \param uiFirstLineOffset allows to adjust the reported line numbers in error messages, in case the given stream represents a sub-section of a
  /// larger file. \param pLog is used for outputting details about parsing errors. If nullptr is given, no details are logged. \param uiCacheSizeInKB
  /// is the internal cache size that the parser uses. If the parsed documents contain primitives lists with several thousand elements in a single
  /// list, increasing the cache size can improve performance, but typically this doesn't need to be adjusted.
  plResult ParseDocument(plStreamReader& inout_stream, plUInt32 uiFirstLineOffset = 0, plLogInterface* pLog = plLog::GetThreadLocalLogSystem(),
    plUInt32 uiCacheSizeInKB = 4); // [tested]

  /// \brief Every document has exactly one root element.
  const plOpenDdlReaderElement* GetRootElement() const; // [tested]

  /// \brief Searches for an element with a global name. NULL if there is no such element.
  const plOpenDdlReaderElement* FindElement(plStringView sGlobalName) const; // [tested]

protected:
  virtual void OnBeginObject(plStringView sType, plStringView sName, bool bGlobalName) override;
  virtual void OnEndObject() override;

  virtual void OnBeginPrimitiveList(plOpenDdlPrimitiveType type, plStringView sName, bool bGlobalName) override;
  virtual void OnEndPrimitiveList() override;

  virtual void OnPrimitiveBool(plUInt32 count, const bool* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveInt8(plUInt32 count, const plInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt16(plUInt32 count, const plInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt32(plUInt32 count, const plInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt64(plUInt32 count, const plInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveUInt8(plUInt32 count, const plUInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt16(plUInt32 count, const plUInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt32(plUInt32 count, const plUInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt64(plUInt32 count, const plUInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveFloat(plUInt32 count, const float* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveDouble(plUInt32 count, const double* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveString(plUInt32 count, const plStringView* pData, bool bThisIsAll) override;

  virtual void OnParsingError(plStringView sMessage, bool bFatal, plUInt32 uiLine, plUInt32 uiColumn) override;

protected:
  plOpenDdlReaderElement* CreateElement(plOpenDdlPrimitiveType type, plStringView sType, plStringView sName, bool bGlobalName);
  plStringView CopyString(const plStringView& string);
  void StorePrimitiveData(bool bThisIsAll, plUInt32 bytecount, const plUInt8* pData);

  void ClearDataChunks();
  plUInt8* AllocateBytes(plUInt32 uiNumBytes);

  static const plUInt32 s_uiChunkSize = 1000 * 4; // 4 KiB

  plHybridArray<plUInt8*, 16> m_DataChunks;
  plUInt8* m_pCurrentChunk;
  plUInt32 m_uiBytesInChunkLeft;

  plDynamicArray<plUInt8> m_TempCache;

  plDeque<plOpenDdlReaderElement> m_Elements;
  plHybridArray<plOpenDdlReaderElement*, 16> m_ObjectStack;

  plDeque<plString> m_Strings;

  plMap<plString, plOpenDdlReaderElement*> m_GlobalNames;
};
