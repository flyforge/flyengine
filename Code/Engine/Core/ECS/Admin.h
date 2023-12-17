#pragma once

#include <Core/CoreDLL.h>

#include <Core/ECS/System.h>

#include <Foundation/Containers/DynamicArray.h>

class PLASMA_CORE_DLL plECSAdmin
{
public:
  plECSAdmin() = default;
  ~plECSAdmin() = default;

  void Update();

private:
  plDynamicArray<plECSSystem*> m_systems;


};
