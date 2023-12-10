#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief Implementation of a thread.
///
/// Since the thread class needs a platform specific entry-point it is usually
/// recommended to use the plThread class instead as the base for long running threads.
class PLASMA_FOUNDATION_DLL plOSThread
{
public:
  /// \brief Initializes the thread instance (e.g. thread creation etc.)
  ///
  /// Note that the thread won't start execution until Start() is called. Please note that szName must be valid until Start() has been
  /// called!
  plOSThread(plOSThreadEntryPoint threadEntryPoint, void* pUserData = nullptr, const char* szName = "plOSThread", plUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor.
  virtual ~plOSThread();

  /// \brief Starts the thread
  void Start(); // [tested]

  /// \brief Waits in the calling thread until the thread has finished execution (e.g. returned from the thread function)
  void Join(); // [tested]

  /// \brief Returns the thread ID of the thread object, may be used in comparison operations with plThreadUtils::GetCurrentThreadID() for
  /// example.
  const plThreadID& GetThreadID() const { return m_ThreadID; }

  /// \brief Returns how many plOSThreads are currently active.
  static plInt32 GetThreadCount() { return s_iThreadCount; }

protected:
  plThreadHandle m_hHandle;
  plThreadID m_ThreadID;

  plOSThreadEntryPoint m_EntryPoint;

  void* m_pUserData;

  const char* m_szName;

  plUInt32 m_uiStackSize;


private:
  /// Stores how many plOSThread are currently active.
  static plAtomicInteger32 s_iThreadCount;

  PLASMA_DISALLOW_COPY_AND_ASSIGN(plOSThread);
};
