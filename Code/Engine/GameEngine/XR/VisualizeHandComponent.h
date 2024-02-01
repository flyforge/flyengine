#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using plVisualizeHandComponentManager = plComponentManagerSimple<class plVisualizeHandComponent, plComponentUpdateType::WhenSimulating>;

class PL_GAMEENGINE_DLL plVisualizeHandComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plVisualizeHandComponent, plComponent, plVisualizeHandComponentManager);

public:
  plVisualizeHandComponent();
  ~plVisualizeHandComponent();

protected:
  void Update();
};
