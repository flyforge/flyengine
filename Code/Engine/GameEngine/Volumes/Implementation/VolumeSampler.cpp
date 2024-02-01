#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

plVolumeSampler::plVolumeSampler() = default;
plVolumeSampler::~plVolumeSampler() = default;

void plVolumeSampler::RegisterValue(plHashedString sName, plVariant defaultValue, plTime interpolationDuration /*= plTime::MakeZero()*/)
{
  auto& value = m_Values[sName];
  value.m_DefaultValue = defaultValue;
  value.m_TargetValue = defaultValue;
  value.m_CurrentValue = defaultValue;

  if (interpolationDuration.IsPositive())
  {
    // Reach 90% of target value after interpolation duration:
    // Lerp factor for exponential moving average:
    // y = 1-(1-f)^t
    // solve for f with y = 0.9:
    // f = 1 - 10^(-1 / t)
    value.m_fInterpolationFactor = 1.0 - plMath::Pow(10.0, -1.0 / interpolationDuration.GetSeconds());
  }
  else
  {
    value.m_fInterpolationFactor = -1.0;
  }
}

void plVolumeSampler::DeregisterValue(plHashedString sName)
{
  m_Values.Remove(sName);
}

void plVolumeSampler::DeregisterAllValues()
{
  m_Values.Clear();
}

void plVolumeSampler::SampleAtPosition(const plWorld& world, plSpatialData::Category spatialCategory, const plVec3& vGlobalPosition, plTime deltaTime)
{
  struct ComponentInfo
  {
    const plVolumeComponent* m_pComponent = nullptr;
    plUInt32 m_uiSortingKey = 0;
    float m_fAlpha = 0.0f;

    bool operator<(const ComponentInfo& other) const
    {
      return m_uiSortingKey < other.m_uiSortingKey;
    }
  };

  auto vPos = plSimdConversion::ToVec3(vGlobalPosition);
  plBoundingSphere sphere = plBoundingSphere::MakeFromCenterAndRadius(vGlobalPosition, 0.01f);

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();

  plHybridArray<ComponentInfo, 16> componentInfos;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](plGameObject* pObject) {
      plVolumeComponent* pComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pComponent))
      {
        ComponentInfo info;
        info.m_pComponent = pComponent;

        plSimdTransform scaledTransform = pComponent->GetOwner()->GetGlobalTransformSimd();

        if (auto pBoxComponent = plDynamicCast<const plVolumeBoxComponent*>(pComponent))
        {
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(plSimdConversion::ToVec3(pBoxComponent->GetExtents())) * 0.5f;

          plSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const plSimdVec4f absLocalPos = globalToLocalTransform.TransformPosition(vPos).Abs();
          if ((absLocalPos <= plSimdVec4f(1.0f)).AllSet<3>())
          {
            plSimdVec4f vAlpha = (plSimdVec4f(1.0f) - absLocalPos).CompDiv(plSimdConversion::ToVec3(pBoxComponent->GetFalloff()));
            vAlpha = vAlpha.CompMin(plSimdVec4f(1.0f)).CompMax(plSimdVec4f::MakeZero());
            info.m_fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();
          }
        }
        else if (auto pSphereComponent = plDynamicCast<const plVolumeSphereComponent*>(pComponent))
        {
          scaledTransform.m_Scale *= pSphereComponent->GetRadius();

          plSimdMat4f globalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();
          const plSimdVec4f localPos = globalToLocalTransform.TransformPosition(vPos);
          const float distSquared = localPos.GetLengthSquared<3>();
          if (distSquared <= 1.0f)
          {
            info.m_fAlpha = plMath::Saturate((1.0f - plMath::Sqrt(distSquared)) / pSphereComponent->GetFalloff());
          }
        }
        else
        {
          PL_ASSERT_NOT_IMPLEMENTED;
        }

        if (info.m_fAlpha > 0.0f)
        {
          info.m_uiSortingKey = ComputeSortingKey(pComponent->GetSortOrder(), scaledTransform.GetMaxScale());

          componentInfos.PushBack(info);
        }
      }

      return plVisitorExecution::Continue; });

  // Sort
  {
    componentInfos.Sort();
  }

  for (auto& it : m_Values)
  {
    auto& sName = it.Key();
    auto& value = it.Value();

    value.m_TargetValue = value.m_DefaultValue;

    for (auto& info : componentInfos)
    {
      plVariant volumeValue = info.m_pComponent->GetValue(sName);
      if (volumeValue.IsValid() == false)
        continue;

      plResult conversionStatus = PL_SUCCESS;
      plEnum<plVariantType> targetType = value.m_TargetValue.GetType();
      plVariant newTargetValue = volumeValue.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        plLog::Error("VolumeSampler: Can't convert volume value '{}' to '{}'.", sName, targetType);
        continue;
      }

      value.m_TargetValue = plMath::Lerp(value.m_TargetValue, newTargetValue, double(info.m_fAlpha));
    }

    if (value.m_fInterpolationFactor > 0.0)
    {
      double f = 1.0 - plMath::Pow(1.0 - value.m_fInterpolationFactor, deltaTime.GetSeconds());
      value.m_CurrentValue = plMath::Lerp(value.m_CurrentValue, value.m_TargetValue, f);
    }
    else
    {
      value.m_CurrentValue = value.m_TargetValue;
    }
  }
}

// static
plUInt32 plVolumeSampler::ComputeSortingKey(float fSortOrder, float fMaxScale)
{
  plUInt32 uiSortingKey = (plUInt32)(plMath::Min(fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingKey = (uiSortingKey << 16) | (0xFFFF - ((plUInt32)(fMaxScale * 100.0f) & 0xFFFF));
  return uiSortingKey;
}


