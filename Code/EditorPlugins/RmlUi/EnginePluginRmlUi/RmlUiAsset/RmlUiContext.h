#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginRmlUi/EnginePluginRmlUiDLL.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINRMLUI_DLL plRmlUiDocumentContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiDocumentContext, PlasmaEngineProcessDocumentContext);

public:
  plRmlUiDocumentContext();
  ~plRmlUiDocumentContext();

  const plRmlUiResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  plGameObject* m_pMainObject = nullptr;
  plRmlUiResourceHandle m_hMainResource;
};
