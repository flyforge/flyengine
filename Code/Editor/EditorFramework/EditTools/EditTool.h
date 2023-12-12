#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

class plGameObjectDocument;
class plQtGameObjectDocumentWindow;
class plObjectAccessorBase;
class PlasmaEditorInputContext;

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectGizmoInterface
{
public:
  virtual plObjectAccessorBase* GetObjectAccessor() = 0;
  virtual bool CanDuplicateSelection() const = 0;
  virtual void DuplicateSelection() = 0;
};

//////////////////////////////////////////////////////////////////////////

enum class plEditToolSupportedSpaces
{
  LocalSpaceOnly,
  WorldSpaceOnly,
  LocalAndWorldSpace,
};

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectEditTool : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectEditTool, plReflectedClass);

public:
  plGameObjectEditTool();

  void ConfigureTool(plGameObjectDocument* pDocument, plQtGameObjectDocumentWindow* pWindow, plGameObjectGizmoInterface* pInterface);

  plGameObjectDocument* GetDocument() const { return m_pDocument; }
  plQtGameObjectDocumentWindow* GetWindow() const { return m_pWindow; }
  plGameObjectGizmoInterface* GetGizmoInterface() const { return m_pInterface; }
  bool IsActive() const { return m_bIsActive; }
  void SetActive(bool active);

  virtual PlasmaEditorInputContext* GetEditorInputContextOverride() { return nullptr; }
  virtual plEditToolSupportedSpaces GetSupportedSpaces() const { return plEditToolSupportedSpaces::WorldSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const { return false; }
  virtual void GetGridSettings(plGridSettingsMsgToEngine& outGridSettings) {}

protected:
  virtual void OnConfigured() = 0;
  virtual void OnActiveChanged(bool bIsActive) {}

private:
  bool m_bIsActive = false;
  plGameObjectDocument* m_pDocument = nullptr;
  plQtGameObjectDocumentWindow* m_pWindow = nullptr;
  plGameObjectGizmoInterface* m_pInterface = nullptr;
};
