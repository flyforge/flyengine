#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plSkeletonViewContext::plSkeletonViewContext(plSkeletonContext* pContext)
  : PlasmaEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::ZeroVector(), plVec3(0.0f, 0.0f, 1.0f));
}

plSkeletonViewContext::~plSkeletonViewContext() {}

bool plSkeletonViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}


void plSkeletonViewContext::Redraw(bool bRenderEditorGizmos)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    const plTag& tagNoOrtho = plTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    if (pView->GetCamera()->IsOrthographic())
    {
      pView->m_ExcludeTags.Set(tagNoOrtho);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagNoOrtho);
    }

    PLASMA_LOCK(pView->GetWorld()->GetWriteMarker());
    if (auto pGizmoManager = pView->GetWorld()->GetComponentManager<plGizmoComponentManager>())
    {
      pGizmoManager->m_uiHighlightID = GetDocumentContext()->m_Context.m_uiHighlightID;
    }
  }

  PlasmaEngineProcessViewContext::Redraw(bRenderEditorGizmos);
}

plViewHandle plSkeletonViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Skeleton Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plSkeletonViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    PlasmaEngineProcessViewContext::DrawSimpleGrid();
  }

  PlasmaEngineProcessViewContext::SetCamera(pMsg);

  const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    plResourceLock<plSkeletonResource> pSkeleton(hSkeleton, plResourceAcquireMode::AllowLoadingFallback);

    plUInt32 uiNumJoints = pSkeleton->GetDescriptor().m_Skeleton.GetJointCount();

    plStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    plDebugRenderer::Draw2DText(m_hView, sText, plVec2I32(10, viewHeight - 10), plColor::White, 16, plDebugTextHAlign::Left,
      plDebugTextVAlign::Bottom);
  }
}

void plSkeletonViewContext::HandleViewMessage(const PlasmaEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewPickingMsgToEngine>())
  {
    const plViewPickingMsgToEngine* pMsg2 = static_cast<const plViewPickingMsgToEngine*>(pMsg);

    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", true);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", true);
    }

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
  else
  {
    PlasmaEngineProcessViewContext::HandleViewMessage(pMsg);
  }
}

void plSkeletonViewContext::PickObjectAt(plUInt16 x, plUInt16 y)
{
  // remote processes do not support picking, just ignore this
  if (PlasmaEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  plViewPickingResultMsgToEditor res;

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView) && pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickingMatrix"))
  {
    pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", plVec2(x, y));
    plVariant varMat = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingMatrix");

    if (varMat.IsA<plMat4>())
    {
      const plUInt32 uiPickingID = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingID").ConvertTo<plUInt32>();
      // const float fPickingDepth = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingDepth").ConvertTo<float>();
      res.m_vPickedNormal = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingNormal").ConvertTo<plVec3>();
      res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<plVec3>();
      res.m_vPickedPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingPosition").ConvertTo<plVec3>();

      PLASMA_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "");

      const plUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
      const plUInt32 uiPartIndex = (uiPickingID >> 24) & 0x7F; // highest bit indicates whether the object is dynamic, ignore this

      res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
      res.m_OtherGuid = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

      if (res.m_ComponentGuid.IsValid())
      {
        plComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

        PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

        // check whether the component is still valid
        plComponent* pComponent = nullptr;
        if (pDocumentContext->GetWorld()->TryGetComponent<plComponent>(hComponent, pComponent))
        {
          // if yes, fill out the parent game object guid
          res.m_ObjectGuid = GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
          res.m_uiPartIndex = uiPartIndex;
        }
        else
        {
          res.m_ComponentGuid = plUuid();
        }
      }
    }
  }

  SendViewMessage(&res);
}
