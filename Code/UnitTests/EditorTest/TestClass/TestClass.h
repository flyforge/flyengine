#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QApplication>
#include <QSettings>
#include <QtNetwork/QHostInfo>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>
#include <memory>

class plSceneDocument;
class QMimeData;
class plScene2Document;

class PlasmaEditorTestApplication : public plApplication
{
public:
  using SUPER = plApplication;

  PlasmaEditorTestApplication();
  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsShutdown() override;
  virtual Execution Run() override;


  virtual void AfterCoreSystemsStartup() override;


  virtual void BeforeHighLevelSystemsShutdown() override;

public:
  plQtEditorApp* m_pEditorApp = nullptr;
};

class PlasmaEditorTest : public plTestBaseClass
{
  using SUPER = plTestBaseClass;

public:
  PlasmaEditorTest();
  ~PlasmaEditorTest();

  virtual PlasmaEditorTestApplication* CreateApplication();
  virtual plResult GetImage(plImage& img) override;

protected:
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;

  plResult CreateAndLoadProject(const char* name);
  /// \brief Opens a project by copying it to a temp location and opening that one.
  /// This ensures that the tests always work on a clean state.
  plResult OpenProject(const char* path);
  plDocument* OpenDocument(const char* subpath);
  void ExecuteDocumentAction(const char* szActionName, plDocument* pDocument, const plVariant& argument = plVariant());
  plResult CaptureImage(plQtDocumentWindow* pWindow, const char* szImageName);

  void CloseCurrentProject();
  void SafeProfilingData();
  void ProcessEvents(plUInt32 uiIterations = 1);

  std::unique_ptr<QMimeData> AssetsToDragMimeData(plArrayPtr<plUuid> assetGuids);
  std::unique_ptr<QMimeData> ObjectsDragMimeData(const plDeque<const plDocumentObject*>& objects);
  void MoveObjectsToLayer(plScene2Document* pDoc, const plDeque<const plDocumentObject*>& objects, const plUuid& layer, plDeque<const plDocumentObject*>& new_objects);
  const plDocumentObject* DropAsset(plScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift = false, bool bCtrl = false);
  const plDocumentObject* CreateGameObject(plScene2Document* pDoc);


  PlasmaEditorTestApplication* m_pApplication = nullptr;
  plString m_sProjectPath;
  plImage m_CapturedImage;
};
