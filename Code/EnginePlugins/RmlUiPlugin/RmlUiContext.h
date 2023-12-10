#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class plRenderData;
class plBlackboard;

namespace plRmlUiInternal
{
  class Extractor;
  class EventListener;
} // namespace plRmlUiInternal

class PLASMA_RMLUIPLUGIN_DLL plRmlUiContext final : public Rml::Context
{
public:
  plRmlUiContext(const Rml::String& name);
  ~plRmlUiContext();

public:
  plResult LoadDocumentFromResource(const plRmlUiResourceHandle& hResource);
  plResult LoadDocumentFromString(const plStringView& sContent);

  void UnloadDocument();
  plResult ReloadDocumentFromResource(const plRmlUiResourceHandle& hResource);

  void ShowDocument();
  void HideDocument();

  void UpdateInput(const plVec2& mousePos);
  bool WantsInput() const { return m_bWantsInput; }

  void SetOffset(const plVec2I32& offset);
  void SetSize(const plVec2U32& size);
  void SetDpiScale(float fScale);

  using EventHandler = plDelegate<void(Rml::Event&)>;

  void RegisterEventHandler(const char* szIdentifier, EventHandler handler);
  void DeregisterEventHandler(const char* szIdentifier);

private:
  bool HasDocument() { return GetNumDocuments() > 0; }

  friend class plRmlUi;
  void ExtractRenderData(plRmlUiInternal::Extractor& extractor);

  friend class plRmlUiInternal::EventListener;
  void ProcessEvent(const plHashedString& sIdentifier, Rml::Event& event);

  plVec2I32 m_vOffset = plVec2I32::MakeZero();

  plHashTable<plHashedString, EventHandler> m_EventHandler;

  plUInt64 m_uiExtractedFrame = 0;
  plRenderData* m_pRenderData = nullptr;

  bool m_bWantsInput = false;
};

namespace plRmlUiInternal
{
  class ContextInstancer : public Rml::ContextInstancer
  {
  public:
    virtual Rml::ContextPtr InstanceContext(const Rml::String& name) override;
    virtual void ReleaseContext(Rml::Context* context) override;

  private:
    virtual void Release() override;
  };
} // namespace plRmlUiInternal
