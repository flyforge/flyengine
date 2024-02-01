#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace
{
  static const char* s_szPlKeys[] = {plInputSlot_KeyTab, plInputSlot_KeyLeft, plInputSlot_KeyUp, plInputSlot_KeyRight, plInputSlot_KeyDown,
    plInputSlot_KeyPageUp, plInputSlot_KeyPageDown, plInputSlot_KeyHome, plInputSlot_KeyEnd, plInputSlot_KeyDelete, plInputSlot_KeyBackspace,
    plInputSlot_KeyReturn, plInputSlot_KeyNumpadEnter, plInputSlot_KeyEscape};

  static Rml::Input::KeyIdentifier s_rmlKeys[] = {Rml::Input::KI_TAB, Rml::Input::KI_LEFT, Rml::Input::KI_UP,
    Rml::Input::KI_RIGHT, Rml::Input::KI_DOWN, Rml::Input::KI_PRIOR, Rml::Input::KI_NEXT, Rml::Input::KI_HOME,
    Rml::Input::KI_END, Rml::Input::KI_DELETE, Rml::Input::KI_BACK, Rml::Input::KI_RETURN, Rml::Input::KI_RETURN,
    Rml::Input::KI_ESCAPE};

  PL_CHECK_AT_COMPILETIME(PL_ARRAY_SIZE(s_szPlKeys) == PL_ARRAY_SIZE(s_rmlKeys));
} // namespace

plRmlUiContext::plRmlUiContext(const Rml::String& sName)
  : Rml::Context(sName)
{
}

plRmlUiContext::~plRmlUiContext() = default;

plResult plRmlUiContext::LoadDocumentFromResource(const plRmlUiResourceHandle& hResource)
{
  UnloadDocument();

  if (hResource.IsValid())
  {
    plResourceLock<plRmlUiResource> pResource(hResource, plResourceAcquireMode::BlockTillLoaded);
    if (pResource.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      LoadDocument(pResource->GetRmlFile().GetData());
    }
  }

  return HasDocument() ? PL_SUCCESS : PL_FAILURE;
}

plResult plRmlUiContext::LoadDocumentFromString(const plStringView& sContent)
{
  UnloadDocument();

  if (!sContent.IsEmpty())
  {
    Rml::String sRmlContent = Rml::String(sContent.GetStartPointer(), sContent.GetElementCount());

    LoadDocumentFromMemory(sRmlContent);
  }

  return HasDocument() ? PL_SUCCESS : PL_FAILURE;
}

void plRmlUiContext::UnloadDocument()
{
  if (HasDocument())
  {
    Rml::Context::UnloadDocument(GetDocument(0));
  }
}

plResult plRmlUiContext::ReloadDocumentFromResource(const plRmlUiResourceHandle& hResource)
{
  Rml::Factory::ClearStyleSheetCache();
  Rml::Factory::ClearTemplateCache();

  return LoadDocumentFromResource(hResource);
}

void plRmlUiContext::ShowDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Show();
  }
}

void plRmlUiContext::HideDocument()
{
  if (HasDocument())
  {
    GetDocument(0)->Hide();
  }
}

void plRmlUiContext::UpdateInput(const plVec2& vMousePos)
{
  float width = static_cast<float>(GetDimensions().x);
  float height = static_cast<float>(GetDimensions().y);

  m_bWantsInput = vMousePos.x >= 0.0f && vMousePos.x <= width && vMousePos.y >= 0.0f && vMousePos.y <= height;

  const bool bCtrlPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftCtrl) >= plKeyState::Pressed ||
                            plInputManager::GetInputSlotState(plInputSlot_KeyRightCtrl) >= plKeyState::Pressed;
  const bool bShiftPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftShift) >= plKeyState::Pressed ||
                             plInputManager::GetInputSlotState(plInputSlot_KeyRightShift) >= plKeyState::Pressed;
  const bool bAltPressed = plInputManager::GetInputSlotState(plInputSlot_KeyLeftAlt) >= plKeyState::Pressed ||
                           plInputManager::GetInputSlotState(plInputSlot_KeyRightAlt) >= plKeyState::Pressed;

  int modifierState = 0;
  modifierState |= bCtrlPressed ? Rml::Input::KM_CTRL : 0;
  modifierState |= bShiftPressed ? Rml::Input::KM_SHIFT : 0;
  modifierState |= bAltPressed ? Rml::Input::KM_ALT : 0;

  // Mouse
  {
    ProcessMouseMove(static_cast<int>(vMousePos.x), static_cast<int>(vMousePos.y), modifierState);

    static const char* szMouseButtons[] = {plInputSlot_MouseButton0, plInputSlot_MouseButton1, plInputSlot_MouseButton2};
    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(szMouseButtons); ++i)
    {
      plKeyState::Enum state = plInputManager::GetInputSlotState(szMouseButtons[i]);
      if (state == plKeyState::Pressed)
      {
        ProcessMouseButtonDown(i, modifierState);
      }
      else if (state == plKeyState::Released)
      {
        ProcessMouseButtonUp(i, modifierState);
      }
    }

    if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelDown) == plKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(1.0f, modifierState);
    }
    if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelUp) == plKeyState::Pressed)
    {
      m_bWantsInput |= !ProcessMouseWheel(-1.0f, modifierState);
    }
  }

  // Keyboard
  {
    plUInt32 uiLastChar = plInputManager::RetrieveLastCharacter(false);
    if (uiLastChar >= 32) // >= space
    {
      char szUtf8[8] = "";
      char* pChar = szUtf8;
      plUnicodeUtils::EncodeUtf32ToUtf8(uiLastChar, pChar);
      if (!plStringUtils::IsNullOrEmpty(szUtf8))
      {
        m_bWantsInput |= !ProcessTextInput(szUtf8);
      }
    }

    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(s_szPlKeys); ++i)
    {
      plKeyState::Enum state = plInputManager::GetInputSlotState(s_szPlKeys[i]);
      if (state == plKeyState::Pressed)
      {
        m_bWantsInput |= !ProcessKeyDown(s_rmlKeys[i], modifierState);
      }
      else if (state == plKeyState::Released)
      {
        m_bWantsInput |= !ProcessKeyUp(s_rmlKeys[i], modifierState);
      }
    }
  }
}

void plRmlUiContext::SetOffset(const plVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void plRmlUiContext::SetSize(const plVec2U32& vSize)
{
  SetDimensions(Rml::Vector2i(vSize.x, vSize.y));
}

void plRmlUiContext::SetDpiScale(float fScale)
{
  SetDensityIndependentPixelRatio(fScale);
}

void plRmlUiContext::RegisterEventHandler(const char* szIdentifier, EventHandler handler)
{
  plHashedString sIdentifier;
  sIdentifier.Assign(szIdentifier);

  m_EventHandler.Insert(sIdentifier, std::move(handler));
}

void plRmlUiContext::DeregisterEventHandler(const char* szIdentifier)
{
  m_EventHandler.Remove(plTempHashedString(szIdentifier));
}

void plRmlUiContext::ExtractRenderData(plRmlUiInternal::Extractor& extractor)
{
  if (m_uiExtractedFrame != plRenderWorld::GetFrameCounter())
  {
    extractor.BeginExtraction(m_vOffset);

    Render();

    extractor.EndExtraction();

    m_uiExtractedFrame = plRenderWorld::GetFrameCounter();
    m_pRenderData = extractor.GetRenderData();
  }
}

void plRmlUiContext::ProcessEvent(const plHashedString& sIdentifier, Rml::Event& event)
{
  EventHandler* pEventHandler = nullptr;
  if (m_EventHandler.TryGetValue(sIdentifier, pEventHandler))
  {
    (*pEventHandler)(event);
  }
}

//////////////////////////////////////////////////////////////////////////

Rml::ContextPtr plRmlUiInternal::ContextInstancer::InstanceContext(const Rml::String& sName)
{
  return Rml::ContextPtr(PL_DEFAULT_NEW(plRmlUiContext, sName));
}

void plRmlUiInternal::ContextInstancer::ReleaseContext(Rml::Context* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

void plRmlUiInternal::ContextInstancer::Release()
{
  // nothing to do here
}
