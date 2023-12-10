#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/LayerContext.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>


// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerContext, 1, plRTTIDefaultAllocator<plLayerContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Layer"),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_FUNCTION_PROPERTY(AllocateContext),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEngineProcessDocumentContext* plLayerContext::AllocateContext(const plDocumentOpenMsgToEngine* pMsg)
{
  if (pMsg->m_DocumentMetaData.IsA<plUuid>())
  {
    return plGetStaticRTTI<plLayerContext>()->GetAllocator()->Allocate<plEngineProcessDocumentContext>();
  }
  else
  {
    return plGetStaticRTTI<plSceneContext>()->GetAllocator()->Allocate<plEngineProcessDocumentContext>();
  }
}

plLayerContext::plLayerContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::None)
{
}

plLayerContext::~plLayerContext()
{
}

void plLayerContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  // Everything in the picking buffer needs a unique ID. As layers and scene share the same world we need to make sure no id is used twice.
  // To achieve this the scene's next ID is retrieved on every change and written back in base new IDs were used up.
  m_Context.m_uiNextComponentPickingID = m_pParentSceneContext->m_Context.m_uiNextComponentPickingID;
  plEngineProcessDocumentContext::HandleMessage(pMsg);
  m_pParentSceneContext->m_Context.m_uiNextComponentPickingID = m_Context.m_uiNextComponentPickingID;

  if (pMsg->IsInstanceOf<plEntityMsgToEngine>())
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());
    m_pParentSceneContext->AddLayerIndexTag(*static_cast<const plEntityMsgToEngine*>(pMsg), m_Context, m_LayerTag);
  }
}

void plLayerContext::SceneDeinitialized()
{
  // If the scene is deinitialized the world is destroyed so there is no use tracking anything further.
  m_pWorld = nullptr;
  m_Context.Clear();
}

const plTag& plLayerContext::GetLayerTag() const
{
  return m_LayerTag;
}

void plLayerContext::OnInitialize()
{
  plUuid parentScene = m_MetaData.Get<plUuid>();
  plEngineProcessDocumentContext* pContext = GetDocumentContext(parentScene);
  m_pParentSceneContext = plDynamicCast<plSceneContext*>(pContext);

  m_pWorld = m_pParentSceneContext->GetWorld();
  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  plUInt32 uiLayerID = m_pParentSceneContext->RegisterLayer(this);
  plStringBuilder sVisibilityTag;
  sVisibilityTag.Format("Layer_{}", uiLayerID);
  m_LayerTag = plTagRegistry::GetGlobalRegistry().RegisterTag(sVisibilityTag);

  plShadowPool::AddExcludeTagToWhiteList(m_LayerTag);
}

void plLayerContext::OnDeinitialize()
{
  if (m_pWorld)
  {
    // If the world still exists we are just unloading the layer not the scene that owns the world.
    // Thus, we need to make sure the layer objects are removed from the still existing world.
    m_Context.DeleteExistingObjects();
  }

  m_LayerTag = plTag();
  m_pParentSceneContext->UnregisterLayer(this);
  m_pParentSceneContext = nullptr;
}

plEngineProcessViewContext* plLayerContext::CreateViewContext()
{
  PLASMA_REPORT_FAILURE("Layers should not create views.");
  return nullptr;
}

void plLayerContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_REPORT_FAILURE("Layers should not create views.");
}

plStatus plLayerContext::ExportDocument(const plExportDocumentMsgToEngine* pMsg)
{
  PLASMA_REPORT_FAILURE("Layers do not support export yet. THe layer content is baked into the main scene instead.");
  return plStatus("Nope");
}

void plLayerContext::UpdateDocumentContext()
{
  SUPER::UpdateDocumentContext();
}
