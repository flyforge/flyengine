#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tasks/PlaceProbesTask.h>
#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

plResult plBakingScene::Extract()
{
  m_Volumes.Clear();
  m_MeshObjects.Clear();
  m_BoundingBox = plBoundingBox::MakeInvalid();
  m_bIsBaked = false;

  const plWorld* pWorld = plWorld::GetWorld(m_uiWorldIndex);
  if (pWorld == nullptr)
  {
    return PL_FAILURE;
  }

  const plWorld& world = *pWorld;
  PL_LOCK(world.GetReadMarker());

  // settings
  {
    auto pManager = world.GetComponentManager<plBakedProbesComponentManager>();
    auto pComponent = pManager->GetSingletonComponent();

    m_Settings = pComponent->m_Settings;
  }

  // volumes
  {
    if (auto pManager = world.GetComponentManager<plBakedProbesVolumeComponentManager>())
    {
      for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
      {
        if (it->IsActiveAndInitialized())
        {
          plSimdTransform scaledTransform = it->GetOwner()->GetGlobalTransformSimd();
          scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(plSimdConversion::ToVec3(it->GetExtents())) * 0.5f;

          auto& volume = m_Volumes.ExpandAndGetRef();
          volume.m_GlobalToLocalTransform = scaledTransform.GetAsMat4().GetInverse();

          plBoundingBoxSphere globalBounds = it->GetOwner()->GetGlobalBounds();
          if (globalBounds.IsValid())
          {
            m_BoundingBox.ExpandToInclude(globalBounds.GetBox());
          }
        }
      }
    }

    if (m_Volumes.IsEmpty())
    {
      plLog::Error("No Baked Probes Volume found");
      return PL_FAILURE;
    }
  }

  plBoundingBox queryBox = m_BoundingBox;
  queryBox.Grow(plVec3(m_Settings.m_fMaxRayDistance));

  plTagSet excludeTags;
  excludeTags.SetByName("Editor");

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderStatic.GetBitmask();
  queryParams.m_pExcludeTags = &excludeTags;

  plMsgExtractGeometry msg;
  msg.m_Mode = plWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
  msg.m_pMeshObjects = &m_MeshObjects;

  world.GetSpatialSystem()->FindObjectsInBox(queryBox, queryParams, [&](plGameObject* pObject)
    {
    pObject->SendMessage(msg);

    return plVisitorExecution::Continue; });

  return PL_SUCCESS;
}

plResult plBakingScene::Bake(const plStringView& sOutputPath, plProgress& progress)
{
  PL_ASSERT_DEV(!plThreadUtils::IsMainThread(), "BakeScene must be executed on a worker thread");

  if (m_pTracer == nullptr)
  {
    m_pTracer = PL_DEFAULT_NEW(plTracerEmbree);
  }

  plProgressRange pgRange("Baking Scene", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  if (!pgRange.BeginNextStep("Building Scene"))
    return PL_FAILURE;

  PL_SUCCEED_OR_RETURN(m_pTracer->BuildScene(*this));

  plBakingInternal::PlaceProbesTask placeProbesTask(m_Settings, m_BoundingBox, m_Volumes);
  placeProbesTask.Execute();

  plBakingInternal::SkyVisibilityTask skyVisibilityTask(m_Settings, *m_pTracer, placeProbesTask.GetProbePositions());
  skyVisibilityTask.Execute();

  if (!pgRange.BeginNextStep("Writing Result"))
    return PL_FAILURE;

  plStringBuilder sFullOutputPath = sOutputPath;
  sFullOutputPath.Append("_Global.plProbeTreeSector");

  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFullOutputPath));

  plAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  PL_SUCCEED_OR_RETURN(header.Write(file));

  plProbeTreeSectorResourceDescriptor desc;
  desc.m_vGridOrigin = placeProbesTask.GetGridOrigin();
  desc.m_vProbeSpacing = m_Settings.m_vProbeSpacing;
  desc.m_vProbeCount = placeProbesTask.GetProbeCount();
  desc.m_ProbePositions = placeProbesTask.GetProbePositions();
  desc.m_SkyVisibility = skyVisibilityTask.GetSkyVisibility();

  PL_SUCCEED_OR_RETURN(desc.Serialize(file));

  m_bIsBaked = true;

  return PL_SUCCESS;
}

plResult plBakingScene::RenderDebugView(const plMat4& InverseViewProjection, plUInt32 uiWidth, plUInt32 uiHeight, plDynamicArray<plColorGammaUB>& out_Pixels,
  plProgress& progress) const
{
  if (!m_bIsBaked)
    return PL_FAILURE;

  const plUInt32 uiNumPixel = uiWidth * uiHeight;
  out_Pixels.SetCountUninitialized(uiNumPixel);

  plHybridArray<plTracerInterface::Ray, 128> rays;
  rays.SetCountUninitialized(128);

  plHybridArray<plTracerInterface::Hit, 128> hits;
  hits.SetCountUninitialized(128);

  plUInt32 uiStartPixel = 0;
  plUInt32 uiPixelPerBatch = rays.GetCount();
  while (uiStartPixel < uiNumPixel)
  {
    uiPixelPerBatch = plMath::Min(uiPixelPerBatch, uiNumPixel - uiStartPixel);

    for (plUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      plUInt32 uiPixelIndex = uiStartPixel + i;
      plUInt32 x = uiPixelIndex % uiWidth;
      plUInt32 y = uiHeight - (uiPixelIndex / uiWidth) - 1;

      auto& ray = rays[i];
      plGraphicsUtils::ConvertScreenPosToWorldPos(InverseViewProjection, 0, 0, uiWidth, uiHeight, plVec3(float(x), float(y), 0), ray.m_vStartPos, &ray.m_vDir).IgnoreResult();
      ray.m_fDistance = 1000.0f;
    }

    m_pTracer->TraceRays(rays, hits);

    for (plUInt32 i = 0; i < uiPixelPerBatch; ++i)
    {
      plUInt32 uiPixelIndex = uiStartPixel + i;

      auto& hit = hits[i];
      if (hit.m_fDistance >= 0.0f)
      {
        plVec3 normal = hit.m_vNormal * 0.5f + plVec3(0.5f);
        out_Pixels[uiPixelIndex] = plColorGammaUB(plMath::ColorFloatToByte(normal.x), plMath::ColorFloatToByte(normal.y), plMath::ColorFloatToByte(normal.z));
      }
      else
      {
        out_Pixels[uiPixelIndex] = plColorGammaUB(0, 0, 0);
      }
    }

    uiStartPixel += uiPixelPerBatch;

    progress.SetCompletion((float)uiStartPixel / uiNumPixel);
    if (progress.WasCanceled())
      break;
  }

  return PL_SUCCESS;
}

plBakingScene::plBakingScene() = default;
plBakingScene::~plBakingScene() = default;

//////////////////////////////////////////////////////////////////////////

namespace
{
  static plDynamicArray<plUniquePtr<plBakingScene>, plStaticsAllocatorWrapper> s_BakingScenes;
}

PL_IMPLEMENT_SINGLETON(plBaking);

plBaking::plBaking()
  : m_SingletonRegistrar(this)
{
}

void plBaking::Startup()
{
}

void plBaking::Shutdown()
{
  s_BakingScenes.Clear();
}

plBakingScene* plBaking::GetOrCreateScene(const plWorld& world)
{
  const plUInt32 uiWorldIndex = world.GetIndex();

  s_BakingScenes.EnsureCount(uiWorldIndex + 1);
  if (s_BakingScenes[uiWorldIndex] == nullptr)
  {
    auto pScene = PL_DEFAULT_NEW(plBakingScene);
    pScene->m_uiWorldIndex = uiWorldIndex;

    s_BakingScenes[uiWorldIndex] = pScene;
  }

  return s_BakingScenes[uiWorldIndex].Borrow();
}

plBakingScene* plBaking::GetScene(const plWorld& world)
{
  const plUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

const plBakingScene* plBaking::GetScene(const plWorld& world) const
{
  const plUInt32 uiWorldIndex = world.GetIndex();

  if (uiWorldIndex < s_BakingScenes.GetCount())
  {
    return s_BakingScenes[uiWorldIndex].Borrow();
  }

  return nullptr;
}

plResult plBaking::RenderDebugView(const plWorld& world, const plMat4& InverseViewProjection, plUInt32 uiWidth, plUInt32 uiHeight, plDynamicArray<plColorGammaUB>& out_Pixels, plProgress& progress) const
{
  if (const plBakingScene* pScene = GetScene(world))
  {
    return pScene->RenderDebugView(InverseViewProjection, uiWidth, uiHeight, out_Pixels, progress);
  }

  return PL_FAILURE;
}
