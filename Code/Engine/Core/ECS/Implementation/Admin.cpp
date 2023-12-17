#include <Core/ECS/Admin.h>

void plECSAdmin::Update()
{
  for (plECSSystem* system : m_systems)
    system->Update();
}
