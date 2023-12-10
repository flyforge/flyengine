#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

struct plRenderPipelineContextLoaderConnection
{
  plUuid m_Source;
  plUuid m_Target;
  plString m_SourcePin;
  plString m_TargetPin;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plRenderPipelineContextLoaderConnection);


class plRenderPipelineRttiConverterContext : public plWorldRttiConverterContext
{
public:
  const plRTTI* FindTypeByName(plStringView sName) const override;
};

class PLASMA_ENGINEPLUGINASSETS_DLL plRenderPipelineContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderPipelineContext, plEngineProcessDocumentContext);

public:
  plRenderPipelineContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;

  virtual plStatus ExportDocument(const plExportDocumentMsgToEngine* pMsg) override;

  virtual plWorldRttiConverterContext& GetContext() override;
  virtual const plWorldRttiConverterContext& GetContext() const override;

private:
  plRenderPipelineRttiConverterContext m_RenderPipelineContext;
};
