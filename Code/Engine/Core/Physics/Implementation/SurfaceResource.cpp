#include <Core/CorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceResource, 1, plRTTIDefaultAllocator<plSurfaceResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plSurfaceResource);
// clang-format on

plEvent<const plSurfaceResourceEvent&, plMutex> plSurfaceResource::s_Events;

plSurfaceResource::plSurfaceResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plSurfaceResource::~plSurfaceResource()
{
  PL_ASSERT_DEV(m_pPhysicsMaterialPhysX == nullptr, "Physics material has not been cleaned up properly");
  PL_ASSERT_DEV(m_pPhysicsMaterialJolt == nullptr, "Physics material has not been cleaned up properly");
}

plResourceLoadDesc plSurfaceResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  plSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = plSurfaceResourceEvent::Type::Destroyed;
  s_Events.Broadcast(e);

  return res;
}

plResourceLoadDesc plSurfaceResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plSurfaceResource::UpdateContent", GetResourceIdOrDescription());

  m_Interactions.Clear();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }


  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    plSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      plTempHashedString s(i.m_sInteractionType.GetData());
      auto& item = m_Interactions.ExpandAndGetRef();
      item.m_uiInteractionTypeHash = s.GetHash();
      item.m_pInteraction = &i;
    }

    m_Interactions.Sort([](const SurfInt& lhs, const SurfInt& rhs) -> bool {
      if (lhs.m_uiInteractionTypeHash != rhs.m_uiInteractionTypeHash)
        return lhs.m_uiInteractionTypeHash < rhs.m_uiInteractionTypeHash;

      return lhs.m_pInteraction->m_fImpulseThreshold > rhs.m_pInteraction->m_fImpulseThreshold; });
  }

  res.m_State = plResourceState::Loaded;
  return res;
}

void plSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plSurfaceResource, plSurfaceResourceDescriptor)
{
  m_Descriptor = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  plSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = plSurfaceResourceEvent::Type::Created;
  s_Events.Broadcast(e);

  return res;
}

const plSurfaceInteraction* plSurfaceResource::FindInteraction(const plSurfaceResource* pCurSurf, plUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue)
{
  while (true)
  {
    bool bFoundAny = false;

    // try to find a matching interaction
    for (const auto& interaction : pCurSurf->m_Interactions)
    {
      if (interaction.m_uiInteractionTypeHash > uiHash)
        break;

      if (interaction.m_uiInteractionTypeHash == uiHash)
      {
        bFoundAny = true;

        // only use it if the threshold is large enough
        if (fImpulseSqr >= plMath::Square(interaction.m_pInteraction->m_fImpulseThreshold))
        {
          const float fImpulse = plMath::Sqrt(fImpulseSqr);
          out_fImpulseParamValue = (fImpulse - interaction.m_pInteraction->m_fImpulseThreshold) * interaction.m_pInteraction->m_fImpulseScale;

          return interaction.m_pInteraction;
        }
      }
    }

    // if we did find something, we just never exceeded the threshold, then do not search in the base surface
    if (bFoundAny)
      break;

    if (pCurSurf->m_Descriptor.m_hBaseSurface.IsValid())
    {
      plResourceLock<plSurfaceResource> pBase(pCurSurf->m_Descriptor.m_hBaseSurface, plResourceAcquireMode::BlockTillLoaded);
      pCurSurf = pBase.GetPointer();
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

bool plSurfaceResource::InteractWithSurface(plWorld* pWorld, plGameObjectHandle hObject, const plVec3& vPosition, const plVec3& vSurfaceNormal, const plVec3& vIncomingDirection, const plTempHashedString& sInteraction, const plUInt16* pOverrideTeamID, float fImpulseSqr /*= 0.0f*/) const
{
  float fImpulseParam = 0;
  const plSurfaceInteraction* pIA = FindInteraction(this, sInteraction.GetHash(), fImpulseSqr, fImpulseParam);

  if (pIA == nullptr)
    return false;

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  plResourceLock<plPrefabResource> pPrefab(pIA->m_hPrefab, plResourceAcquireMode::BlockTillLoaded);

  plVec3 vDir;

  switch (pIA->m_Alignment)
  {
    case plSurfaceInteractionAlignment::SurfaceNormal:
      vDir = vSurfaceNormal;
      break;

    case plSurfaceInteractionAlignment::IncidentDirection:
      vDir = -vIncomingDirection;
      ;
      break;

    case plSurfaceInteractionAlignment::ReflectedDirection:
      vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;

    case plSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vDir = -vSurfaceNormal;
      break;

    case plSurfaceInteractionAlignment::ReverseIncidentDirection:
      vDir = vIncomingDirection;
      ;
      break;

    case plSurfaceInteractionAlignment::ReverseReflectedDirection:
      vDir = -vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;
  }

  vDir.Normalize();
  plVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();

  // random rotation around the spawn direction
  {
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleInRange(0.0, plMath::Pi<double>() * 2.0);

    plMat3 rotMat = plMat3::MakeAxisRotation(vDir, plAngle::MakeFromRadian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > plAngle::MakeFromRadian(0.0f))
  {
    plAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case plSurfaceInteractionAlignment::IncidentDirection:
      case plSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = plMath::Pi<float>() - plMath::ACos(fCosAngle).GetRadian();

        maxDeviation = plMath::Min(pIA->m_Deviation, plAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      case plSurfaceInteractionAlignment::ReflectedDirection:
      case plSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = plMath::Pi<float>() - plMath::ACos(fCosAngle).GetRadian();

        maxDeviation = plMath::Min(pIA->m_Deviation, plAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const plAngle deviation = plAngle::MakeFromRadian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    plMat3 matTilt = plMat3::MakeAxisRotation(vTangent, deviation);

    vDir = matTilt * vDir;
  }


  // finally compute the bi-tangent
  const plVec3 vBiTangent = vDir.CrossRH(vTangent);

  plMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  plTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation = plQuat::MakeFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  plGameObjectHandle hParent;

  plGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t = plTransform::MakeLocalTransform(pObject->GetGlobalTransform(), t);
  }

  plHybridArray<plGameObject*, 8> rootObjects;

  plPrefabInstantiationOptions options;
  options.m_hParent = hParent;
  options.m_pCreatedRootObjectsOut = &rootObjects;
  options.m_pOverrideTeamID = pOverrideTeamID;

  pPrefab->InstantiatePrefab(*pWorld, t, options, &pIA->m_Parameters);

  {
    plMsgSetFloatParameter msgSetFloat;
    msgSetFloat.m_sParameterName = "Impulse";
    msgSetFloat.m_fValue = fImpulseParam;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msgSetFloat, plTime::MakeZero(), plObjectMsgQueueType::AfterInitialized);
    }
  }

  if (pObject != nullptr && pObject->IsDynamic())
  {
    plMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, plTime::MakeZero(), plObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}

bool plSurfaceResource::IsBasedOn(const plSurfaceResource* pThisOrBaseSurface) const
{
  if (pThisOrBaseSurface == this)
    return true;

  if (m_Descriptor.m_hBaseSurface.IsValid())
  {
    plResourceLock<plSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, plResourceAcquireMode::BlockTillLoaded);

    return pBase->IsBasedOn(pThisOrBaseSurface);
  }

  return false;
}

bool plSurfaceResource::IsBasedOn(const plSurfaceResourceHandle hThisOrBaseSurface) const
{
  plResourceLock<plSurfaceResource> pThisOrBaseSurface(hThisOrBaseSurface, plResourceAcquireMode::BlockTillLoaded);

  return IsBasedOn(pThisOrBaseSurface.GetPointer());
}


PL_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResource);
