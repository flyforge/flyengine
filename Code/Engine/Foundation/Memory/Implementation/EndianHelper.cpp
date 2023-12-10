#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/EndianHelper.h>

void plEndianHelper::SwitchStruct(void* pDataPointer, const char* szFormat)
{
  PLASMA_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  PLASMA_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");

  plUInt8* pWorkPointer = static_cast<plUInt8*>(pDataPointer);
  char cCurrentElement = *szFormat;

  while (cCurrentElement != '\0')
  {
    switch (cCurrentElement)
    {
      case 'c':
      case 'b':
        pWorkPointer++;
        break;

      case 's':
      case 'w':
      {
        plUInt16* pWordElement = reinterpret_cast<plUInt16*>(pWorkPointer);
        *pWordElement = Switch(*pWordElement);
        pWorkPointer += sizeof(plUInt16);
      }
      break;

      case 'd':
      {
        plUInt32* pDWordElement = reinterpret_cast<plUInt32*>(pWorkPointer);
        *pDWordElement = Switch(*pDWordElement);
        pWorkPointer += sizeof(plUInt32);
      }
      break;

      case 'q':
      {
        plUInt64* pQWordElement = reinterpret_cast<plUInt64*>(pWorkPointer);
        *pQWordElement = Switch(*pQWordElement);
        pWorkPointer += sizeof(plUInt64);
      }
      break;
    }

    szFormat++;
    cCurrentElement = *szFormat;
  }
}

void plEndianHelper::SwitchStructs(void* pDataPointer, const char* szFormat, plUInt32 uiStride, plUInt32 uiCount)
{
  PLASMA_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  PLASMA_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");
  PLASMA_ASSERT_DEBUG(uiStride > 0, "Struct size necessary!");

  for (plUInt32 i = 0; i < uiCount; i++)
  {
    SwitchStruct(pDataPointer, szFormat);
    pDataPointer = plMemoryUtils::AddByteOffset(pDataPointer, uiStride);
  }
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_EndianHelper);
