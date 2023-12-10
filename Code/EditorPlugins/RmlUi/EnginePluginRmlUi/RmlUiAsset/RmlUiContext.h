#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginRmlUi/EnginePluginRmlUiDLL.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINRMLUI_DLL plRmlUiDocumentContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiDocumentContext, plEngineProcessDocumentContext);

public:
  plRmlUiDocumentContext();
  ~plRmlUiDocumentContext();

  const plRmlUiResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  plGameObject* m_pMainObject = nullptr;
  plRmlUiResourceHandle m_hMainResource;
};
