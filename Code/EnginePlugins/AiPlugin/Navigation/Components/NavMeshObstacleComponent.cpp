#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshObstacleComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plNavMeshObstacleComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(InvalidateSectors),
  }
  PL_END_FUNCTIONS;

  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Navigation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plNavMeshObstacleComponent::plNavMeshObstacleComponent() = default;
plNavMeshObstacleComponent::~plNavMeshObstacleComponent() = default;

void plNavMeshObstacleComponent::OnActivated()
{
  SUPER::OnActivated();

  if (IsSimulationStarted())
    InvalidateSectors();
}

void plNavMeshObstacleComponent::OnSimulationStarted()
{
  plComponent::OnSimulationStarted();

  InvalidateSectors();
}

void plNavMeshObstacleComponent::OnDeactivated()
{
  InvalidateSectors();

  SUPER::OnDeactivated();
}

void plNavMeshObstacleComponent::InvalidateSectors()
{
  // TODO: dynamic obstacles not implemented yet
  if (GetOwner()->IsDynamic())
    return;

  auto* pPhysics = GetWorld()->GetModule<plPhysicsWorldModuleInterface>();
  if (pPhysics == nullptr)
    return;

  auto* pNavMeshModule = GetWorld()->GetOrCreateModule<plAiNavMeshWorldModule>();
  if (pNavMeshModule == nullptr)
    return;

  for (const auto& navConfig : pNavMeshModule->GetConfig().m_NavmeshConfigs)
  {
    plAiNavMesh* pNavMesh = pNavMeshModule->GetNavMesh(navConfig.m_sName);
    plUInt8 uiCollisionLayer = navConfig.m_uiCollisionLayer;

    // TODO: change to plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic when dynamic obstacles are supported
    auto bounds = pPhysics->GetWorldSpaceBounds(GetOwner(), uiCollisionLayer, plPhysicsShapeType::Static, true);
    if (bounds.IsValid())
    {
      pNavMesh->InvalidateSector(bounds.GetBox().GetCenter().GetAsVec2(), bounds.GetBox().GetHalfExtents().GetAsVec2(), false);
    }
  }
}