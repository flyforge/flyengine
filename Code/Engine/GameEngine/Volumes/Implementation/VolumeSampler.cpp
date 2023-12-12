#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Volumes/VolumeComponent.h>
#include <GameEngine/Volumes/VolumeSampler.h>

extern plSpatialData::Category s_VolumeCategory;

plVolumeSampler::plVolumeSampler() = default;
plVolumeSampler::~plVolumeSampler() = default;

void plVolumeSampler::RegisterValue(plHashedString sName, plVariant defaultValue, plTime interpolationDuration /*= plTime::Zero()*/)
{
  auto& value = m_Values[sName];
  value.m_DefaultValue = defaultValue;
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

void plVolumeSampler::SampleAtPosition(plWorld& world, const plVec3& vGlobalPosition, plTime deltaTime)
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

  m_TargetValues.Clear();

  auto vPos = plSimdConversion::ToVec3(vGlobalPosition);
  plBoundingSphere sphere(vGlobalPosition, 0.01f);

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = s_VolumeCategory.GetBitmask();

  plHybridArray<ComponentInfo, 16> componentInfos;
  world.GetSpatialSystem()->FindObjectsInSphere(sphere, queryParams, [&](plGameObject* pObject)
    {
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
            plSimdVec4f vAlpha = (plSimdVec4f(1.0f) - absLocalPos).CompDiv(plSimdConversion::ToVec3(pBoxComponent->GetFalloff().CompMax(plVec3(0.0001f))));
            vAlpha = vAlpha.CompMin(plSimdVec4f(1.0f)).CompMax(plSimdVec4f::ZeroVector());
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
          PLASMA_ASSERT_NOT_IMPLEMENTED;
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

  for (auto& info : componentInfos)
  {
    plResourceLock<plBlackboardTemplateResource> blackboardTemplate(info.m_pComponent->GetTemplate(), plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (blackboardTemplate.GetAcquireResult() != plResourceAcquireResult::Final)
      continue;

    auto& desc = blackboardTemplate->GetDescriptor();
    for (auto& entry : desc.m_Entries)
    {
      auto pValue = m_Values.GetValue(entry.m_sName);
      if (pValue == nullptr)
        continue;

      plVariant currentValue;
      if (m_TargetValues.TryGetValue(entry.m_sName, currentValue) == false)
      {
        currentValue = pValue->m_DefaultValue;
      }

      plEnum<plVariantType> targetType = currentValue.GetType();

      plResult conversionStatus = PLASMA_SUCCESS;
      plVariant targetValue = entry.m_InitialValue.ConvertTo(targetType, &conversionStatus);
      if (conversionStatus.Failed())
      {
        plLog::Error("VolumeSampler: Can't convert template value '{}' to '{}'.", entry.m_sName, targetType);
        continue;
      }      

      m_TargetValues[entry.m_sName] = plMath::Lerp(currentValue, targetValue, double(info.m_fAlpha));
    }
  }

  for (auto& it : m_Values)
  {
    auto& value = it.Value();

    plVariant targetValue;
    if (m_TargetValues.TryGetValue(it.Key(), targetValue) == false)
    {
      targetValue = value.m_DefaultValue;
    }

    if (value.m_fInterpolationFactor > 0.0)
    {
      double f = 1.0 - plMath::Pow(1.0 - value.m_fInterpolationFactor, deltaTime.GetSeconds());
      value.m_CurrentValue = plMath::Lerp(value.m_CurrentValue, targetValue, f);
    }
    else
    {
      value.m_CurrentValue = targetValue;
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
