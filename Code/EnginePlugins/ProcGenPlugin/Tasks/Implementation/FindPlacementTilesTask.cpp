#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/FindPlacementTilesTask.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plCVarFloat cvar_ProcGenCullingDistanceScale("ProcGen.Culling.DistanceScale", 1.0f, plCVarFlags::Default, "Global scale to control cull distance for all placement outputs");
plCVarInt cvar_ProcGenCullingMaxRadius("ProcGen.Culling.MaxRadius", 10, plCVarFlags::Default, "Maximum cull radius in number of tiles");

using namespace plProcGenInternal;

FindPlacementTilesTask::FindPlacementTilesTask(plProcPlacementComponent* pComponent, plUInt32 uiOutputIndex)
  : m_pComponent(pComponent)
  , m_uiOutputIndex(uiOutputIndex)
{
  plStringBuilder sName;
  sName.Format("UpdateTiles {}", m_pComponent->m_OutputContexts[m_uiOutputIndex].m_pOutput->m_sName);

  ConfigureTask(sName, plTaskNesting::Never);
}

FindPlacementTilesTask::~FindPlacementTilesTask() = default;

void FindPlacementTilesTask::Execute()
{
  m_NewTiles.Clear();
  m_OldTileKeys.Clear();

  plHybridArray<plSimdMat4f, 8, plAlignedAllocatorWrapper> globalToLocalBoxTransforms;

  auto& outputContext = m_pComponent->m_OutputContexts[m_uiOutputIndex];

  const float fTileSize = outputContext.m_pOutput->GetTileSize();
  const float fPatternSize = outputContext.m_pOutput->m_pPattern->m_fSize;
  const float fCullDistance = outputContext.m_pOutput->m_fCullDistance * cvar_ProcGenCullingDistanceScale;

  float fRadius = plMath::Min(plMath::Ceil(fCullDistance / fTileSize + 1.0f), static_cast<float>(cvar_ProcGenCullingMaxRadius));
  plInt32 iRadius = static_cast<plInt32>(fRadius);
  plInt32 iRadiusSqr = iRadius * iRadius;

  plSimdVec4f fHalfTileSize = plSimdVec4f(fTileSize * 0.5f);

  for (plVec3 vCameraPosition : m_CameraPositions)
  {
    plVec3 cameraPos = vCameraPosition / fTileSize;
    float fPosX = plMath::Round(cameraPos.x);
    float fPosY = plMath::Round(cameraPos.y);
    plInt32 iPosX = static_cast<plInt32>(fPosX);
    plInt32 iPosY = static_cast<plInt32>(fPosY);

    float fY = (fPosY - fRadius) * fTileSize;
    plInt32 iY = -iRadius;

    while (iY <= iRadius)
    {
      float fX = (fPosX - fRadius) * fTileSize;
      plInt32 iX = -iRadius;

      while (iX <= iRadius)
      {
        if (iX * iX + iY * iY <= iRadiusSqr)
        {
          plUInt64 uiTileKey = GetTileKey(iPosX + iX, iPosY + iY);
          if (auto pTile = outputContext.m_TileIndices.GetValue(uiTileKey))
          {
            pTile->m_uiLastSeenFrame = plRenderWorld::GetFrameCounter();
          }
          else
          {
            plSimdVec4f testPos = plSimdVec4f(fX, fY, 0.0f);
            plSimdFloat minZ = 10000.0f;
            plSimdFloat maxZ = -10000.0f;

            globalToLocalBoxTransforms.Clear();

            for (auto& bounds : m_pComponent->m_Bounds)
            {
              plSimdBBox extendedBox = bounds.m_GlobalBoundingBox;
              extendedBox.Grow(fHalfTileSize);

              if (((testPos >= extendedBox.m_Min) && (testPos <= extendedBox.m_Max)).AllSet<2>())
              {
                minZ = minZ.Min(bounds.m_GlobalBoundingBox.m_Min.z());
                maxZ = maxZ.Max(bounds.m_GlobalBoundingBox.m_Max.z());

                globalToLocalBoxTransforms.PushBack(bounds.m_GlobalToLocalBoxTransform);
              }
            }

            if (!globalToLocalBoxTransforms.IsEmpty())
            {
              plProcPlacementComponent::OutputContext::TileIndexAndAge emptyTile;
              emptyTile.m_uiIndex = NewTileIndex;
              emptyTile.m_uiLastSeenFrame = plRenderWorld::GetFrameCounter();

              outputContext.m_TileIndices.Insert(uiTileKey, emptyTile);

              auto& newTile = m_NewTiles.ExpandAndGetRef();
              newTile.m_hComponent = m_pComponent->GetHandle();
              newTile.m_uiOutputIndex = m_uiOutputIndex;
              newTile.m_iPosX = iPosX + iX;
              newTile.m_iPosY = iPosY + iY;
              newTile.m_fMinZ = minZ;
              newTile.m_fMaxZ = maxZ;
              newTile.m_fTileSize = fTileSize;
              newTile.m_fDistanceToCamera = plMath::MaxValue<float>();
              newTile.m_GlobalToLocalBoxTransforms = globalToLocalBoxTransforms;
            }
          }
        }

        ++iX;
        fX += fTileSize;
      }

      ++iY;
      fY += fTileSize;
    }
  }

  m_CameraPositions.Clear();

  // Find old tiles
  plUInt32 uiMaxOldTiles = (plUInt32)iRadius * 2;
  uiMaxOldTiles *= uiMaxOldTiles;

  if (outputContext.m_TileIndices.GetCount() > uiMaxOldTiles)
  {
    m_TilesByAge.Clear();

    plUInt64 uiCurrentFrame = plRenderWorld::GetFrameCounter();
    for (auto it = outputContext.m_TileIndices.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_uiIndex == EmptyTileIndex)
        continue;

      if (it.Value().m_uiLastSeenFrame == uiCurrentFrame)
      {
        --uiMaxOldTiles;
        continue;
      }

      m_TilesByAge.PushBack({it.Key(), it.Value().m_uiLastSeenFrame});
    }

    if (m_TilesByAge.GetCount() > uiMaxOldTiles)
    {
      m_TilesByAge.Sort([](auto& tileA, auto& tileB) { return tileA.m_uiLastSeenFrame < tileB.m_uiLastSeenFrame; });

      plUInt32 uiOldTileCount = m_TilesByAge.GetCount() - uiMaxOldTiles;
      for (plUInt32 i = 0; i < uiOldTileCount; ++i)
      {
        m_OldTileKeys.PushBack(m_TilesByAge[i].m_uiTileKey);
      }
    }
  }
}
