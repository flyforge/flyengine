#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Memory/CommonAllocators.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <RendererCore/Pipeline/Declarations.h>

#  include <Imgui/imgui.h>

using plTexture2DResourceHandle = plTypedResourceHandle<class plTexture2DResource>;

struct ImGuiContext;

using plImguiConfigFontCallback = plDelegate<void(ImFontAtlas&)>;
using plImguiConfigStyleCallback = plDelegate<void(ImGuiStyle&)>;

/// \brief Singleton class through which one can control the third-party library 'Dear Imgui'
///
/// Instance has to be manually created and destroyed. Do this for example in plGameState::OnActivation()
/// and plGameState::OnDeactivation().
/// You need to call SetCurrentContextForView before you can use the Imgui functions directly.
/// E.g. 'ImGui::Text("Hello, world!");'
/// To prevent Imgui from using mouse and keyboard input (but still do rendering) use SetPassInputToImgui().
/// To prevent your app from using mouse and keyboard input when Imgui has focus, query WantsInput().
///
/// \note Don't forget that to see the GUI on screen, your render pipeline must contain an plImguiExtractor
/// and you need to have an plImguiRenderer set (typically on an plSimpleRenderPass).
class PL_GAMEENGINE_DLL plImgui
{
  PL_DECLARE_SINGLETON(plImgui);

public:
  plImgui(plImguiConfigFontCallback configFontCallback = plImguiConfigFontCallback(),
    plImguiConfigStyleCallback configStyleCallback = plImguiConfigStyleCallback());
  ~plImgui();

  /// \brief Sets the ImGui context for the given view
  void SetCurrentContextForView(const plViewHandle& hView);

  /// \brief Returns the value that was passed to BeginFrame(). Useful for positioning UI elements.
  plSizeU32 GetCurrentWindowResolution() const { return m_CurrentWindowResolution; }

  /// \brief When this is disabled, the GUI will be rendered, but it will not react to any input. Useful if something else shall get
  /// exclusive input.
  void SetPassInputToImgui(bool bPassInput) { m_bPassInputToImgui = bPassInput; }

  /// \brief If this returns true, the GUI wants to use the input, and thus you might want to not use the input for anything else.
  ///
  /// This is the case when the mouse hovers over any window or a text field has keyboard focus.
  bool WantsInput() const { return m_bImguiWantsInput; }

  /// \brief Returns the shared font atlas
  ImFontAtlas& GetFontAtlas() { return *m_pSharedFontAtlas; }

private:
  friend class plImguiExtractor;
  friend class plImguiRenderer;

  void Startup(plImguiConfigFontCallback configFontCallback);
  void Shutdown();

  ImGuiContext* CreateContext();
  void BeginFrame(const plViewHandle& hView);

  plProxyAllocator m_Allocator;

  bool m_bPassInputToImgui = true;
  bool m_bImguiWantsInput = false;
  plSizeU32 m_CurrentWindowResolution;
  plHybridArray<plTexture2DResourceHandle, 4> m_Textures;

  plImguiConfigStyleCallback m_ConfigStyleCallback;

  plUniquePtr<ImFontAtlas> m_pSharedFontAtlas;

  struct Context
  {
    ImGuiContext* m_pImGuiContext = nullptr;
    plUInt64 m_uiFrameBeginCounter = -1;
    plUInt64 m_uiFrameRenderCounter = -1;
  };

  plMutex m_ViewToContextTableMutex;
  plHashTable<plViewHandle, Context> m_ViewToContextTable;
};

#endif
