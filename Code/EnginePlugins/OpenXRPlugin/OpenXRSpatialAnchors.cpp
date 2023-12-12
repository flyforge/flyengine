#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>

PLASMA_IMPLEMENT_SINGLETON(plOpenXRSpatialAnchors);

plOpenXRSpatialAnchors::plOpenXRSpatialAnchors(plOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
  PLASMA_ASSERT_DEV(m_pOpenXR->m_extensions.m_bSpatialAnchor, "Spatial anchors not supported");
}

plOpenXRSpatialAnchors::~plOpenXRSpatialAnchors()
{
  for (auto it = m_Anchors.GetIterator(); it.IsValid(); ++it)
  {
    AnchorData anchorData;
    m_Anchors.TryGetValue(it.Id(), anchorData);
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
    XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  }
  m_Anchors.Clear();
}

plXRSpatialAnchorID plOpenXRSpatialAnchors::CreateAnchor(const plTransform& globalTransform)
{
  plWorld* pWorld = m_pOpenXR->GetWorld();
  if (pWorld == nullptr)
    return plXRSpatialAnchorID();

  plTransform globalStageTransform;
  globalStageTransform.SetIdentity();
  if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
  {
    if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
    {
      globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
    }
  }
  plTransform local;
  local.SetLocalTransform(globalStageTransform, globalTransform);

  XrSpatialAnchorCreateInfoMSFT createInfo{XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT};
  createInfo.space = m_pOpenXR->GetBaseSpace();
  createInfo.pose.position = plOpenXR::ConvertPosition(local.m_vPosition);
  createInfo.pose.orientation = plOpenXR::ConvertOrientation(local.m_qRotation);
  createInfo.time = m_pOpenXR->m_frameState.predictedDisplayTime;

  XrSpatialAnchorMSFT anchor;
  XrResult res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorMSFT(m_pOpenXR->m_session, &createInfo, &anchor);
  if (res != XrResult::XR_SUCCESS)
    return plXRSpatialAnchorID();

  XrSpatialAnchorSpaceCreateInfoMSFT createSpaceInfo{XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT};
  createSpaceInfo.anchor = anchor;
  createSpaceInfo.poseInAnchorSpace = plOpenXR::ConvertTransform(plTransform::IdentityTransform());

  XrSpace space;
  res = m_pOpenXR->m_extensions.pfn_xrCreateSpatialAnchorSpaceMSFT(m_pOpenXR->m_session, &createSpaceInfo, &space);

  return m_Anchors.Insert({anchor, space});
}

plResult plOpenXRSpatialAnchors::DestroyAnchor(plXRSpatialAnchorID id)
{
  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return PLASMA_FAILURE;

  XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroySpatialAnchorMSFT(anchorData.m_Anchor));
  XR_LOG_ERROR(xrDestroySpace(anchorData.m_Space));
  m_Anchors.Remove(id);

  return PLASMA_SUCCESS;
}

plResult plOpenXRSpatialAnchors::TryGetAnchorTransform(plXRSpatialAnchorID id, plTransform& out_globalTransform)
{
  plWorld* pWorld = m_pOpenXR->GetWorld();
  if (!pWorld)
    return PLASMA_FAILURE;

  AnchorData anchorData;
  if (!m_Anchors.TryGetValue(id, anchorData))
    return PLASMA_FAILURE;

  const XrTime time = m_pOpenXR->m_frameState.predictedDisplayTime;
  XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
  XrResult res = xrLocateSpace(anchorData.m_Space, m_pOpenXR->m_sceneSpace, time, &viewInScene);
  if (res != XrResult::XR_SUCCESS)
    return PLASMA_FAILURE;

  if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
      (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
  {
    plTransform globalStageTransform;
    globalStageTransform.SetIdentity();
    if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
    {
      if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
      }
    }
    plTransform local(plOpenXR::ConvertPosition(viewInScene.pose.position), plOpenXR::ConvertOrientation(viewInScene.pose.orientation));
    out_globalTransform.SetGlobalTransform(globalStageTransform, local);

    return PLASMA_SUCCESS;
  }
  return PLASMA_FAILURE;
}

PLASMA_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSpatialAnchors);
