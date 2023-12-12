#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Time.h>

class plStreamWriter;
class plThread;

/// \brief This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro PLASMA_PROFILE_SCOPE provided below.
class PLASMA_FOUNDATION_DLL plProfilingScope
{
public:
  plProfilingScope(plStringView sName, const char* szFunctionName, plTime timeout);
  ~plProfilingScope();

protected:
  plStringView m_sName;
  const char* m_szFunction;
  plTime m_BeginTime;
  plTime m_Timeout;
};

/// \brief This class implements a profiling scope similar to plProfilingScope, but with additional sub-scopes which can be added easily without
/// introducing actual C++ scopes.
///
/// The constructor pushes one surrounding scope on the stack and then a nested scope as the first section.
/// The function StartNextSection() will end the nested scope and start a new inner scope.
/// This allows to end one scope and start a new one, without having to add actual C++ scopes for starting/stopping profiling scopes.
///
/// You shouldn't need to use this directly, just use the macro PLASMA_PROFILE_LIST_SCOPE provided below.
class plProfilingListScope
{
public:
  PLASMA_FOUNDATION_DLL plProfilingListScope(plStringView sListName, plStringView sFirstSectionName, const char* szFunctionName);
  PLASMA_FOUNDATION_DLL ~plProfilingListScope();

  PLASMA_FOUNDATION_DLL static void StartNextSection(plStringView sNextSectionName);

protected:
  static thread_local plProfilingListScope* s_pCurrentList;

  plProfilingListScope* m_pPreviousList;

  plStringView m_sListName;
  const char* m_szListFunction;
  plTime m_ListBeginTime;

  plStringView m_sCurSectionName;
  plTime m_CurSectionBeginTime;
};

/// \brief Helper functionality of the profiling system.
class PLASMA_FOUNDATION_DLL plProfilingSystem
{
public:
  struct ThreadInfo
  {
    plUInt64 m_uiThreadId;
    plString m_sName;
  };

  struct CPUScope
  {
    PLASMA_DECLARE_POD_TYPE();

    static constexpr plUInt32 NAME_SIZE = 40;

    const char* m_szFunctionName;
    plTime m_BeginTime;
    plTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct CPUScopesBufferFlat
  {
    plDynamicArray<CPUScope> m_Data;
    plUInt64 m_uiThreadId = 0;
  };

  /// \brief Helper struct to hold GPU profiling data.
  struct GPUScope
  {
    PLASMA_DECLARE_POD_TYPE();

    static constexpr plUInt32 NAME_SIZE = 48;

    plTime m_BeginTime;
    plTime m_EndTime;
    char m_szName[NAME_SIZE];
  };

  struct PLASMA_FOUNDATION_DLL ProfilingData
  {
    plUInt32 m_uiFramesThreadID = 0;
    plUInt32 m_uiProcessSortIndex = 0;
    plOsProcessID m_uiProcessID = 0;

    plHybridArray<ThreadInfo, 16> m_ThreadInfos;

    plDynamicArray<CPUScopesBufferFlat> m_AllEventBuffers;

    plUInt64 m_uiFrameCount = 0;
    plDynamicArray<plTime> m_FrameStartTimes;

    plDynamicArray<plDynamicArray<GPUScope>> m_GPUScopes;

    /// \brief Writes profiling data as JSON to the output stream.
    plResult Write(plStreamWriter& ref_outputStream) const;

    void Clear();

    /// \brief Concatenates all given ProfilingData instances into one merge struct
    static void Merge(ProfilingData& out_merged, plArrayPtr<const ProfilingData*> inputs);
  };

public:
  static void Clear();

  static void Capture(plProfilingSystem::ProfilingData& out_capture, bool bClearAfterCapture = false);

  /// \brief Scopes are discarded if their duration is shorter than the specified threshold. Default is 0.1ms.
  static void SetDiscardThreshold(plTime threshold);

  using ScopeTimeoutDelegate = plDelegate<void(plStringView sName, plStringView sFunctionName, plTime duration)>;

  /// \brief Sets a callback that is triggered when a profiling scope takes longer than desired.
  static void SetScopeTimeoutCallback(ScopeTimeoutDelegate callback);

  /// \brief Should be called once per frame to capture the timestamp of the new frame.
  static void StartNewFrame();

  /// \brief Adds a new scoped event for the calling thread in the profiling system
  static void AddCPUScope(plStringView sName, const char* szFunctionName, plTime beginTime, plTime endTime, plTime scopeTimeout);

  /// \brief Get current frame counter
  static plUInt64 GetFrameCount();

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend plUInt32 RunThread(plThread* pThread);

  static void Initialize();
  /// \brief Removes profiling data of dead threads.
  static void Reset();

  /// \brief Sets the name of the current thread.
  static void SetThreadName(plStringView sThreadName);
  /// \brief Removes the current thread from the profiling system.
  ///  Needs to be called before the thread exits to be able to release profiling memory of dead threads on Reset.
  static void RemoveThread();

public:
  /// \brief Initialized internal data structures for GPU profiling data. Needs to be called before adding any data.
  static void InitializeGPUData(plUInt32 uiGpuCount = 1);

  /// \brief Adds a GPU profiling scope in the internal event ringbuffer.
  static void AddGPUScope(plStringView sName, plTime beginTime, plTime endTime, plUInt32 uiGpuIndex = 0);
};

#if PLASMA_ENABLED(PLASMA_USE_PROFILING) || defined(PLASMA_DOCS)

/// \brief Profiles the current scope using the given name.
///
/// It is allowed to nest PLASMA_PROFILE_SCOPE, also with PLASMA_PROFILE_LIST_SCOPE. However PLASMA_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa plProfilingScope
/// \sa PLASMA_PROFILE_LIST_SCOPE
#  define PLASMA_PROFILE_SCOPE(szScopeName) plProfilingScope PLASMA_CONCAT(_plProfilingScope, PLASMA_SOURCE_LINE)(szScopeName, PLASMA_SOURCE_FUNCTION, plTime::Zero())


/// \brief Same as PLASMA_PROFILE_SCOPE but if the scope takes longer than 'Timeout', the plProfilingSystem's timeout callback is executed.
///
/// This can be used to log an error or save a callstack, etc. when a scope exceeds an expected amount of time.
/// 
/// \sa plProfilingSystem::SetScopeTimeoutCallback()
#  define PLASMA_PROFILE_SCOPE_WITH_TIMEOUT(szScopeName, Timeout) plProfilingScope PLASMA_CONCAT(_plProfilingScope, PLASMA_SOURCE_LINE)(szScopeName, PLASMA_SOURCE_FUNCTION, Timeout)

/// \brief Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use PLASMA_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest PLASMA_PROFILE_SCOPE, also with PLASMA_PROFILE_LIST_SCOPE. However PLASMA_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa plProfilingListScope
/// \sa PLASMA_PROFILE_LIST_NEXT_SECTION
#  define PLASMA_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) \
    plProfilingListScope PLASMA_CONCAT(_plProfilingScope, PLASMA_SOURCE_LINE)(szListName, szFirstSectionName, PLASMA_SOURCE_FUNCTION)

/// \brief Starts a new section in a PLASMA_PROFILE_LIST_SCOPE
///
/// \sa plProfilingListScope
/// \sa PLASMA_PROFILE_LIST_SCOPE
#  define PLASMA_PROFILE_LIST_NEXT_SECTION(szNextSectionName) plProfilingListScope::StartNextSection(szNextSectionName)

#else

#  define PLASMA_PROFILE_SCOPE(Name) /*empty*/

#  define PLASMA_PROFILE_SCOPE_WITH_TIMEOUT(szScopeName, Timeout) /*empty*/

#  define PLASMA_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) /*empty*/

#  define PLASMA_PROFILE_LIST_NEXT_SECTION(szNextSectionName) /*empty*/

#endif
