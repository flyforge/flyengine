#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class plPlayerApplication : public plGameApplication
{
public:
  using SUPER = plGameApplication;

  plPlayerApplication();

protected:
  virtual void Run_InputUpdate() override;
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

private:
  void DetermineProjectPath();
};
