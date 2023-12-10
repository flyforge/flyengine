#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiView.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiDocumentContext, 1, plRTTIDefaultAllocator<plRmlUiDocumentContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "RmlUi"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiDocumentContext::plRmlUiDocumentContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

plRmlUiDocumentContext::~plRmlUiDocumentContext() = default;

void plRmlUiDocumentContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());

  // Preview object
  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("RmlUiPreview");
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    plRmlUiCanvas2DComponent* pComponent = nullptr;
    plRmlUiCanvas2DComponent::CreateComponent(m_pMainObject, pComponent);

    plStringBuilder sResourceGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sResourceGuid);
    m_hMainResource = plResourceManager::LoadResource<plRmlUiResource>(sResourceGuid);

    pComponent->SetRmlResource(m_hMainResource);
  }
}

plEngineProcessViewContext* plRmlUiDocumentContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plRmlUiViewContext, this);
}

void plRmlUiDocumentContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

bool plRmlUiDocumentContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  PLASMA_LOCK(m_pMainObject->GetWorld()->GetWriteMarker());

  m_pMainObject->UpdateLocalBounds();
  plBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  plRmlUiViewContext* pMeshViewContext = static_cast<plRmlUiViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}
