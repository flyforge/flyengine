#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

PL_WARNING_PUSH()
PL_WARNING_DISABLE_MSVC(4355)

#ifndef PL_THREAD_CLASS_ENTRY_POINT
#  error "Definition for plThreadClassEntryPoint is missing on this platform!"
#endif

PL_THREAD_CLASS_ENTRY_POINT;

struct plThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the plThread instance
    ThreadDestroyed,   ///< Called on the thread that destroys the plThread instance
    StartingExecution, ///< Called on the thread that executes the plThread instance
    FinishedExecution, ///< Called on the thread that executes the plThread instance
  };

  Type m_Type;
  plThread* m_pThread = nullptr;
};

/// \brief This class is the base class for platform independent long running threads
///
/// Used by deriving from this class and overriding the Run() method.
class PL_FOUNDATION_DLL plThread : public plOSThread
{
public:
  /// \brief Describes the thread status
  enum plThreadStatus
  {
    Created = 0,
    Running,
    Finished
  };

  /// \brief Initializes the runnable class
  plThread(const char* szName = "plThread", plUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~plThread();

  /// \brief Returns the thread status
  inline plThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// \brief Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// \brief Returns the thread name
  inline const char* GetThreadName() const { return m_sName.GetData(); }

  /// \brief These events inform about threads starting and finishing.
  ///
  /// The events are raised on the executing thread! That means thread-specific code may be executed during the event callback,
  /// e.g. to set up thread-local functionality.
  static plEvent<const plThreadEvent&, plMutex> s_ThreadEvents;

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual plUInt32 Run() = 0;


  volatile plThreadStatus m_ThreadStatus = Created;

  plString m_sName;

  friend plUInt32 RunThread(plThread* pThread);
};

PL_WARNING_POP()
