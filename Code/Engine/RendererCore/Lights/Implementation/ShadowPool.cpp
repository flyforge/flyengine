#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ShadowPool)
BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  plShadowPool::OnEngineStartup();
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  plShadowPool::OnEngineShutdown();
}

PLASMA_END_SUBSYSTEM_DECLARATION;
  // clang-format on

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
plCVarBool cvar_RenderingShadowsShowPoolStats("Rendering.Shadows.ShowPoolStats", false, plCVarFlags::Default, "Display same stats of the shadow pool");
#endif

static plUInt32 s_uiShadowAtlasTextureWidth = 4096; ///\todo make this configurable
static plUInt32 s_uiShadowAtlasTextureHeight = 4096;
static plUInt32 s_uiShadowMapSize = 1024;
static plUInt32 s_uiMinShadowMapSize = 64;
static float s_fFadeOutScaleStart = (s_uiMinShadowMapSize + 1.0f) / s_uiShadowMapSize;
static float s_fFadeOutScaleEnd = s_fFadeOutScaleStart * 0.5f;

struct ShadowView
{
  plViewHandle m_hView;
  plCamera m_Camera;
};

struct ShadowData
{
  plHybridArray<plViewHandle, 6> m_Views;
  plUInt32 m_uiType;
  float m_fShadowMapScale;
  float m_fPenumbraSize;
  float m_fSlopeBias;
  float m_fConstantBias;
  float m_fFadeOutStart;
  float m_fMinRange;
  plUInt32 m_uiPackedDataOffset; // in 16 bytes steps
};

struct LightAndRefView
{
  PLASMA_DECLARE_POD_TYPE();

  const plLightComponent* m_pLight;
  const plView* m_pReferenceView;
};

struct SortedShadowData
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 m_uiIndex;
  float m_fShadowMapScale;

  PLASMA_ALWAYS_INLINE bool operator<(const SortedShadowData& other) const
  {
    if (m_fShadowMapScale > other.m_fShadowMapScale) // we want to sort descending (higher scale first)
      return true;

    return m_uiIndex < other.m_uiIndex;
  }
};

static plDynamicArray<SortedShadowData> s_SortedShadowData;

struct AtlasCell
{
  PLASMA_DECLARE_POD_TYPE();

  PLASMA_ALWAYS_INLINE AtlasCell()
    : m_Rect(0, 0, 0, 0)
  {
    m_uiChildIndices[0] = m_uiChildIndices[1] = m_uiChildIndices[2] = m_uiChildIndices[3] = 0xFFFF;
    m_uiDataIndex = plInvalidIndex;
  }

  PLASMA_ALWAYS_INLINE bool IsLeaf() const
  {
    return m_uiChildIndices[0] == 0xFFFF && m_uiChildIndices[1] == 0xFFFF && m_uiChildIndices[2] == 0xFFFF && m_uiChildIndices[3] == 0xFFFF;
  }

  plRectU32 m_Rect;
  plUInt16 m_uiChildIndices[4];
  plUInt32 m_uiDataIndex;
};

static plDeque<AtlasCell> s_AtlasCells;

static AtlasCell* Insert(AtlasCell* pCell, plUInt32 uiShadowMapSize, plUInt32 uiDataIndex)
{
  if (!pCell->IsLeaf())
  {
    for (plUInt32 i = 0; i < 4; ++i)
    {
      AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[i]];
      if (AtlasCell* pNewCell = Insert(pChildCell, uiShadowMapSize, uiDataIndex))
      {
        return pNewCell;
      }
    }

    return nullptr;
  }
  else
  {
    if (pCell->m_uiDataIndex != plInvalidIndex)
      return nullptr;

    if (pCell->m_Rect.width < uiShadowMapSize || pCell->m_Rect.height < uiShadowMapSize)
      return nullptr;

    if (pCell->m_Rect.width == uiShadowMapSize && pCell->m_Rect.height == uiShadowMapSize)
    {
      pCell->m_uiDataIndex = uiDataIndex;
      return pCell;
    }

    // Split
    plUInt32 x = pCell->m_Rect.x;
    plUInt32 y = pCell->m_Rect.y;
    plUInt32 w = pCell->m_Rect.width / 2;
    plUInt32 h = pCell->m_Rect.height / 2;

    plUInt32 uiCellIndex = s_AtlasCells.GetCount();
    s_AtlasCells.ExpandAndGetRef().m_Rect = plRectU32(x, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = plRectU32(x + w, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = plRectU32(x, y + h, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = plRectU32(x + w, y + h, w, h);

    for (plUInt32 i = 0; i < 4; ++i)
    {
      pCell->m_uiChildIndices[i] = static_cast<plUInt16>(uiCellIndex + i);
    }

    AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[0]];
    return Insert(pChildCell, uiShadowMapSize, uiDataIndex);
  }
}

static plRectU32 FindAtlasRect(plUInt32 uiShadowMapSize, plUInt32 uiDataIndex)
{
  PLASMA_ASSERT_DEBUG(plMath::IsPowerOf2(uiShadowMapSize), "Size must be power of 2");

  AtlasCell* pCell = Insert(&s_AtlasCells[0], uiShadowMapSize, uiDataIndex);
  if (pCell != nullptr)
  {
    PLASMA_ASSERT_DEBUG(pCell->IsLeaf() && pCell->m_uiDataIndex == uiDataIndex, "Implementation error");
    return pCell->m_Rect;
  }

  plLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
  return plRectU32(0, 0, 0, 0);
}

static float AddSafeBorder(plAngle fov, float fPenumbraSize)
{
  float fHalfHeight = plMath::Tan(fov * 0.5f);
  float fNewFov = plMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
  return fNewFov;
}

plTagSet s_ExcludeTagsWhiteList;

static void CopyExcludeTagsOnWhiteList(const plTagSet& referenceTags, plTagSet& out_targetTags)
{
  out_targetTags.Clear();
  out_targetTags.SetByName("EditorHidden");

  for (auto& tag : referenceTags)
  {
    if (s_ExcludeTagsWhiteList.IsSet(tag))
    {
      out_targetTags.Set(tag);
    }
  }
}

// must not be in anonymous namespace
template <>
struct plHashHelper<LightAndRefView>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(LightAndRefView value) { return plHashingUtils::xxHash32(&value.m_pLight, sizeof(LightAndRefView)); }

  PLASMA_ALWAYS_INLINE static bool Equal(const LightAndRefView& a, const LightAndRefView& b)
  {
    return a.m_pLight == b.m_pLight && a.m_pReferenceView == b.m_pReferenceView;
  }
};

//////////////////////////////////////////////////////////////////////////

struct plShadowPool::Data
{
  Data() { Clear(); }

  ~Data()
  {
    for (auto& shadowView : m_ShadowViews)
    {
      plRenderWorld::DeleteView(shadowView.m_hView);
    }

    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    if (!m_hShadowAtlasTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hShadowAtlasTexture);
      m_hShadowAtlasTexture.Invalidate();
    }

    if (!m_hShadowDataBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hShadowDataBuffer);
      m_hShadowDataBuffer.Invalidate();
    }
  }

  enum
  {
    MAX_SHADOW_DATA = 1024
  };

  void CreateShadowAtlasTexture()
  {
    if (m_hShadowAtlasTexture.IsInvalidated())
    {
      plGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight, plGALResourceFormat::D16);

      m_hShadowAtlasTexture = plGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(plVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferType = plGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hShadowDataBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  plViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();
    CreateShadowDataBuffer();

    plView* pView = nullptr;
    plViewHandle hView = plRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(plCameraUsageHint::Shadow);

    plGALRenderTargets renderTargets;
    renderTargets.m_hDSTarget = m_hShadowAtlasTexture;
    pView->SetRenderTargets(renderTargets);

    PLASMA_ASSERT_DEV(m_ShadowViewsMutex.IsLocked(), "m_ShadowViewsMutex must be locked at this point.");
    m_ShadowViewsMutex.Unlock(); // if the resource gets loaded in the call below, his could lead to a deadlock

    // ShadowMapRenderPipeline.plRenderPipelineAsset
    pView->SetRenderPipelineResource(plResourceManager::LoadResource<plRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

    m_ShadowViewsMutex.Lock();

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnEndExtraction before
    // rendering.
    pView->SetViewport(plRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    pView->m_IncludeTags.Set(tagCastShadows);

    pView->m_ExcludeTags.SetByName("EditorHidden");

    return hView;
  }

  ShadowView& GetShadowView(plView*& out_pView)
  {
    PLASMA_LOCK(m_ShadowViewsMutex);

    if (m_uiUsedViews == m_ShadowViews.GetCount())
    {
      m_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = m_ShadowViews[m_uiUsedViews];
    if (plRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
      out_pView->SetLodCamera(nullptr);
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(const plLightComponent* pLight, const plView* pReferenceView, float fShadowMapScale, plUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
  {
    PLASMA_LOCK(m_ShadowDataMutex);

    LightAndRefView key = {pLight, pReferenceView};

    plUInt32 uiDataIndex = plInvalidIndex;
    if (m_LightToShadowDataTable.TryGetValue(key, uiDataIndex))
    {
      out_pData = &m_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = plMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    m_ShadowData.EnsureCount(m_uiUsedShadowData + 1);

    out_pData = &m_ShadowData[m_uiUsedShadowData];
    out_pData->m_fShadowMapScale = fShadowMapScale;
    out_pData->m_fPenumbraSize = pLight->GetPenumbraSize();
    out_pData->m_fSlopeBias = pLight->GetSlopeBias() * 100.0f;       // map from user friendly range to real range
    out_pData->m_fConstantBias = pLight->GetConstantBias() / 100.0f; // map from user friendly range to real range
    out_pData->m_fFadeOutStart = 1.0f;
    out_pData->m_fMinRange = 1.0f;
    out_pData->m_uiPackedDataOffset = m_uiUsedPackedShadowData;

    m_LightToShadowDataTable.Insert(key, m_uiUsedShadowData);

    ++m_uiUsedShadowData;
    m_uiUsedPackedShadowData += uiPackedDataSizeInBytes / sizeof(plVec4);

    return false;
  }

  void Clear()
  {
    m_uiUsedViews = 0;
    m_uiUsedShadowData = 0;

    m_LightToShadowDataTable.Clear();

    m_uiUsedPackedShadowData = 0;
  }

  plMutex m_ShadowViewsMutex;
  plDeque<ShadowView> m_ShadowViews;
  plUInt32 m_uiUsedViews = 0;

  plMutex m_ShadowDataMutex;
  plDeque<ShadowData> m_ShadowData;
  plUInt32 m_uiUsedShadowData = 0;
  plHashTable<LightAndRefView, plUInt32> m_LightToShadowDataTable;

  plDynamicArray<plVec4, plAlignedAllocatorWrapper> m_PackedShadowData[2];
  plUInt32 m_uiUsedPackedShadowData = 0; // in 16 bytes steps (sizeof(plVec4))

  plGALTextureHandle m_hShadowAtlasTexture;
  plGALBufferHandle m_hShadowDataBuffer;
};

//////////////////////////////////////////////////////////////////////////

plShadowPool::Data* plShadowPool::s_pData = nullptr;

// static
plUInt32 plShadowPool::AddDirectionalLight(const plDirectionalLightComponent* pDirLight, const plView* pReferenceView)
{
  PLASMA_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  // No shadows in orthographic views
  if (pReferenceView->GetCullingCamera()->IsOrthographic())
  {
    return plInvalidIndex;
  }

  float fMaxReferenceSize = plMath::Max(pReferenceView->GetViewport().width, pReferenceView->GetViewport().height);
  float fShadowMapScale = fMaxReferenceSize / s_uiShadowMapSize;

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pDirLight, pReferenceView, fShadowMapScale, sizeof(plDirShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  plUInt32 uiNumCascades = plMath::Min(pDirLight->GetNumCascades(), 4u);
  const plCamera* pReferenceCamera = pReferenceView->GetCullingCamera();

  pData->m_uiType = LIGHT_TYPE_DIR;
  pData->m_fFadeOutStart = pDirLight->GetFadeOutStart();
  pData->m_fMinRange = pDirLight->GetMinShadowRange();
  pData->m_Views.SetCount(uiNumCascades);

  // determine cascade ranges
  float fNearPlane = pReferenceCamera->GetNearPlane();
  float fShadowRange = pDirLight->GetMinShadowRange();
  float fSplitModeWeight = pDirLight->GetSplitModeWeight();

  float fCascadeRanges[4];
  for (plUInt32 i = 0; i < uiNumCascades; ++i)
  {
    float f = float(i + 1) / uiNumCascades;
    float logDistance = fNearPlane * plMath::Pow(fShadowRange / fNearPlane, f);
    float linearDistance = fNearPlane + (fShadowRange - fNearPlane) * f;
    fCascadeRanges[i] = plMath::Lerp(linearDistance, logDistance, fSplitModeWeight);
  }

  const char* viewNames[4] = {"DirLightViewC0", "DirLightViewC1", "DirLightViewC2", "DirLightViewC3"};

  const plGameObject* pOwner = pDirLight->GetOwner();
  plVec3 vForward = pOwner->GetGlobalDirForwards();
  plVec3 vUp = pOwner->GetGlobalDirUp();

  float fAspectRatio = pReferenceView->GetViewport().width / pReferenceView->GetViewport().height;

  float fCascadeStart = 0.0f;
  float fCascadeEnd = 0.0f;
  float fTanFovX = plMath::Tan(pReferenceCamera->GetFovX(fAspectRatio) * 0.5f);
  float fTanFovY = plMath::Tan(pReferenceCamera->GetFovY(fAspectRatio) * 0.5f);
  plVec3 corner = plVec3(fTanFovX, fTanFovY, 1.0f);

  float fNearPlaneOffset = pDirLight->GetNearPlaneOffset();

  for (plUInt32 i = 0; i < uiNumCascades; ++i)
  {
    plView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<plWorld*>(pDirLight->GetWorld()));
      pView->SetLodCamera(pReferenceCamera);
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;
      fCascadeEnd = fCascadeRanges[i];

      plVec3 startCorner = corner * fCascadeStart;
      plVec3 endCorner = corner * fCascadeEnd;

      // Find the enclosing sphere for the frustum:
      // The sphere center must be on the view's center ray and should be equally far away from the corner points.
      // x = distance from camera origin to sphere center
      // d1^2 = sc.x^2 + sc.y^2 + (x - sc.z)^2
      // d2^2 = ec.x^2 + ec.y^2 + (x - ec.z)^2
      // d1 == d2 and solve for x:
      float x = (endCorner.Dot(endCorner) - startCorner.Dot(startCorner)) / (2.0f * (endCorner.z - startCorner.z));
      x = plMath::Min(x, fCascadeEnd);

      plVec3 center = pReferenceCamera->GetPosition() + pReferenceCamera->GetDirForwards() * x;

      // prevent too large values
      // sometimes this can happen when imported data is badly scaled and thus way too large
      // then adding dirForwards result in no change and we run into other asserts later
      center.x = plMath::Clamp(center.x, -1000000.0f, +1000000.0f);
      center.y = plMath::Clamp(center.y, -1000000.0f, +1000000.0f);
      center.z = plMath::Clamp(center.z, -1000000.0f, +1000000.0f);

      endCorner.z -= x;
      float radius = endCorner.GetLength();

      if (false)
      {
        plDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), plBoundingSphere(center, radius), plColor::OrangeRed);
      }

      float fCameraToCenterDistance = radius + fNearPlaneOffset;
      plVec3 shadowCameraPos = center - vForward * fCameraToCenterDistance;
      float fFarPlane = radius + fCameraToCenterDistance;

      plCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vUp);
      camera.SetCameraMode(plCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      plMat4 worldToLightMatrix = pView->GetViewMatrix(plCameraEye::Left);
      plVec3 offset = worldToLightMatrix.TransformPosition(plVec3::ZeroVector());
      float texelInWorld = (2.0f * radius) / s_uiShadowMapSize;
      offset.x -= plMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= plMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);
    }

    plRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
plUInt32 plShadowPool::AddPointLight(const plPointLightComponent* pPointLight, float fScreenSpaceSize, const plView* pReferenceView)
{
  PLASMA_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd * 2.0f)
  {
    return plInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fScreenSpaceSize, sizeof(plPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  plVec3 faceDirs[6] = {
    plVec3(1.0f, 0.0f, 0.0f),
    plVec3(-1.0f, 0.0f, 0.0f),
    plVec3(0.0f, 1.0f, 0.0f),
    plVec3(0.0f, -1.0f, 0.0f),
    plVec3(0.0f, 0.0f, 1.0f),
    plVec3(0.0f, 0.0f, -1.0f),
  };

  const char* viewNames[6] = {
    "PointLightView+X",
    "PointLightView-X",
    "PointLightView+Y",
    "PointLightView-Y",
    "PointLightView+Z",
    "PointLightView-Z",
  };

  const plGameObject* pOwner = pPointLight->GetOwner();
  plVec3 vPosition = pOwner->GetGlobalPosition();
  plVec3 vUp = plVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = plMath::Max(pPointLight->GetPenumbraSize(), (0.5f / s_uiMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(plAngle::Degree(90.0f), fPenumbraSize);

  float fNearPlane = 0.1f; ///\todo expose somewhere
  float fFarPlane = pPointLight->GetRange();

  for (plUInt32 i = 0; i < 6; ++i)
  {
    plView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<plWorld*>(pPointLight->GetWorld()));
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      plVec3 vForward = faceDirs[i];

      plCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    plRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
plUInt32 plShadowPool::AddSpotLight(const plSpotLightComponent* pSpotLight, float fScreenSpaceSize, const plView* pReferenceView)
{
  PLASMA_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd)
  {
    return plInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fScreenSpaceSize, sizeof(plSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  plView* pView = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0] = shadowView.m_hView;

  // Setup view
  {
    pView->SetName("SpotLightView");
    pView->SetWorld(const_cast<plWorld*>(pSpotLight->GetWorld()));
    CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
  }

  // Setup camera
  {
    const plGameObject* pOwner = pSpotLight->GetOwner();
    plVec3 vPosition = pOwner->GetGlobalPosition();
    plVec3 vForward = pOwner->GetGlobalDirForwards();
    plVec3 vUp = pOwner->GetGlobalDirUp();

    float fFov = AddSafeBorder(pSpotLight->GetOuterSpotAngle(), pSpotLight->GetPenumbraSize());
    float fNearPlane = 0.1f; ///\todo expose somewhere
    float fFarPlane = pSpotLight->GetRange();

    plCamera& camera = shadowView.m_Camera;
    camera.LookAt(vPosition, vPosition + vForward, vUp);
    camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
  }

  plRenderWorld::AddViewToRender(shadowView.m_hView);

  return pData->m_uiPackedDataOffset;
}

// static
plGALTextureHandle plShadowPool::GetShadowAtlasTexture()
{
  return s_pData->m_hShadowAtlasTexture;
}

// static
plGALBufferHandle plShadowPool::GetShadowDataBuffer()
{
  return s_pData->m_hShadowDataBuffer;
}

// static
void plShadowPool::AddExcludeTagToWhiteList(const plTag& tag)
{
  s_ExcludeTagsWhiteList.Set(tag);
}

// static
void plShadowPool::OnEngineStartup()
{
  s_pData = PLASMA_DEFAULT_NEW(plShadowPool::Data);

  plRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  plRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void plShadowPool::OnEngineShutdown()
{
  plRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  plRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  PLASMA_DEFAULT_DELETE(s_pData);
}

// static
void plShadowPool::OnExtractionEvent(const plRenderWorldExtractionEvent& e)
{
  if (e.m_Type != plRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  PLASMA_PROFILE_SCOPE("Shadow Pool Update");

  plUInt32 uiDataIndex = plRenderWorld::GetDataIndexForExtraction();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  packedShadowData.SetCountUninitialized(s_pData->m_uiUsedPackedShadowData);

  if (s_pData->m_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (plUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_pData->m_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    auto& sorted = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_uiType == LIGHT_TYPE_DIR ? 100.0f : plMath::Min(shadowData.m_fShadowMapScale, 10.0f);
  }

  s_SortedShadowData.Sort();

  // Prepare atlas
  s_AtlasCells.Clear();
  s_AtlasCells.ExpandAndGetRef().m_Rect = plRectU32(0, 0, s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight);

  float fAtlasInvWidth = 1.0f / s_uiShadowAtlasTextureWidth;
  float fAtlasInvHeight = 1.0f / s_uiShadowAtlasTextureWidth;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  plUInt32 uiTotalAtlasSize = s_uiShadowAtlasTextureWidth * s_uiShadowAtlasTextureHeight;
  plUInt32 uiUsedAtlasSize = 0;

  plDebugRendererContext debugContext(plWorld::GetWorld(0));
  if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
  {
    debugContext = plDebugRendererContext(pView->GetHandle());
  }

  if (cvar_RenderingShadowsShowPoolStats)
  {
    plDebugRenderer::DrawInfoText(debugContext, plDebugTextPlacement::TopLeft, "ShadowPoolStats", "Shadow Pool Stats:", plColor::LightSteelBlue);
    plDebugRenderer::DrawInfoText(debugContext, plDebugTextPlacement::TopLeft, "ShadowPoolStats", "Details (Name: Size - Atlas Offset)", plColor::LightSteelBlue);
  }

#endif

  for (auto& sorted : s_SortedShadowData)
  {
    plUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    plUInt32 uiShadowMapSize = s_uiShadowMapSize;
    float fadeOutStart = s_fFadeOutScaleStart;
    float fadeOutEnd = s_fFadeOutScaleEnd;

    // point lights use a lot of atlas space thus we cut the shadow map size in half
    if (shadowData.m_uiType == LIGHT_TYPE_POINT)
    {
      uiShadowMapSize /= 2;
      fadeOutStart *= 2.0f;
      fadeOutEnd *= 2.0f;
    }

    uiShadowMapSize = plMath::PowerOfTwo_Ceil((plUInt32)(uiShadowMapSize * plMath::Clamp(shadowData.m_fShadowMapScale, fadeOutStart, 1.0f)));

    plHybridArray<plView*, 8> shadowViews;
    plHybridArray<plRectU32, 8> atlasRects;

    // Fill atlas
    for (plUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      plView* pShadowView = nullptr;
      plRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView);
      shadowViews.PushBack(pShadowView);

      PLASMA_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

      plRectU32 atlasRect = FindAtlasRect(uiShadowMapSize, uiShadowDataIndex);
      atlasRects.PushBack(atlasRect);

      pShadowView->SetViewport(plRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsShowPoolStats)
      {
        plDebugRenderer::DrawInfoText(debugContext, plDebugTextPlacement::TopLeft, "ShadowPoolStats", plFmt("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y), plColor::LightSteelBlue);

        uiUsedAtlasSize += atlasRect.width * atlasRect.height;
      }
#endif
    }

    // Fill shadow data
    if (shadowData.m_uiType == LIGHT_TYPE_DIR)
    {
      plUInt32 uiNumCascades = shadowData.m_Views.GetCount();

      plUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, 0);
      plMat4& worldToLightMatrix = *reinterpret_cast<plMat4*>(&packedShadowData[uiMatrixIndex]);

      worldToLightMatrix = shadowViews[0]->GetViewProjectionMatrix(plCameraEye::Left);

      for (plUInt32 uiViewIndex = 0; uiViewIndex < uiNumCascades; ++uiViewIndex)
      {
        if (uiViewIndex >= 1)
        {
          plMat4 cascadeToWorldMatrix = shadowViews[uiViewIndex]->GetInverseViewProjectionMatrix(plCameraEye::Left);
          plVec3 cascadeCorner = cascadeToWorldMatrix.TransformPosition(plVec3(0.0f));
          cascadeCorner = worldToLightMatrix.TransformPosition(cascadeCorner);

          plVec3 otherCorner = cascadeToWorldMatrix.TransformPosition(plVec3(1.0f));
          otherCorner = worldToLightMatrix.TransformPosition(otherCorner);

          plUInt32 uiCascadeScaleIndex = GET_CASCADE_SCALE_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);
          plUInt32 uiCascadeOffsetIndex = GET_CASCADE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);

          plVec4& cascadeScale = packedShadowData[uiCascadeScaleIndex];
          plVec4& cascadeOffset = packedShadowData[uiCascadeOffsetIndex];

          cascadeScale = plVec3(1.0f).CompDiv(otherCorner - cascadeCorner).GetAsVec4(1.0f);
          cascadeOffset = cascadeCorner.GetAsVec4(0.0f).CompMul(-cascadeScale);
        }

        plUInt32 uiAtlasScaleOffsetIndex = GET_ATLAS_SCALE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        plVec4& atlasScaleOffset = packedShadowData[uiAtlasScaleOffsetIndex];

        plRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          plVec2 scale = plVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          plVec2 offset = plVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          // combine with tex scale offset
          atlasScaleOffset.x = scale.x * 0.5f;
          atlasScaleOffset.y = scale.y * -0.5f;
          atlasScaleOffset.z = offset.x + scale.x * 0.5f;
          atlasScaleOffset.w = offset.y + scale.y * 0.5f;
        }
        else
        {
          atlasScaleOffset.Set(1.0f, 1.0f, 0.0f, 0.0f);
        }
      }

      const plCamera* pFirstCascadeCamera = shadowViews[0]->GetCamera();
      const plCamera* pLastCascadeCamera = shadowViews[uiNumCascades - 1]->GetCamera();

      float cascadeSize = pFirstCascadeCamera->GetFovOrDim();
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = plMath::Max(shadowData.m_fPenumbraSize / cascadeSize, texelSize);
      float goodPenumbraSize = 8.0f / uiShadowMapSize;
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // params
      {
        // tweak values to keep the default values consistent with spot and point lights
        float slopeBias = shadowData.m_fSlopeBias * plMath::Max(penumbraSize, goodPenumbraSize);
        float constantBias = shadowData.m_fConstantBias * 0.2f;
        plUInt32 uilastCascadeIndex = uiNumCascades - 1;

        plUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        plVec4& shadowParams = packedShadowData[uiParamsIndex];
        shadowParams.x = slopeBias;
        shadowParams.y = constantBias;
        shadowParams.z = penumbraSize * relativeShadowSize;
        shadowParams.w = *reinterpret_cast<float*>(&uilastCascadeIndex);
      }

      // params2
      {
        float ditherMultiplier = 0.2f / cascadeSize;
        float zRange = cascadeSize / pFirstCascadeCamera->GetFarPlane();

        float actualPenumbraSize = shadowData.m_fPenumbraSize / pLastCascadeCamera->GetFovOrDim();
        float penumbraSizeIncrement = plMath::Max(goodPenumbraSize - actualPenumbraSize, 0.0f) / shadowData.m_fMinRange;

        plUInt32 uiParams2Index = GET_SHADOW_PARAMS2_INDEX(shadowData.m_uiPackedDataOffset);
        plVec4& shadowParams2 = packedShadowData[uiParams2Index];
        shadowParams2.x = 1.0f - (plMath::Max(penumbraSize, goodPenumbraSize) + texelSize) * 2.0f;
        shadowParams2.y = ditherMultiplier;
        shadowParams2.z = ditherMultiplier * zRange;
        shadowParams2.w = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale = -1.0f / fadeOutRange;
        float xyOffset = -xyScale;

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale = -1.0f / zFadeOutRange;
        float zOffset = -zScale;

        plUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        plVec4& fadeOutParams = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x = xyScale;
        fadeOutParams.y = xyOffset;
        fadeOutParams.z = zScale;
        fadeOutParams.w = zOffset;
      }
    }
    else // spot or point light
    {
      plMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(plVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(plVec3(0.5f, 0.5f, 0.0f));

      plAngle fov;

      for (plUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
      {
        plView* pShadowView = shadowViews[uiViewIndex];
        PLASMA_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

        plUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        plMat4& worldToLightMatrix = *reinterpret_cast<plMat4*>(&packedShadowData[uiMatrixIndex]);

        plRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          plVec2 scale = plVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          plVec2 offset = plVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          plMat4 atlasMatrix;
          atlasMatrix.SetIdentity();
          atlasMatrix.SetDiagonal(plVec4(scale.x, scale.y, 1.0f, 1.0f));
          atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

          fov = pShadowView->GetCamera()->GetFovY(1.0f);
          const plMat4& viewProjection = pShadowView->GetViewProjectionMatrix(plCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      float screenHeight = plMath::Tan(fov * 0.5f) * 20.0f; // screen height in worldspace at 10m distance
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = plMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      float slopeBias = shadowData.m_fSlopeBias * penumbraSize * plMath::Tan(fov * 0.5f);
      float constantBias = shadowData.m_fConstantBias * s_uiShadowMapSize / uiShadowMapSize;
      float fadeOut = plMath::Clamp((shadowData.m_fShadowMapScale - fadeOutEnd) / (fadeOutStart - fadeOutEnd), 0.0f, 1.0f);

      plUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      plVec4& shadowParams = packedShadowData[uiParamsIndex];
      shadowParams.x = slopeBias;
      shadowParams.y = constantBias;
      shadowParams.z = penumbraSize * relativeShadowSize;
      shadowParams.w = plMath::Sqrt(fadeOut);
    }
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingShadowsShowPoolStats)
  {
    plDebugRenderer::DrawInfoText(debugContext, plDebugTextPlacement::TopLeft, "ShadowPoolStats", plFmt("Atlas Utilization: {0}%%", plArgF(100.0 * (double)uiUsedAtlasSize / uiTotalAtlasSize, 2)), plColor::LightSteelBlue);
  }
#endif

  s_pData->Clear();
}

// static
void plShadowPool::OnRenderEvent(const plRenderWorldRenderEvent& e)
{
  if (e.m_Type != plRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hShadowAtlasTexture.IsInvalidated() || s_pData->m_hShadowDataBuffer.IsInvalidated())
    return;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALPass* pGALPass = pDevice->BeginPass("Shadow Atlas");

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(s_pData->m_hShadowAtlasTexture));
  renderingSetup.m_bClearDepth = true;

  auto pCommandEncoder = pGALPass->BeginRendering(renderingSetup);

  plUInt32 uiDataIndex = plRenderWorld::GetDataIndexForRendering();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  if (!packedShadowData.IsEmpty())
  {
    PLASMA_PROFILE_SCOPE("Shadow Data Buffer Update");

    pCommandEncoder->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr());
  }

  pGALPass->EndRendering(pCommandEncoder);
  pDevice->EndPass(pGALPass);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ShadowPool);
