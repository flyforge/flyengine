#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plSimpleWindWorldModule);

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleWindWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSimpleWindWorldModule::plSimpleWindWorldModule(plWorld* pWorld)
  : plWindWorldModuleInterface(pWorld)
{
  m_vFallbackWind.SetZero();
}

plSimpleWindWorldModule::~plSimpleWindWorldModule() = default;

plVec3 plSimpleWindWorldModule::GetWindAt(const plVec3& vPosition) const
{
  if (auto pSpatial = GetWorld()->GetSpatialSystem())
  {
    plHybridArray<plGameObject*, 16> volumes;

    plSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = plWindVolumeComponent::SpatialDataCategory.GetBitmask();

    pSpatial->FindObjectsInSphere(plBoundingSphere::MakeFromCenterAndRadius(vPosition, 0.5f), queryParams, volumes);

    const plSimdVec4f pos = plSimdConversion::ToVec3(vPosition);
    plSimdVec4f force = plSimdVec4f::MakeZero();

    for (plGameObject* pObj : volumes)
    {
      plWindVolumeComponent* pVol;
      if (pObj->TryGetComponentOfBaseType(pVol))
      {
        force += pVol->ComputeForceAtGlobalPosition(pos);
      }
    }

    return m_vFallbackWind + plSimdConversion::ToVec3(force);
  }

  return m_vFallbackWind;
}

void plSimpleWindWorldModule::SetFallbackWind(const plVec3& vWind)
{
  m_vFallbackWind = vWind;
}



PL_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindWorldModule);
