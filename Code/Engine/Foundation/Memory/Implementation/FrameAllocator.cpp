#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

plDoubleBufferedStackAllocator::plDoubleBufferedStackAllocator(plStringView sName0, plAllocatorBase* pParent)
{
  plStringBuilder sName = sName0;
  sName.Append("0");

  m_pCurrentAllocator = PLASMA_DEFAULT_NEW(StackAllocatorType, sName, pParent);

  sName = sName0;
  sName.Append("1");

  m_pOtherAllocator = PLASMA_DEFAULT_NEW(StackAllocatorType, sName, pParent);
}

plDoubleBufferedStackAllocator::~plDoubleBufferedStackAllocator()
{
  PLASMA_DEFAULT_DELETE(m_pCurrentAllocator);
  PLASMA_DEFAULT_DELETE(m_pOtherAllocator);
}

void plDoubleBufferedStackAllocator::Swap()
{
  plMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void plDoubleBufferedStackAllocator::Reset()
{
  m_pCurrentAllocator->Reset();
  m_pOtherAllocator->Reset();
}


// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    plFrameAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plFrameAllocator::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plDoubleBufferedStackAllocator* plFrameAllocator::s_pAllocator;

// static
void plFrameAllocator::Swap()
{
  PLASMA_PROFILE_SCOPE("FrameAllocator.Swap");

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
  s_pAllocator = PLASMA_DEFAULT_NEW(plDoubleBufferedStackAllocator, "FrameAllocator", plFoundation::GetAlignedAllocator());
}

// static
void plFrameAllocator::Shutdown()
{
  PLASMA_DEFAULT_DELETE(s_pAllocator);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
