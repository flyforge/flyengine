#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <pthread.h>
#include <semaphore.h>

using plThreadHandle = pthread_t;
using plThreadID = pthread_t;
using plMutexHandle = pthread_mutex_t;
using plOSThreadEntryPoint = void* (*)(void* pThreadParameter);

struct plSemaphoreHandle
{
  sem_t* m_pNamedOrUnnamed = nullptr;
  sem_t* m_pNamed = nullptr;
  sem_t m_Unnamed;
};

#define PLASMA_THREAD_CLASS_ENTRY_POINT void* plThreadClassEntryPoint(void* pThreadParameter);

struct plConditionVariableData
{
  pthread_cond_t m_ConditionVariable;
};


/// \endcond
