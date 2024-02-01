#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

plDoubleBufferedLinearAllocator::plDoubleBufferedLinearAllocator(plStringView sName0, plAllocator* pParent)
{
  plStringBuilder sName = sName0;
  sName.Append("0");

  m_pCurrentAllocator = PL_DEFAULT_NEW(StackAllocatorType, sName, pParent);

  sName = sName0;
  sName.Append("1");

  m_pOtherAllocator = PL_DEFAULT_NEW(StackAllocatorType, sName, pParent);
}

plDoubleBufferedLinearAllocator::~plDoubleBufferedLinearAllocator()
{
  PL_DEFAULT_DELETE(m_pCurrentAllocator);
  PL_DEFAULT_DELETE(m_pOtherAllocator);
}

void plDoubleBufferedLinearAllocator::Swap()
{
  plMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void plDoubleBufferedLinearAllocator::Reset()
{
  m_pCurrentAllocator->Reset();
  m_pOtherAllocator->Reset();
}


// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    plFrameAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plFrameAllocator::Shutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plDoubleBufferedLinearAllocator* plFrameAllocator::s_pAllocator;

// static
void plFrameAllocator::Swap()
{
  PL_PROFILE_SCOPE("FrameAllocator.Swap");

  s_pAllocator->Swap();
}

// static
void plFrameAllocator::Reset()
{
  if (s_pAllocator)
  {
    s_pAllocator->Reset();
  }
}

// static
void plFrameAllocator::Startup()
{
  s_pAllocator = PL_DEFAULT_NEW(plDoubleBufferedLinearAllocator, "FrameAllocator", plFoundation::GetAlignedAllocator());
}

// static
void plFrameAllocator::Shutdown()
{
  PL_DEFAULT_DELETE(s_pAllocator);
}

PL_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
