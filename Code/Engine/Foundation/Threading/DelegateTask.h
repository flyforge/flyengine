#pragma once

#include <Foundation/Threading/TaskSystem.h>

/// \brief A simple task implementation that calls a delegate function.
template <typename T>
class plDelegateTask final : public plTask
{
public:
  using FunctionType = plDelegate<void(const T&)>;

  plDelegateTask(const char* szTaskName, FunctionType func, const T& param)
  {
    m_Func = func;
    m_param = param;
    ConfigureTask(szTaskName, plTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(m_param); }

  FunctionType m_Func;
  T m_param;
};

template <>
class plDelegateTask<void> final : public plTask
{
public:
  using FunctionType = plDelegate<void()>;

  plDelegateTask(const char* szTaskName, FunctionType func)
  {
    m_Func = func;
    ConfigureTask(szTaskName, plTaskNesting::Never);
  }

private:
  virtual void Execute() override { m_Func(); }

  FunctionType m_Func;
};
