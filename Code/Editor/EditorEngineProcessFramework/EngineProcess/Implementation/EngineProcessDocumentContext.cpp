#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/ImageUtils.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEngineProcessDocumentContext, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plHashTable<plUuid, plEngineProcessDocumentContext*> plEngineProcessDocumentContext::s_DocumentContexts;

plEngineProcessDocumentContext* plEngineProcessDocumentContext::GetDocumentContext(plUuid guid)
{
  plEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void plEngineProcessDocumentContext::AddDocumentContext(plUuid guid, const plVariant& metaData, plEngineProcessDocumentContext* pContext, plEngineProcessCommunicationChannel* pIPC, plStringView sDocumentType)
{
  PL_ASSERT_DEV(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
  s_DocumentContexts[guid] = pContext;

  pContext->Initialize(guid, metaData, pIPC, sDocumentType);
}

bool plEngineProcessDocumentContext::PendingOperationsInProgress()
{
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->PendingOperationInProgress())
      return true;
  }
  return false;
}

void plEngineProcessDocumentContext::UpdateDocumentContexts()
{
  PL_PROFILE_SCOPE("UpdateDocumentContexts");
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->UpdateDocumentContext();
  }
}

void plEngineProcessDocumentContext::DestroyDocumentContext(plUuid guid)
{
  plEngineProcessDocumentContext* pContext = nullptr;
  if (s_DocumentContexts.Remove(guid, &pContext))
  {
    pContext->Deinitialize();
    pContext->GetDynamicRTTI()->GetAllocator()->Deallocate(pContext);
  }
}

plBoundingBoxSphere plEngineProcessDocumentContext::GetWorldBounds(plWorld* pWorld)
{
  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PL_LOCK(pWorld->GetReadMarker());

    PL_ASSERT_DEV(!pWorld->GetWorldSimulationEnabled(), "World simulation must be disabled to get bounds!");

    const plWorld* pConstWorld = pWorld;
    for (auto it = pConstWorld->GetObjects(); it.IsValid(); ++it)
    {
      const plGameObject* pObj = it;

      const auto& b = pObj->GetGlobalBounds();

      if (b.IsValid())
        bounds.ExpandToInclude(b);
    }
  }

  if (!bounds.IsValid())
    bounds = plBoundingBoxSphere::MakeFromCenterExtents(plVec3::MakeZero(), plVec3(1, 1, 1), 2);

  return bounds;
}

plEngineProcessDocumentContext::plEngineProcessDocumentContext(plBitflags<plEngineProcessDocumentContextFlags> flags)
  : m_Flags(flags)
{
  GetContext().m_Events.AddEventHandler(plMakeDelegate(&plEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

plEngineProcessDocumentContext::~plEngineProcessDocumentContext()
{
  PL_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'plEngineProcessDocumentContext::DestroyDocumentContext'");

  GetContext().m_Events.RemoveEventHandler(plMakeDelegate(&plEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

void plEngineProcessDocumentContext::Initialize(const plUuid& documentGuid, const plVariant& metaData, plEngineProcessCommunicationChannel* pIPC, plStringView sDocumentType)
{
  m_DocumentGuid = documentGuid;
  m_MetaData = metaData;
  m_pIPC = pIPC;

  if (m_sDocumentType != sDocumentType)
  {
    m_sDocumentType = sDocumentType;
  }

  if (m_Flags.IsSet(plEngineProcessDocumentContextFlags::CreateWorld))
  {
    plStringBuilder tmp;
    plWorldDesc desc(plConversionUtils::ToString(m_DocumentGuid, tmp));
    desc.m_bReportErrorWhenStaticObjectMoves = false;

    m_pWorld = PL_DEFAULT_NEW(plWorld, desc);
    m_pWorld->SetGameObjectReferenceResolver(plMakeDelegate(&plEngineProcessDocumentContext::ResolveStringToGameObjectHandle, this));

    GetContext().m_pWorld = m_pWorld;
    m_Mirror.InitReceiver(&GetContext());
  }
  OnInitialize();
}

void plEngineProcessDocumentContext::Deinitialize()
{
  OnDeinitialize();

  ClearViewContexts();
  m_Mirror.Clear();
  m_Mirror.DeInit();
  GetContext().Clear();

  CleanUpContextSyncObjects();
  if (m_Flags.IsSet(plEngineProcessDocumentContextFlags::CreateWorld))
  {
    PL_DEFAULT_DELETE(m_pWorld);
  }
  m_pWorld = nullptr;
}

void plEngineProcessDocumentContext::SendProcessMessage(plProcessMessage* pMsg)
{
  m_pIPC->SendMessage(pMsg);
}

void plEngineProcessDocumentContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  PL_LOCK(m_pWorld->GetWriteMarker());

  const bool bIsRemoteProcess = plEditorEngineProcessApp::GetSingleton()->IsRemoteMode();

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plEntityMsgToEngine>())
  {
    const plEntityMsgToEngine* pMsg2 = static_cast<const plEntityMsgToEngine*>(pMsg);
    m_Mirror.ApplyOp(const_cast<plObjectChange&>(pMsg2->m_change));

    plRttiConverterObject target = GetContext().GetObjectByGUID(pMsg2->m_change.m_Root);

    if (target.m_pType == nullptr || target.m_pObject == nullptr)
      return;

    if (target.m_pType == plGetStaticRTTI<plGameObject>())
    {
      plGameObject* pObject = static_cast<plGameObject*>(target.m_pObject);
      if (pObject != nullptr && pObject->IsStatic())
      {
        plRenderWorld::DeleteCachedRenderDataForObjectRecursive(pObject);
      }
    }
    else if (target.m_pType->IsDerivedFrom<plComponent>())
    {
      plComponent* pComponent = static_cast<plComponent*>(target.m_pObject);
      if (pComponent != nullptr && pComponent->GetOwner()->IsStatic())
      {
        plRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineSyncObjectMsg>())
  {
    const plEditorEngineSyncObjectMsg* pMsg2 = static_cast<const plEditorEngineSyncObjectMsg*>(pMsg);

    ProcessEditorEngineSyncObjectMsg(*pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plObjectTagMsgToEngine>())
  {
    const plObjectTagMsgToEngine* pMsg2 = static_cast<const plObjectTagMsgToEngine*>(pMsg);

    SetTagOnObject(pMsg2->m_ObjectGuid, pMsg2->m_sTag, pMsg2->m_bSetTag, pMsg2->m_bApplyOnAllChildren);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plExportDocumentMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const plExportDocumentMsgToEngine* pMsg2 = static_cast<const plExportDocumentMsgToEngine*>(pMsg);
    plExportDocumentMsgToEditor ret;
    ret.m_DocumentGuid = pMsg->m_DocumentGuid;

    plStatus res = ExportDocument(pMsg2);
    ret.m_bOutputSuccess = res.Succeeded();
    ret.m_sFailureMsg = res.m_sMessage;

    if (!ret.m_bOutputSuccess)
    {
      plLog::Error("Could not export to file '{0}'.", pMsg2->m_sOutputFile);
    }

    SendProcessMessage(&ret);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plCreateThumbnailMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    plFileSystem::ReloadAllExternalDataDirectoryConfigs();
    plResourceManager::ReloadAllResources(false);
    UpdateSyncObjects();
    const plCreateThumbnailMsgToEngine* pMsg2 = static_cast<const plCreateThumbnailMsgToEngine*>(pMsg);
    // As long as the thumbnail context is alive, we will trigger UpdateThumbnailViewContext
    // inside the UpdateDocumentContext function until the thumbnail rendering has converged and
    // the data is send back as a response.
    CreateThumbnailViewContext(pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineViewMsg>())
  {
    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewRedrawMsgToEngine>())
    {
      UpdateSyncObjects();
    }

    const plEditorEngineViewMsg* pViewMsg = static_cast<const plEditorEngineViewMsg*>(pMsg);
    PL_ASSERT_DEV(pViewMsg->m_uiViewID < 0xFFFFFFFF, "Invalid view ID in '{0}'", pMsg->GetDynamicRTTI()->GetTypeName());

    m_ViewContexts.EnsureCount(pViewMsg->m_uiViewID + 1);

    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewDestroyedMsgToEngine>())
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] != nullptr)
      {
        DestroyViewContext(m_ViewContexts[pViewMsg->m_uiViewID]);
        m_ViewContexts[pViewMsg->m_uiViewID] = nullptr;

        plLog::Debug("Destroyed View {0}", pViewMsg->m_uiViewID);
      }
      plViewDestroyedResponseMsgToEditor response;
      response.m_DocumentGuid = pViewMsg->m_DocumentGuid;
      response.m_uiViewID = pViewMsg->m_uiViewID;
      m_pIPC->SendMessage(&response);
    }
    else
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] == nullptr)
      {
        if (!plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = CreateViewContext();
        }
        else
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = PL_DEFAULT_NEW(plRemoteEngineProcessViewContext, this);
        }

        m_ViewContexts[pViewMsg->m_uiViewID]->SetViewID(pViewMsg->m_uiViewID);
      }

      m_ViewContexts[pViewMsg->m_uiViewID]->HandleViewMessage(pViewMsg);
    }

    return;
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewHighlightMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const plViewHighlightMsgToEngine* pMsg2 = static_cast<const plViewHighlightMsgToEngine*>(pMsg);

    GetContext().m_uiHighlightID = GetContext().m_ComponentPickingMap.GetHandle(pMsg2->m_HighlightObject);

    if (GetContext().m_uiHighlightID == 0)
      GetContext().m_uiHighlightID = GetContext().m_OtherPickingMap.GetHandle(pMsg2->m_HighlightObject);
  }
}

void plEngineProcessDocumentContext::AddSyncObject(plEditorEngineSyncObject* pSync)
{
  pSync->Configure(m_DocumentGuid, [this](plEditorEngineSyncObject* pSync) { RemoveSyncObject(pSync); });

  m_SyncObjects[pSync->GetGuid()] = pSync;
}

void plEngineProcessDocumentContext::RemoveSyncObject(plEditorEngineSyncObject* pSync)
{
  m_SyncObjects.Remove(pSync->GetGuid());
}

plEditorEngineSyncObject* plEngineProcessDocumentContext::FindSyncObject(const plUuid& guid)
{
  return m_SyncObjects.GetValueOrDefault(guid, nullptr);
}

void plEngineProcessDocumentContext::ClearViewContexts()
{
  for (auto* pContext : m_ViewContexts)
  {
    DestroyViewContext(pContext);
  }

  m_ViewContexts.Clear();
}


void plEngineProcessDocumentContext::CleanUpContextSyncObjects()
{
  while (!m_SyncObjects.IsEmpty())
  {
    auto it = m_SyncObjects.GetIterator();
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }
}

void plEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const plEditorEngineSyncObjectMsg& msg)
{
  auto it = m_SyncObjects.Find(msg.m_ObjectGuid);

  if (msg.m_sObjectType.IsEmpty())
  {
    // object has been deleted!
    if (it.IsValid())
    {
      it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
    }

    return;
  }

  const plRTTI* pRtti = plRTTI::FindTypeByName(msg.m_sObjectType);
  plEditorEngineSyncObject* pSyncObject = nullptr;
  bool bSetOwner = false;

  if (pRtti == nullptr)
  {
    plLog::Error("Cannot sync object of type unknown '{0}' to engine process", msg.m_sObjectType);
    return;
  }

  if (!it.IsValid())
  {
    // object does not yet exist
    PL_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '{0}' does not have a default allocator", msg.m_sObjectType);
    void* pObject = pRtti->GetAllocator()->Allocate<void>();

    pSyncObject = static_cast<plEditorEngineSyncObject*>(pObject);
    bSetOwner = true;
  }
  else
  {
    pSyncObject = it.Value();
  }

  plRawMemoryStreamReader reader(msg.m_ObjectData);

  plReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, pSyncObject);

  if (bSetOwner)
  {
    AddSyncObject(pSyncObject);
  }

  pSyncObject->SetModified(true);
}

void plEngineProcessDocumentContext::Reset()
{
  plUuid guid = m_DocumentGuid;
  auto ipc = m_pIPC;

  Deinitialize();

  Initialize(guid, m_MetaData, ipc, m_sDocumentType);
}

void plEngineProcessDocumentContext::ClearExistingObjects()
{
  GetContext().DeleteExistingObjects();
}

void plEngineProcessDocumentContext::OnInitialize() {}
void plEngineProcessDocumentContext::OnDeinitialize() {}

bool plEngineProcessDocumentContext::PendingOperationInProgress() const
{
  auto pState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetWorld());
  return m_pThumbnailViewContext != nullptr || pState != nullptr;
}

void plEngineProcessDocumentContext::UpdateDocumentContext()
{
  if (plEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    // in remote mode simply redraw all all views every time a context is updated
    for (auto pView : m_ViewContexts)
    {
      if (pView)
        pView->Redraw(false);
    }
  }

  {
    // If we have a running game state we always want to render it (e.g. play the game).
    auto pState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetWorld());
    if (pState != nullptr)
    {
      pState->ScheduleRendering();
    }
  }

  if (m_pThumbnailViewContext)
  {
    plResourceManager::ForceNoFallbackAcquisition(3);
    m_uiThumbnailConvergenceFrames++;

    if (!UpdateThumbnailViewContext(m_pThumbnailViewContext))
    {
      m_uiThumbnailConvergenceFrames = 0;
    }

    if (m_uiThumbnailConvergenceFrames > ThumbnailConvergenceFramesTarget)
    {
      plCreateThumbnailMsgToEditor ret;
      ret.m_DocumentGuid = GetDocumentGuid();

      // Download image
      {
        auto pGALPass = plGALDevice::GetDefaultDevice()->BeginPass("Thumbnail Readback");
        PL_SCOPE_EXIT(plGALDevice::GetDefaultDevice()->EndPass(pGALPass));

        auto pGALCommandEncoder = pGALPass->BeginRendering(plGALRenderingSetup());
        PL_SCOPE_EXIT(pGALPass->EndRendering(pGALCommandEncoder));

        pGALCommandEncoder->ReadbackTexture(m_hThumbnailColorRT);
        const plGALTexture* pThumbnailColor = plGALDevice::GetDefaultDevice()->GetTexture(m_hThumbnailColorRT);
        const plEnum<plGALResourceFormat> format = pThumbnailColor->GetDescription().m_Format;

        plGALSystemMemoryDescription MemDesc;
        {
          MemDesc.m_uiRowPitch = 4 * m_uiThumbnailWidth;
          MemDesc.m_uiSlicePitch = 4 * m_uiThumbnailWidth * m_uiThumbnailHeight;
        }

        plImageHeader header;
        header.SetImageFormat(plTextureUtils::GalFormatToImageFormat(format, true));
        header.SetWidth(m_uiThumbnailWidth);
        header.SetHeight(m_uiThumbnailHeight);
        plImage image;
        image.ResetAndAlloc(header);
        PL_ASSERT_DEV(static_cast<plUInt64>(m_uiThumbnailWidth) * static_cast<plUInt64>(m_uiThumbnailHeight) * 4 == header.ComputeDataSize(), "Thumbnail plImage has different size than data buffer!");

        MemDesc.m_pData = image.GetPixelPointer<plUInt8>();
        plArrayPtr<plGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

        plGALTextureSubresource sourceSubResource;
        plArrayPtr<plGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);

        pGALCommandEncoder->CopyTextureReadbackResult(m_hThumbnailColorRT, sourceSubResources, SysMemDescs);

        plImage imageSwap;
        plImage* pImage = &image;
        plImage* pImageSwap = &imageSwap;
        for (plUInt32 uiSuperscaleFactor = ThumbnailSuperscaleFactor; uiSuperscaleFactor > 1; uiSuperscaleFactor /= 2)
        {
          plImageUtils::Scale(*pImage, *pImageSwap, pImage->GetWidth() / 2, pImage->GetHeight() / 2).IgnoreResult();
          plMath::Swap(pImage, pImageSwap);
        }


        ret.m_ThumbnailData.SetCountUninitialized((m_uiThumbnailWidth / ThumbnailSuperscaleFactor) * (m_uiThumbnailHeight / ThumbnailSuperscaleFactor) * 4);
        plMemoryUtils::Copy(ret.m_ThumbnailData.GetData(), pImage->GetPixelPointer<plUInt8>(), ret.m_ThumbnailData.GetCount());
      }

      DestroyThumbnailViewContext();

      // Send response.
      SendProcessMessage(&ret);
    }
    else
    {
      m_pThumbnailViewContext->Redraw(false);
    }
  }
}

plStatus plEngineProcessDocumentContext::ExportDocument(const plExportDocumentMsgToEngine* pMsg)
{
  return plStatus(plFmt("Export document not implemented for '{0}'", GetDynamicRTTI()->GetTypeName()));
}


void plEngineProcessDocumentContext::CreateThumbnailViewContext(const plCreateThumbnailMsgToEngine* pMsg)
{
  PL_ASSERT_DEV(!plEditorEngineProcessApp::GetSingleton()->IsRemoteMode(), "Wrong mode for thumbnail creation");
  PL_ASSERT_DEV(m_pThumbnailViewContext == nullptr, "Thumbnail rendering already in progress.");
  PL_CHECK_AT_COMPILETIME_MSG((ThumbnailSuperscaleFactor & (ThumbnailSuperscaleFactor - 1)) == 0, "ThumbnailSuperscaleFactor must be power of 2.");
  m_uiThumbnailConvergenceFrames = 0;
  m_uiThumbnailWidth = pMsg->m_uiWidth * ThumbnailSuperscaleFactor;
  m_uiThumbnailHeight = pMsg->m_uiHeight * ThumbnailSuperscaleFactor;
  m_pThumbnailViewContext = CreateViewContext();

  // make sure the world is not simulating while making a screenshot
  {
    PL_LOCK(m_pWorld->GetWriteMarker());
    m_bWorldSimStateBeforeThumbnail = m_pWorld->GetWorldSimulationEnabled();
    m_pWorld->SetWorldSimulationEnabled(false);
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Create render target for picking
  plGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView = false;
  tcd.m_bAllowUAV = false;
  tcd.m_bCreateRenderTarget = true;
  tcd.m_Format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type = plGALTextureType::Texture2D;
  tcd.m_uiWidth = m_uiThumbnailWidth;
  tcd.m_uiHeight = m_uiThumbnailHeight;

  m_hThumbnailColorRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = plGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = false;

  m_hThumbnailDepthRT = pDevice->CreateTexture(tcd);

  m_ThumbnailRenderTargets.m_hRTs[0] = m_hThumbnailColorRT;
  m_ThumbnailRenderTargets.m_hDSTarget = m_hThumbnailDepthRT;
  m_pThumbnailViewContext->SetupRenderTarget({}, &m_ThumbnailRenderTargets, m_uiThumbnailWidth, m_uiThumbnailHeight);

  plResourceManager::ForceNoFallbackAcquisition(3);
  OnThumbnailViewContextRequested();
  UpdateThumbnailViewContext(m_pThumbnailViewContext);

  // disable editor specific render passes in the thumbnail view
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_pThumbnailViewContext->GetViewHandle(), pView))
  {
    pView->SetViewRenderMode(plViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "Active", false);

    for (const plString& sTag : pMsg->m_ViewExcludeTags)
    {
      pView->m_ExcludeTags.SetByName(sTag);
    }
  }

  m_pThumbnailViewContext->Redraw(false);

  OnThumbnailViewContextCreated();
}

void plEngineProcessDocumentContext::DestroyThumbnailViewContext()
{
  OnDestroyThumbnailViewContext();

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  DestroyViewContext(m_pThumbnailViewContext);
  m_pThumbnailViewContext = nullptr;

  if (!m_hThumbnailColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hThumbnailColorRT);
    m_hThumbnailColorRT.Invalidate();
  }

  if (!m_hThumbnailDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hThumbnailDepthRT);
    m_hThumbnailDepthRT.Invalidate();
  }

  m_pWorld->SetWorldSimulationEnabled(m_bWorldSimStateBeforeThumbnail);
}

bool plEngineProcessDocumentContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  plLog::Error("UpdateThumbnailViewContext not implemented for '{0}'", GetDynamicRTTI()->GetTypeName());
  return true;
}

void plEngineProcessDocumentContext::OnThumbnailViewContextCreated() {}
void plEngineProcessDocumentContext::OnDestroyThumbnailViewContext() {}

void plEngineProcessDocumentContext::SetTagOnObject(const plUuid& object, const char* szTag, bool bSet, bool recursive)
{
  plGameObjectHandle hObject = GetContext().m_GameObjectMap.GetHandle(object);

  const plTag& tag = plTagRegistry::GetGlobalRegistry().RegisterTag(szTag);

  plGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (recursive)
    {
      if (bSet)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (bSet)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void plEngineProcessDocumentContext::SetTagRecursive(plGameObject* pObject, const plTag& tag)
{
  pObject->SetTag(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {

    SetTagRecursive(itChild, tag);
  }
}

void plEngineProcessDocumentContext::ClearTagRecursive(plGameObject* pObject, const plTag& tag)
{
  pObject->RemoveTag(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    ClearTagRecursive(itChild, tag);
  }
}

void plEngineProcessDocumentContext::WorldRttiConverterContextEventHandler(const plWorldRttiConverterContext::Event& e)
{
  if (e.m_Type == plWorldRttiConverterContext::Event::Type::GameObjectCreated)
  {
    // see whether the newly created object is already referenced by other objects
    auto it = m_GoRef_ReferencedBy.Find(e.m_ObjectGuid);
    if (!it.IsValid())
      return;

    plStringBuilder tmp;

    // iterate over all objects that may reference the new object
    for (const auto& ref : it.Value())
    {
      const plUuid compGuid = ref.m_ReferencedByComponent;
      if (!compGuid.IsValid())
        continue;

      // check whether the object that references the new object is already known (may be dead or not yet created as well)
      plComponentHandle hRefComp = GetContext().m_ComponentMap.GetHandle(compGuid);

      if (hRefComp.IsInvalidated())
        continue;

      plComponent* pRefComp = nullptr;
      if (!GetWorld()->TryGetComponent(hRefComp, pRefComp))
        continue;

      if (!ref.m_sComponentProperty.IsEmpty())
      {
        // in this case, a 'regular' component+property reference the new object
        // so we can just re-apply the reference (by setting the property again)
        // and thus trigger that the other object updates/fixes its internal state

        const plAbstractProperty* pAbsProp = pRefComp->GetDynamicRTTI()->FindPropertyByName(ref.m_sComponentProperty);
        if (pAbsProp == nullptr)
          continue;

        if (pAbsProp->GetCategory() != plPropertyCategory::Member)
          continue;

        plConversionUtils::ToString(e.m_ObjectGuid, tmp);

        plReflectionUtils::SetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pAbsProp), pRefComp, tmp.GetData());
      }
      else
      {
        // in this case, the object was referenced from a component inside a prefab
        // the problem with prefabs is, that their internal objects and components are recreated all the time
        // (e.g. every time any exposed parameter is changed)
        // and since we have no GUIDs for the internal objects, we cannot directly update those internal references
        // we can, however, just update the entire prefab, which will kill all internal objects and recreate them

        plPrefabReferenceComponent* pPrefab = plDynamicCast<plPrefabReferenceComponent*>(pRefComp);
        PL_ASSERT_DEV(pPrefab != nullptr, "Game-Object reference update: Expected an plPrefabReferenceComponent");

        plPrefabReferenceComponentManager* pManager = plStaticCast<plPrefabReferenceComponentManager*>(pPrefab->GetOwningManager());
        pManager->AddToUpdateList(pPrefab);
      }
    }
  }
  else if (e.m_Type == plWorldRttiConverterContext::Event::Type::GameObjectDeleted)
  {
    m_GoRef_ReferencesTo.Remove(e.m_ObjectGuid);
  }
}

/// Tries to resolve a 'reference' (given in pData) to an plGameObject.
/// hThis is the 'owner' of the reference and szComponentProperty is the name of the reference property in that component.
///
/// There are two different use cases:
///
///  1) hThis is invalid and szComponentProperty is null:
///
///     This is used by plPrefabReferenceComponent::SerializeComponent() to check whether a string represents a game object reference.
///     It may be any arbitrary string and thus must not assert.
///     In this case a reference is always a stringyfied GUID.
///     Since this is only used for scene export, only the lookup shall be done and nothing else.
///
///  2) hThis and szComponentProperty represent a valid component+property combination:
///
///     This is called at edit time whenever a reference property is queried, which also happens whenever a reference is modified.
///     In this case we need to maintain two maps:
///       one that know which object references which other objects
///       one that knows by which other objects an object is referenced
///     These are needed to fix up references during undo/redo when objects get deleted and recreated.
///     Ie. when an object that has references or is referenced gets deleted and then undo restores it, the references should appear as well.
///
plGameObjectHandle plEngineProcessDocumentContext::ResolveStringToGameObjectHandle(const void* pData, plComponentHandle hThis, plStringView sComponentProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pData);

  if (hThis.IsInvalidated() && sComponentProperty.IsEmpty())
  {
    // This code path is used by plPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!plConversionUtils::IsStringUuid(szTargetGuid))
      return plGameObjectHandle();

    // convert string to GUID and check if references a known object
    return GetContext().m_GameObjectMap.GetHandle(plConversionUtils::ConvertStringToUuid(szTargetGuid));
  }



  plUuid srcComponentGuid = GetContext().m_ComponentMap.GetGuid(hThis);
  if (!srcComponentGuid.IsValid())
  {
    // if we do not know hThis, it is usually a component that was created by a prefab instance
    // since we need hThis/srcComponentGuid to update our tables who references whom, we now try to walk up the node hierarchy
    // until we find a known game object
    // there, currently, we assume to find an plPrefabReferenceComponent, which will be used as srcComponentGuid

    plComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hThis, pComponent))
      return plGameObjectHandle();

    plGameObject* pOwner = pComponent->GetOwner();

    // search the parents for a known game object
    while (pOwner)
    {
      srcComponentGuid = GetContext().m_GameObjectMap.GetGuid(pOwner->GetHandle());

      if (srcComponentGuid.IsValid())
        break;

      pOwner = pOwner->GetParent();
    }

    // currently we assume all these conditions should be met
    // have to check with reality, though
    PL_ASSERT_DEV(pOwner != nullptr, "Expected a known top level object");
    PL_ASSERT_DEV(srcComponentGuid.IsValid(), "Expected a known top level object");

    // for the time being we assume to find a prefab component here, but this may change if new use cases come up
    plPrefabReferenceComponent* pPrefabComponent;
    if (pOwner->TryGetComponentOfBaseType(pPrefabComponent))
    {
      srcComponentGuid = GetContext().m_ComponentMap.GetGuid(pPrefabComponent->GetHandle());
      PL_ASSERT_DEV(srcComponentGuid.IsValid(), "");

      // tag this reference as being special
      sComponentProperty = {};
    }
    else
    {
      // this could probably happen if we have a component that creates sub-objects even at edit time
      // and is not a prefab reference component
      // and uses game object references
      PL_ASSERT_DEV(false, "Expected a known plPrefabReferenceComponent as top-level object");
      return plGameObjectHandle();
    }
  }


  plUuid newTargetGuid;
  plUuid oldTargetGuid;

  if (plConversionUtils::IsStringUuid(szTargetGuid))
  {
    newTargetGuid = plConversionUtils::ConvertStringToUuid(szTargetGuid);
  }
  else
  {
    PL_ASSERT_DEV(plStringUtils::IsNullOrEmpty(szTargetGuid), "Expected GUID references");
  }

  if (sComponentProperty.IsEmpty())
  {
    return GetContext().m_GameObjectMap.GetHandle(newTargetGuid);
  }

  // overview for the steps below:
  //
  // check if m_GoRef_ReferencesTo[srcComponentGuid] already maps from [szComponentProperty] to something -> update (remove if pData is empty/invalid)
  // otherwise add reference
  //
  // if already mapped to something, remove reference from m_GoRef_ReferencedBy
  // then add new reference to m_GoRef_ReferencedBy



  // update which object this component+property map to
  {
    auto& referencesTo = m_GoRef_ReferencesTo[srcComponentGuid];

    // check all references from the src component
    for (plUInt32 i = 0; i < referencesTo.GetCount(); ++i)
    {
      // if this is the desired property, update it
      if (referencesTo[i].m_sComponentProperty == sComponentProperty)
      {
        // retrieve previous reference, needed to update m_GoRef_ReferencedBy
        oldTargetGuid = referencesTo[i].m_ReferenceToGameObject;

        // write the new reference
        if (newTargetGuid.IsValid())
        {
          referencesTo[i].m_ReferenceToGameObject = newTargetGuid;
        }
        else
        {
          referencesTo.RemoveAtAndSwap(i);
        }

        goto ref_to_is_updated;
      }
    }

    // if we end up here, the reference-to was previously unknown and must be added
    if (newTargetGuid.IsValid())
    {
      auto& refTo = referencesTo.ExpandAndGetRef();
      refTo.m_sComponentProperty = sComponentProperty;
      refTo.m_ReferenceToGameObject = newTargetGuid;
    }
  }

ref_to_is_updated:

  // only need to updated m_GoRef_ReferencedBy if the reference has actually changed
  if (oldTargetGuid != newTargetGuid)
  {
    // if we referenced another object previously, remove the 'referenced-by' link to the old object
    if (oldTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[oldTargetGuid];

      for (plUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid && referencedBy[i].m_sComponentProperty == sComponentProperty)
        {
          referencedBy.RemoveAtAndSwap(i);
          break;
        }
      }
    }

    // if we are now referencing a valid object, add a 'referenced-by' link to the new object
    if (newTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[newTargetGuid];

      // this loop is currently only to validate that no bugs creeped in
      for (plUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid && referencedBy[i].m_sComponentProperty == sComponentProperty)
        {
          PL_REPORT_FAILURE("Go-reference was not updated correctly");
        }
      }

      // add the back-reference
      auto& newRef = referencedBy.ExpandAndGetRef();
      newRef.m_ReferencedByComponent = srcComponentGuid;
      newRef.m_sComponentProperty = sComponentProperty;
    }
  }

  // just an optimization for the common case
  if (!newTargetGuid.IsValid())
    return plGameObjectHandle();

  return GetContext().m_GameObjectMap.GetHandle(newTargetGuid);
}

void plEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto it : m_SyncObjects)
  {
    auto pSyncObject = it.Value();

    if (pSyncObject->GetModified())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      PL_LOCK(m_pWorld->GetWriteMarker());

      if (pSyncObject->SetupForEngine(m_pWorld, GetContext().m_uiNextComponentPickingID))
      {
        GetContext().m_OtherPickingMap.RegisterObject(pSyncObject->GetGuid(), GetContext().m_uiNextComponentPickingID);
        ++GetContext().m_uiNextComponentPickingID;
      }

      pSyncObject->UpdateForEngine(m_pWorld);
    }
  }
}
