#pragma once

#include <Core/CoreDLL.h>

class PLASMA_CORE_DLL plECSSystem
{
public:

  plECSSystem() = default;
  ~plECSSystem() = default;

  virtual void Initialize() = 0;
  virtual void Update() = 0;

  plECSAdmin m_admin;
};
