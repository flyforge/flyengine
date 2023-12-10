#pragma once

#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class plGameObject;
class plLogInterface;

enum class [[nodiscard]] plAiActionResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again.
};

template <typename TYPE>
class plAiActionAlloc
{
public:
  TYPE* Acquire()
  {
    PLASMA_LOCK(m_Mutex);

    TYPE* pType = nullptr;

    if (m_FreeList.IsEmpty())
    {
      pType = &m_Allocated.ExpandAndGetRef();
    }
    else
    {
      pType = m_FreeList.PeekBack();
      m_FreeList.PopBack();
    }

    pType->Reset();
    return pType;
  }

  void Release(TYPE* pType)
  {
    PLASMA_LOCK(m_Mutex);
    PLASMA_ASSERT_DEBUG(!m_FreeList.Contains(pType), "");
    m_FreeList.PushBack(pType);
  }

private:
  plMutex m_Mutex;
  plHybridArray<TYPE*, 16> m_FreeList;
  plDeque<TYPE> m_Allocated;
};

#define PLASMA_DECLARE_AICMD(OwnType)           \
public:                                     \
  static OwnType* Create()                  \
  {                                         \
    OwnType* pType = s_Allocator.Acquire(); \
    pType->m_bFromAllocator = true;         \
    return pType;                           \
  }                                         \
                                            \
private:                                    \
  virtual void Destroy() override           \
  {                                         \
    Reset();                                \
    if (m_bFromAllocator)                   \
      s_Allocator.Release(this);            \
  }                                         \
  bool m_bFromAllocator = false;            \
  static plAiActionAlloc<OwnType> s_Allocator;

#define PLASMA_IMPLEMENT_AICMD(OwnType) \
  plAiActionAlloc<OwnType> OwnType::s_Allocator;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiAction
{
public:
  plAiAction() = default;
  virtual ~plAiAction() = default;

  virtual void Reset() = 0;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) = 0;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) = 0;
  virtual void Cancel(plGameObject& owner) = 0;

private:
  friend class plAiActionQueue;
  virtual void Destroy() = 0;
};
