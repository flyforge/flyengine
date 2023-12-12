#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/OSFile.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_MEMORY_MAPPED_FILE)

PLASMA_CREATE_SIMPLE_TEST(IO, MemoryMappedFile)
{
  plStringBuilder sOutputFile = plTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO");
  sOutputFile.AppendPath("MemoryMappedFile.dat");

  const plUInt32 uiFileSize = 1024 * 1024 * 16; // * 4

  // generate test data
  {
    plOSFile file;
    if (!PLASMA_TEST_BOOL_MSG(file.Open(sOutputFile, plFileOpenMode::Write).Succeeded(), "File for memory mapping could not be created"))
      return;

    plDynamicArray<plUInt32> data;
    data.SetCountUninitialized(uiFileSize);

    for (plUInt32 i = 0; i < uiFileSize; ++i)
    {
      data[i] = i;
    }

    file.Write(data.GetData(), data.GetCount() * sizeof(plUInt32)).IgnoreResult();
    file.Close();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Memory map for writing")
  {
    plMemoryMappedFile memFile;

    if (!PLASMA_TEST_BOOL_MSG(memFile.Open(sOutputFile, plMemoryMappedFile::Mode::ReadWrite).Succeeded(), "Memory mapping a file failed"))
      return;

    PLASMA_TEST_BOOL(memFile.GetWritePointer() != nullptr);
    PLASMA_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(plUInt32));

    plUInt32* ptr = static_cast<plUInt32*>(memFile.GetWritePointer());

    for (plUInt32 i = 0; i < uiFileSize; ++i)
    {
      PLASMA_TEST_INT(ptr[i], i);
      ptr[i] = ptr[i] + 1;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Memory map for reading")
  {
    plMemoryMappedFile memFile;

    if (!PLASMA_TEST_BOOL_MSG(memFile.Open(sOutputFile, plMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file failed"))
      return;

    PLASMA_TEST_BOOL(memFile.GetReadPointer() != nullptr);
    PLASMA_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(plUInt32));

    const plUInt32* ptr = static_cast<const plUInt32*>(memFile.GetReadPointer());

    for (plUInt32 i = 0; i < uiFileSize; ++i)
    {
      PLASMA_TEST_INT(ptr[i], i + 1);
    }

    // try to map it a second time
    plMemoryMappedFile memFile2;

    if (!PLASMA_TEST_BOOL_MSG(memFile2.Open(sOutputFile, plMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file twice failed"))
      return;
  }

  plOSFile::DeleteFile(sOutputFile).IgnoreResult();
}
#endif
