#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Memory/Policies/AllocPolicyGuarding.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

struct AlloctionMetaData
{
  AlloctionMetaData()
  {
    m_uiSize = 0;

    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(m_magic); ++i)
    {
      m_magic[i] = 0x12345678;
    }
  }

  ~AlloctionMetaData()
  {
    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(m_magic); ++i)
    {
      PL_ASSERT_DEV(m_magic[i] == 0x12345678, "Magic value has been overwritten. This might be the result of a buffer underrun!");
    }
  }

  size_t m_uiSize;
  plUInt32 m_magic[32];
};

plAllocPolicyGuarding::plAllocPolicyGuarding(plAllocator* pParent)
{
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  m_uiPageSize = sysInfo.dwPageSize;
}


void* plAllocPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
{
  PL_ASSERT_DEV(plMath::IsPowerOf2((plUInt32)uiAlign), "Alignment must be power of two");
  uiAlign = plMath::Max<size_t>(uiAlign, PL_ALIGNMENT_MINIMUM);

  size_t uiAlignedSize = plMemoryUtils::AlignSize(uiSize, uiAlign);
  size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);

  // align to full pages and add one page in front and one in back
  size_t uiPageSize = m_uiPageSize;
  size_t uiFullPageSize = plMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
  void* pMemory = VirtualAlloc(nullptr, uiFullPageSize + 2 * uiPageSize, MEM_RESERVE, PAGE_NOACCESS);
  PL_ASSERT_DEV(pMemory != nullptr, "Could not reserve memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

  // add one page and commit the payload pages
  pMemory = plMemoryUtils::AddByteOffset(pMemory, uiPageSize);
  void* ptr = VirtualAlloc(pMemory, uiFullPageSize, MEM_COMMIT, PAGE_READWRITE);
  PL_ASSERT_DEV(ptr != nullptr, "Could not commit memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

  // store information in meta data
  AlloctionMetaData* metaData = plMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(ptr), uiFullPageSize - uiTotalSize);
  plMemoryUtils::Construct<SkipTrivialTypes>(metaData, 1);
  metaData->m_uiSize = uiAlignedSize;

  // finally add offset to the actual payload
  ptr = plMemoryUtils::AddByteOffset(metaData, sizeof(AlloctionMetaData));
  return ptr;
}

// deactivate analysis warning for VirtualFree flags, it is needed for the specific functionality
PL_MSVC_ANALYSIS_WARNING_PUSH
PL_MSVC_ANALYSIS_WARNING_DISABLE(6250)

void plAllocPolicyGuarding::Deallocate(void* pPtr)
{
  plLock<plMutex> lock(m_Mutex);

  if (!m_AllocationsToFreeLater.CanAppend())
  {
    void* pMemory = m_AllocationsToFreeLater.PeekFront();
    PL_VERIFY(::VirtualFree(pMemory, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

    m_AllocationsToFreeLater.PopFront();
  }

  // Retrieve info from meta data first.
  AlloctionMetaData* metaData = plMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(pPtr), -((std::ptrdiff_t)sizeof(AlloctionMetaData)));
  size_t uiAlignedSize = metaData->m_uiSize;

  plMemoryUtils::Destruct(metaData, 1);

  // Decommit the pages but do not release the memory yet so use-after-free can be detected.
  size_t uiPageSize = m_uiPageSize;
  size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);
  size_t uiFullPageSize = plMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
  pPtr = plMemoryUtils::AddByteOffset(pPtr, ((std::ptrdiff_t)uiAlignedSize) - uiFullPageSize);

  PL_VERIFY(
    ::VirtualFree(pPtr, uiFullPageSize, MEM_DECOMMIT), "Could not decommit memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

  // Finally store the allocation so we can release it later
  void* pMemory = plMemoryUtils::AddByteOffset(pPtr, -((std::ptrdiff_t)uiPageSize));
  m_AllocationsToFreeLater.PushBack(pMemory);
}

PL_MSVC_ANALYSIS_WARNING_POP

#endif
