#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

typedef plComponentManagerSimple<class plVisualizeHandComponent, plComponentUpdateType::WhenSimulating> plVisualizeHandComponentManager;

class PLASMA_GAMEENGINE_DLL plVisualizeHandComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plVisualizeHandComponent, plComponent, plVisualizeHandComponentManager);

public:
  plVisualizeHandComponent();
  ~plVisualizeHandComponent();

protected:
  void Update();
};
