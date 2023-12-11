#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/Input/InputManager.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Time/Clock.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/GameApplication/GameApplication.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Textures/Texture2DResource.h>

namespace
{
  void* plImguiAllocate(size_t uiSize, void* pUserData)
  {
    plAllocatorBase* pAllocator = static_cast<plAllocatorBase*>(pUserData);
    return pAllocator->Allocate(uiSize, PLASMA_ALIGNMENT_MINIMUM);
  }

  void plImguiDeallocate(void* ptr, void* pUserData)
  {
    if (ptr != nullptr)
    {
      plAllocatorBase* pAllocator = static_cast<plAllocatorBase*>(pUserData);
      pAllocator->Deallocate(ptr);
    }
  }
} // namespace

PLASMA_IMPLEMENT_SINGLETON(plImgui);

plImgui::plImgui(plImguiConfigFontCallback configFontCallback, plImguiConfigStyleCallback configStyleCallback)
  : m_SingletonRegistrar(this)
  , m_Allocator("ImGui", plFoundation::GetDefaultAllocator())
  , m_ConfigStyleCallback(configStyleCallback)
{
  Startup(configFontCallback);
}

plImgui::~plImgui()
{
  Shutdown();
}

void plImgui::SetCurrentContextForView(const plViewHandle& hView)
{
  PLASMA_LOCK(m_ViewToContextTableMutex);

  Context& context = m_ViewToContextTable[hView];
  if (context.m_pImGuiContext == nullptr)
  {
    context.m_pImGuiContext = CreateContext();
  }

  ImGui::SetCurrentContext(context.m_pImGuiContext);

  plUInt64 uiCurrentFrameCounter = plRenderWorld::GetFrameCounter();
  if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
  {
    // Last frame was not rendered. This can happen if a render pipeline with dear imgui renderer is used.
    if (context.m_uiFrameRenderCounter != context.m_uiFrameBeginCounter)
    {
      ImGui::EndFrame();
    }

    BeginFrame(hView);
    context.m_uiFrameBeginCounter = uiCurrentFrameCounter;
  }
}

void plImgui::Startup(plImguiConfigFontCallback configFontCallback)
{
  ImGui::SetAllocatorFunctions(&plImguiAllocate, &plImguiDeallocate, &m_Allocator);

  m_pSharedFontAtlas = PLASMA_DEFAULT_NEW(ImFontAtlas);

  if (configFontCallback.IsValid())
  {
    configFontCallback(*m_pSharedFontAtlas);
  }

  unsigned char* pixels;
  int width, height;
  m_pSharedFontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height); // Load as RGBA 32-bits (75% of the memory is wasted, but default font
                                                                    // is so small) because it is more likely to be compatible with user's
                                                                    // existing shaders. If your ImTextureId represent a higher-level
                                                                    // concept than just a GL texture id, consider calling
                                                                    // GetTexDataAsAlpha8() instead to save on GPU memory.

  plTexture2DResourceHandle hFont = plResourceManager::GetExistingResource<plTexture2DResource>("ImguiFont");

  if (!hFont.IsValid())
  {
    plGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = pixels;
    memoryDesc.m_uiRowPitch = width * 4;
    memoryDesc.m_uiSlicePitch = width * height * 4;

    plTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = width;
    desc.m_DescGAL.m_uiHeight = height;
    desc.m_DescGAL.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = plMakeArrayPtr(&memoryDesc, 1);

    hFont = plResourceManager::GetOrCreateResource<plTexture2DResource>("ImguiFont", std::move(desc));
  }

  m_Textures.PushBack(hFont);

  const size_t id = (size_t)m_Textures.GetCount() - 1;
  m_pSharedFontAtlas->TexID = reinterpret_cast<void*>(id);
}

void plImgui::Shutdown()
{
  m_Textures.Clear();

  m_pSharedFontAtlas = nullptr;

  for (auto it = m_ViewToContextTable.GetIterator(); it.IsValid(); ++it)
  {
    Context& context = it.Value();
    ImGui::DestroyContext(context.m_pImGuiContext);
    context.m_pImGuiContext = nullptr;
  }
  m_ViewToContextTable.Clear();
}

ImGuiContext* plImgui::CreateContext()
{
  // imgui reads the global context pointer WHILE creating a new context
  // so if we don't reset it to null here, it will try to access it, and crash
  // if imgui was active on the same thread before
  ImGui::SetCurrentContext(nullptr);
  ImGuiContext* context = ImGui::CreateContext(m_pSharedFontAtlas.Borrow());
  ImGui::SetCurrentContext(context);

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = 1650;
  cfg.DisplaySize.y = 1080;

  cfg.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
  cfg.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
  cfg.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
  cfg.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
  cfg.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
  cfg.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
  cfg.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
  cfg.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
  cfg.KeyMap[ImGuiKey_End] = ImGuiKey_End;
  cfg.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
  cfg.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
  cfg.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
  cfg.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
  cfg.KeyMap[ImGuiKey_A] = ImGuiKey_A;
  cfg.KeyMap[ImGuiKey_C] = ImGuiKey_C;
  cfg.KeyMap[ImGuiKey_V] = ImGuiKey_V;
  cfg.KeyMap[ImGuiKey_X] = ImGuiKey_X;
  cfg.KeyMap[ImGuiKey_Y] = ImGuiKey_Y;
  cfg.KeyMap[ImGuiKey_Z] = ImGuiKey_Z;

  if (m_ConfigStyleCallback.IsValid())
  {
    m_ConfigStyleCallback(ImGui::GetStyle());
  }

  return context;
}

void plImgui::BeginFrame(const plViewHandle& hView)
{
  plView* pView = nullptr;
  if (!plRenderWorld::TryGetView(hView, pView))
  {
    return;
  }

  auto viewport = pView->GetViewport();
  m_CurrentWindowResolution = plSizeU32(static_cast<plUInt32>(viewport.width), static_cast<plUInt32>(viewport.height));

  ImGuiIO& cfg = ImGui::GetIO();

  cfg.DisplaySize.x = viewport.width;
  cfg.DisplaySize.y = viewport.height;
  cfg.DeltaTime = (float)plClock::GetGlobalClock()->GetTimeDiff().GetSeconds();

  if (m_bPassInputToImgui)
  {
    char szUtf8[8] = "";
    char* pChar = szUtf8;
    plUnicodeUtils::EncodeUtf32ToUtf8(plInputManager::RetrieveLastCharacter(false), pChar);
    cfg.AddInputCharactersUTF8(szUtf8);

    float mousex, mousey;
    plInputManager::GetInputSlotState(plInputSlot_MousePositionX, &mousex);
    plInputManager::GetInputSlotState(plInputSlot_MousePositionY, &mousey);
    cfg.MousePos.x = cfg.DisplaySize.x * mousex;
    cfg.MousePos.y = cfg.DisplaySize.y * mousey;
    cfg.MouseDown[0] = plInputManager::GetInputSlotState(plInputSlot_MouseButton0) >= plKeyState::Pressed;
    cfg.MouseDown[1] = plInputManager::GetInputSlotState(plInputSlot_MouseButton1) >= plKeyState::Pressed;
    cfg.MouseDown[2] = plInputManager::GetInputSlotState(plInputSlot_MouseButton2) >= plKeyState::Pressed;

    cfg.MouseWheel = 0;
    if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelDown) == plKeyState::Pressed)
      cfg.MouseWheel = -1;
    if (plInputManager::GetInputSlotState(plInputSlot_MouseWheelUp) == plKeyState::Pressed)
      cfg.MouseWheel = +1;

    cfg.KeyAlt = plInputManager::GetInputSlotState(plInputSlot_KeyLeftAlt) >= plKeyState::Pressed ||
                 plInputManager::GetInputSlotState(plInputSlot_KeyRightAlt) >= plKeyState::Pressed;
    cfg.KeyCtrl = plInputManager::GetInputSlotState(plInputSlot_KeyLeftCtrl) >= plKeyState::Pressed ||
                  plInputManager::GetInputSlotState(plInputSlot_KeyRightCtrl) >= plKeyState::Pressed;
    cfg.KeyShift = plInputManager::GetInputSlotState(plInputSlot_KeyLeftShift) >= plKeyState::Pressed ||
                   plInputManager::GetInputSlotState(plInputSlot_KeyRightShift) >= plKeyState::Pressed;
    cfg.KeySuper = plInputManager::GetInputSlotState(plInputSlot_KeyLeftWin) >= plKeyState::Pressed ||
                   plInputManager::GetInputSlotState(plInputSlot_KeyRightWin) >= plKeyState::Pressed;

    cfg.KeysDown[ImGuiKey_Tab] = plInputManager::GetInputSlotState(plInputSlot_KeyTab) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_LeftArrow] = plInputManager::GetInputSlotState(plInputSlot_KeyLeft) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_RightArrow] = plInputManager::GetInputSlotState(plInputSlot_KeyRight) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_UpArrow] = plInputManager::GetInputSlotState(plInputSlot_KeyUp) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_DownArrow] = plInputManager::GetInputSlotState(plInputSlot_KeyDown) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageUp] = plInputManager::GetInputSlotState(plInputSlot_KeyPageUp) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_PageDown] = plInputManager::GetInputSlotState(plInputSlot_KeyPageDown) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Home] = plInputManager::GetInputSlotState(plInputSlot_KeyHome) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_End] = plInputManager::GetInputSlotState(plInputSlot_KeyEnd) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Delete] = plInputManager::GetInputSlotState(plInputSlot_KeyDelete) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Backspace] = plInputManager::GetInputSlotState(plInputSlot_KeyBackspace) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Enter] = plInputManager::GetInputSlotState(plInputSlot_KeyReturn) >= plKeyState::Pressed ||
                                   plInputManager::GetInputSlotState(plInputSlot_KeyNumpadEnter) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Escape] = plInputManager::GetInputSlotState(plInputSlot_KeyEscape) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_A] = plInputManager::GetInputSlotState(plInputSlot_KeyA) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_C] = plInputManager::GetInputSlotState(plInputSlot_KeyC) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_V] = plInputManager::GetInputSlotState(plInputSlot_KeyV) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_X] = plInputManager::GetInputSlotState(plInputSlot_KeyX) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Y] = plInputManager::GetInputSlotState(plInputSlot_KeyY) >= plKeyState::Pressed;
    cfg.KeysDown[ImGuiKey_Z] = plInputManager::GetInputSlotState(plInputSlot_KeyZ) >= plKeyState::Pressed;
  }
  else
  {
    cfg.ClearInputCharacters();

    cfg.MousePos.x = -1;
    cfg.MousePos.y = -1;

    cfg.MouseDown[0] = false;
    cfg.MouseDown[1] = false;
    cfg.MouseDown[2] = false;

    cfg.MouseWheel = 0;

    cfg.KeyAlt = false;
    cfg.KeyCtrl = false;
    cfg.KeyShift = false;
    cfg.KeySuper = false;

    for (int i = 0; i <= ImGuiKey_COUNT; ++i)
      cfg.KeysDown[i] = false;
  }

  ImGui::NewFrame();

  m_bImguiWantsInput = cfg.WantCaptureKeyboard || cfg.WantCaptureMouse;
}

#endif

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImgui);