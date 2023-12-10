#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Declarations.h>

class plEngineProcessDocumentContext;
class plEditorEngineDocumentMsg;
class plViewRedrawMsgToEngine;
class plEditorEngineViewMsg;
class plGALRenderTargetSetup;
class plActor;
struct plGALRenderTargets;

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;

/// \brief Represents the window inside the editor process, into which the engine process renders
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorProcessViewWindow : public plWindowBase
{
public:
  plEditorProcessViewWindow()
  {
    m_hWnd = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }

  ~plEditorProcessViewWindow();

  plResult UpdateWindow(plWindowHandle hParentWindow, plUInt16 uiWidth, plUInt16 uiHeight);

  // Inherited via plWindowBase
  virtual plSizeU32 GetClientAreaSize() const override { return plSizeU32(m_uiWidth, m_uiHeight); }
  virtual plWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void ProcessWindowMessages() override {}
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  plUInt16 m_uiWidth;
  plUInt16 m_uiHeight;

private:
  plWindowHandle m_hWnd;
  plAtomicInteger32 m_iReferenceCount = 0;
};

/// \brief Represents the view/window on the engine process side, holds all data necessary for rendering
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineProcessViewContext
{
public:
  plEngineProcessViewContext(plEngineProcessDocumentContext* pContext);
  virtual ~plEngineProcessViewContext();

  void SetViewID(plUInt32 uiId);

  plEngineProcessDocumentContext* GetDocumentContext() const { return m_pDocumentContext; }

  virtual void HandleViewMessage(const plEditorEngineViewMsg* pMsg);
  virtual void SetupRenderTarget(plGALSwapChainHandle hSwapChain, const plGALRenderTargets* pRenderTargets, plUInt16 uiWidth, plUInt16 uiHeight);
  virtual void Redraw(bool bRenderEditorGizmos);

  /// \brief Focuses camera on the given object
  static bool FocusCameraOnObject(plCamera& inout_camera, const plBoundingBoxSphere& objectBounds, float fFov, const plVec3& vViewDir);

  plViewHandle GetViewHandle() const { return m_hView; }

  void DrawSimpleGrid() const;

protected:
  void SendViewMessage(plEditorEngineViewMsg* pViewMsg);
  void HandleWindowUpdate(plWindowHandle hWnd, plUInt16 uiWidth, plUInt16 uiHeight);
  void OnSwapChainChanged(plGALSwapChainHandle hSwapChain, plSizeU32 size);

  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg);

  /// \brief Returns the handle to the default render pipeline.
  virtual plRenderPipelineResourceHandle CreateDefaultRenderPipeline();

  /// \brief Returns the handle to the debug render pipeline.
  virtual plRenderPipelineResourceHandle CreateDebugRenderPipeline();

  /// \brief Create the actual view.
  virtual plViewHandle CreateView() = 0;

private:
  plEngineProcessDocumentContext* m_pDocumentContext;
  plActor* m_pEditorWndActor = nullptr;

protected:
  plCamera m_Camera;
  plViewHandle m_hView;
  plUInt32 m_uiViewID;
};
