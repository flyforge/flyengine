#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>

plOpenDdlReader::plOpenDdlReader()
{
  m_pCurrentChunk = nullptr;
  m_uiBytesInChunkLeft = 0;
}

plOpenDdlReader::~plOpenDdlReader()
{
  ClearDataChunks();
}

plResult plOpenDdlReader::ParseDocument(plStreamReader& inout_stream, plUInt32 uiFirstLineOffset, plLogInterface* pLog, plUInt32 uiCacheSizeInKB)
{
  PLASMA_ASSERT_DEBUG(m_ObjectStack.IsEmpty(), "A reader can only be used once.");

  SetLogInterface(pLog);
  SetCacheSize(uiCacheSizeInKB);
  SetInputStream(inout_stream, uiFirstLineOffset);

  m_TempCache.Reserve(s_uiChunkSize);

  plOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = plOpenDdlPrimitiveType::Custom;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_sCustomType = CopyString("root");
  pElement->m_sName = nullptr;
  pElement->m_uiNumChildElements = 0;

  m_ObjectStack.PushBack(pElement);

  return ParseAll();
}

const plOpenDdlReaderElement* plOpenDdlReader::GetRootElement() const
{
  PLASMA_ASSERT_DEBUG(!m_ObjectStack.IsEmpty(), "The reader has not parsed any document yet or an error occurred during parsing.");

  return m_ObjectStack[0];
}


const plOpenDdlReaderElement* plOpenDdlReader::FindElement(plStringView sGlobalName) const
{
  return m_GlobalNames.GetValueOrDefault(sGlobalName, nullptr);
}

plStringView plOpenDdlReader::CopyString(const plStringView& string)
{
  if (string.IsEmpty())
    return {};

  // no idea how to make this more efficient without running into lots of other problems
  m_Strings.PushBack(string);
  return m_Strings.PeekBack();
}

plOpenDdlReaderElement* plOpenDdlReader::CreateElement(plOpenDdlPrimitiveType type, plStringView sType, plStringView sName, bool bGlobalName)
{
  plOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = type;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_sCustomType = sType;
  pElement->m_sName = CopyString(sName);
  pElement->m_uiNumChildElements = 0;

  if (bGlobalName)
  {
    pElement->m_uiNumChildElements = PLASMA_BIT(31);
  }

  if (bGlobalName && !sName.IsEmpty())
  {
    m_GlobalNames[sName] = pElement;
  }

  plOpenDdlReaderElement* pParent = m_ObjectStack.PeekBack();
  pParent->m_uiNumChildElements++;

  if (pParent->m_pFirstChild == nullptr)
  {
    pParent->m_pFirstChild = pElement;
    pParent->m_pLastChild = pElement;
  }
  else
  {
    ((plOpenDdlReaderElement*)pParent->m_pLastChild)->m_pSiblingElement = pElement;
    pParent->m_pLastChild = pElement;
  }

  m_ObjectStack.PushBack(pElement);

  return pElement;
}


void plOpenDdlReader::OnBeginObject(plStringView sType, plStringView sName, bool bGlobalName)
{
  CreateElement(plOpenDdlPrimitiveType::Custom, CopyString(sType), sName, bGlobalName);
}

void plOpenDdlReader::OnEndObject()
{
  m_ObjectStack.PopBack();
}

void plOpenDdlReader::OnBeginPrimitiveList(plOpenDdlPrimitiveType type, plStringView sName, bool bGlobalName)
{
  CreateElement(type, nullptr, sName, bGlobalName);

  m_TempCache.Clear();
}

void plOpenDdlReader::OnEndPrimitiveList()
{
  // if we had to temporarily store the primitive data, copy it into a new destination
  if (!m_TempCache.IsEmpty())
  {
    plUInt8* pTarget = AllocateBytes(m_TempCache.GetCount());
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;

    plMemoryUtils::Copy(pTarget, m_TempCache.GetData(), m_TempCache.GetCount());
  }

  m_ObjectStack.PopBack();
}

void plOpenDdlReader::StorePrimitiveData(bool bThisIsAll, plUInt32 bytecount, const plUInt8* pData)
{
  plUInt8* pTarget = nullptr;

  if (!bThisIsAll || !m_TempCache.IsEmpty())
  {
    // if this is not all, accumulate the data in a temp buffer
    plUInt32 offset = m_TempCache.GetCount();
    m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + bytecount);
    pTarget = &m_TempCache[offset]; // have to index m_TempCache after the resize, otherwise it could be empty and not like it
  }
  else
  {
    // otherwise, allocate the final storage immediately
    pTarget = AllocateBytes(bytecount);
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;
  }

  plMemoryUtils::Copy(pTarget, pData, bytecount);
}


void plOpenDdlReader::OnPrimitiveBool(plUInt32 count, const bool* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(bool) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveInt8(plUInt32 count, const plInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plInt8) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveInt16(plUInt32 count, const plInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plInt16) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveInt32(plUInt32 count, const plInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plInt32) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveInt64(plUInt32 count, const plInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plInt64) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveUInt8(plUInt32 count, const plUInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plUInt8) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveUInt16(plUInt32 count, const plUInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plUInt16) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveUInt32(plUInt32 count, const plUInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plUInt32) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveUInt64(plUInt32 count, const plUInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(plUInt64) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveFloat(plUInt32 count, const float* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(float) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveDouble(plUInt32 count, const double* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(double) * count, (const plUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void plOpenDdlReader::OnPrimitiveString(plUInt32 count, const plStringView* pData, bool bThisIsAll)
{
  const plUInt32 uiDataSize = count * sizeof(plStringView);

  const plUInt32 offset = m_TempCache.GetCount();
  m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + uiDataSize);
  plStringView* pTarget = (plStringView*)&m_TempCache[offset];

  for (plUInt32 i = 0; i < count; ++i)
  {
    pTarget[i] = CopyString(pData[i]);
  }

  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}


void plOpenDdlReader::OnParsingError(plStringView sMessage, bool bFatal, plUInt32 uiLine, plUInt32 uiColumn)
{
  if (bFatal)
  {
    m_ObjectStack.Clear();
    m_GlobalNames.Clear();
    m_Elements.Clear();

    ClearDataChunks();
  }
}

//////////////////////////////////////////////////////////////////////////

void plOpenDdlReader::ClearDataChunks()
{
  for (plUInt32 i = 0; i < m_DataChunks.GetCount(); ++i)
  {
    PLASMA_DEFAULT_DELETE(m_DataChunks[i]);
  }

  m_DataChunks.Clear();
}

plUInt8* plOpenDdlReader::AllocateBytes(plUInt32 uiNumBytes)
{
  uiNumBytes = plMemoryUtils::AlignSize(uiNumBytes, static_cast<plUInt32>(PLASMA_ALIGNMENT_MINIMUM));

  // if the requested data is very large, just allocate it as an individual chunk
  if (uiNumBytes > s_uiChunkSize / 2)
  {
    plUInt8* pResult = PLASMA_DEFAULT_NEW_ARRAY(plUInt8, uiNumBytes).GetPtr();
    m_DataChunks.PushBack(pResult);
    return pResult;
  }

  // if our current chunk is too small, discard the remaining free bytes and just allocate a new chunk
  if (m_uiBytesInChunkLeft < uiNumBytes)
  {
    m_pCurrentChunk = PLASMA_DEFAULT_NEW_ARRAY(plUInt8, s_uiChunkSize).GetPtr();
    m_uiBytesInChunkLeft = s_uiChunkSize;
    m_DataChunks.PushBack(m_pCurrentChunk);
  }

  // no fulfill the request from the current chunk
  plUInt8* pResult = m_pCurrentChunk;
  m_pCurrentChunk += uiNumBytes;
  m_uiBytesInChunkLeft -= uiNumBytes;

  return pResult;
}

//////////////////////////////////////////////////////////////////////////

plUInt32 plOpenDdlReaderElement::GetNumChildObjects() const
{
  if (m_PrimitiveType != plOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~PLASMA_BIT(31)); // Bit 31 stores whether the name is global
}

plUInt32 plOpenDdlReaderElement::GetNumPrimitives() const
{
  if (m_PrimitiveType == plOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~PLASMA_BIT(31)); // Bit 31 stores whether the name is global
}


bool plOpenDdlReaderElement::HasPrimitives(plOpenDdlPrimitiveType type, plUInt32 uiMinNumberOfPrimitives /*= 1*/) const
{
  /// \test This is new

  if (m_PrimitiveType != type)
    return false;

  return m_uiNumChildElements >= uiMinNumberOfPrimitives;
}

const plOpenDdlReaderElement* plOpenDdlReaderElement::FindChild(plStringView sName) const
{
  PLASMA_ASSERT_DEBUG(m_PrimitiveType == plOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const plOpenDdlReaderElement* pChild = static_cast<const plOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetName() == sName)
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const plOpenDdlReaderElement* plOpenDdlReaderElement::FindChildOfType(plOpenDdlPrimitiveType type, plStringView sName, plUInt32 uiMinNumberOfPrimitives /* = 1*/) const
{
  /// \test This is new

  PLASMA_ASSERT_DEBUG(m_PrimitiveType == plOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const plOpenDdlReaderElement* pChild = static_cast<const plOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == type && pChild->GetName() == sName)
    {
      if (type == plOpenDdlPrimitiveType::Custom || pChild->GetNumPrimitives() >= uiMinNumberOfPrimitives)
        return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const plOpenDdlReaderElement* plOpenDdlReaderElement::FindChildOfType(plStringView sType, plStringView sName /*= {}*/) const
{
  const plOpenDdlReaderElement* pChild = static_cast<const plOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == plOpenDdlPrimitiveType::Custom && pChild->GetCustomType() == sType && (sName.IsEmpty() || pChild->GetName() == sName))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlReader);
