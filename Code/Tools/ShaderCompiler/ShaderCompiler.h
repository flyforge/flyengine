#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

class plShaderCompilerApplication : public plGameApplication
{
public:
  typedef plGameApplication SUPER;

  plShaderCompilerApplication();

  virtual plApplication::Execution Run() override;

private:
  void PrintConfig();
  plResult CompileShader(plStringView sShaderFile);
  plResult ExtractPermutationVarValues(plStringView sShaderFile);

  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void Init_LoadProjectPlugins() override {}
  virtual void Init_SetupDefaultResources() override {}
  virtual void Init_ConfigureInput() override {}
  virtual void Init_ConfigureTags() override {}
  virtual bool Run_ProcessApplicationInput() override { return true; }

  plPermutationGenerator m_PermutationGenerator;
  plString m_sPlatforms;
  plString m_sShaderFiles;
  plMap<plString, plHybridArray<plString, 4>> m_FixedPermVars;
};
