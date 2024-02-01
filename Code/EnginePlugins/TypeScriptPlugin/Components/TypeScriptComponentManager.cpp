#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>

plTypeScriptComponentManager::plTypeScriptComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

plTypeScriptComponentManager::~plTypeScriptComponentManager() = default;

void plTypeScriptComponentManager::Initialize()
{
  SUPER::Initialize();

  auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plTypeScriptComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}

void plTypeScriptComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
}

void plTypeScriptComponentManager::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_TsBinding.Initialize(*GetWorld()).IgnoreResult();
}

void plTypeScriptComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  PL_PROFILE_SCOPE("TypeScript Update");

  m_TsBinding.Update();

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
    {
      it->Update(m_TsBinding);
    }
  }

  m_TsBinding.CleanupStash(10);
}
