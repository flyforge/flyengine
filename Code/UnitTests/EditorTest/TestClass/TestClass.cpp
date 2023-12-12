#include <EditorTest/EditorTestPCH.h>

#include "TestClass.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QMimeData>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

PlasmaEditorTestApplication::PlasmaEditorTestApplication()
  : plApplication("PlasmaEditor")
{
  EnableMemoryLeakReporting(true);

  m_pEditorApp = new plQtEditorApp;
}

plResult PlasmaEditorTestApplication::BeforeCoreSystemsStartup()
{
  if (SUPER::BeforeCoreSystemsStartup().Failed())
    return PLASMA_FAILURE;

  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("editor");
  plStartup::AddApplicationTag("editorapp");

  plQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());
  return PLASMA_SUCCESS;
}

void PlasmaEditorTestApplication::AfterCoreSystemsShutdown()
{
  plQtEditorApp::GetSingleton()->DeInitQt();

  delete m_pEditorApp;
  m_pEditorApp = nullptr;
}

plApplication::Execution PlasmaEditorTestApplication::Run()
{
  qApp->processEvents();
  return plApplication::Execution::Continue;
}

void PlasmaEditorTestApplication::AfterCoreSystemsStartup()
{
  PLASMA_PROFILE_SCOPE("AfterCoreSystemsStartup");
  // We override the user data dir to not pollute the editor settings.
  plStringBuilder userDataDir = plOSFile::GetUserDataFolder();
  userDataDir.AppendPath("PlasmaEngine Project", "EditorTest");
  userDataDir.MakeCleanPath();

  plQtEditorApp::GetSingleton()->StartupEditor(plQtEditorApp::StartupFlags::SafeMode | plQtEditorApp::StartupFlags::NoRecent | plQtEditorApp::StartupFlags::UnitTest, userDataDir);
  // Disable msg boxes.
  plQtUiServices::SetHeadless(true);
  plFileSystem::SetSpecialDirectory("testout", plTestFramework::GetInstance()->GetAbsOutputPath());

  plFileSystem::AddDataDirectory(">pltest/", "ImageComparisonDataDir", "imgout", plFileSystem::AllowWrites).IgnoreResult();
}

void PlasmaEditorTestApplication::BeforeHighLevelSystemsShutdown()
{
  PLASMA_PROFILE_SCOPE("BeforeHighLevelSystemsShutdown");
  plQtEditorApp::GetSingleton()->ShutdownEditor();
}

//////////////////////////////////////////////////////////////////////////

PlasmaEditorTest::PlasmaEditorTest()
{
  plQtEngineViewWidget::s_FixedResolution = plSizeU32(512, 512);
}

PlasmaEditorTest::~PlasmaEditorTest() = default;

PlasmaEditorTestApplication* PlasmaEditorTest::CreateApplication()
{
  return PLASMA_DEFAULT_NEW(PlasmaEditorTestApplication);
}

plResult PlasmaEditorTest::GetImage(plImage& img)
{
  if (!m_CapturedImage.IsValid())
    return PLASMA_FAILURE;

  img.ResetAndMove(std::move(m_CapturedImage));
  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTest::InitializeTest()
{
  m_pApplication = CreateApplication();
  m_sProjectPath.Clear();

  if (m_pApplication == nullptr)
    return PLASMA_FAILURE;

  PLASMA_SUCCEED_OR_RETURN(plRun_Startup(m_pApplication));

  static bool s_bCheckedReferenceDriver = false;
  static bool s_bIsReferenceDriver = false;
  static bool s_bIsAMDDriver = false;

  if (!s_bCheckedReferenceDriver)
  {
    s_bCheckedReferenceDriver = true;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    plUniquePtr<plGALDevice> pDevice;
    plGALDeviceCreationDescription DeviceInit;

    pDevice = plGALDeviceFactory::CreateDevice(plGameApplication::GetActiveRenderer(), plFoundation::GetDefaultAllocator(), DeviceInit);

    PLASMA_SUCCEED_OR_RETURN(pDevice->Init());

    if (pDevice->GetCapabilities().m_sAdapterName == "Microsoft Basic Render Driver" || pDevice->GetCapabilities().m_sAdapterName.StartsWith_NoCase("Intel(R) UHD Graphics"))
    {
      s_bIsReferenceDriver = true;
    }
    else if (pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("AMD") || pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Radeon"))
    {
      s_bIsAMDDriver = true;
    }

    PLASMA_SUCCEED_OR_RETURN(pDevice->Shutdown());
    pDevice.Clear();
#endif
  }

  if (plGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && s_bIsReferenceDriver)
  {
    // Use different images for comparison when running the D3D11 Reference Device
    plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_D3D11Ref");
  }
  else if (plGameApplication::GetActiveRenderer().IsEqual_NoCase("DX11") && s_bIsAMDDriver)
  {
    // Line rendering on DX11 is different on AMD and requires separate images for tests rendering lines.
    plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_AMD");
  }
  else
  {
    plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("");
  }

  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTest::DeInitializeTest()
{
  CloseCurrentProject();

  if (m_pApplication)
  {
    plRun_Shutdown(m_pApplication);

    PLASMA_DEFAULT_DELETE(m_pApplication);
  }


  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTest::CreateAndLoadProject(const char* name)
{
  PLASMA_PROFILE_SCOPE("CreateAndLoadProject");
  plStringBuilder relPath;
  relPath = ":APPDATA";
  relPath.AppendPath(name);

  plStringBuilder absPath;
  if (plFileSystem::ResolvePath(relPath, &absPath, nullptr).Failed())
  {
    plLog::Error("Failed to resolve project path '{0}'.", relPath);
    return PLASMA_FAILURE;
  }
  if (plOSFile::DeleteFolder(absPath).Failed())
  {
    plLog::Error("Failed to delete old project folder '{0}'.", absPath);
    return PLASMA_FAILURE;
  }

  plStringBuilder projectFile = absPath;
  projectFile.AppendPath("plProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(true, projectFile).Failed())
  {
    plLog::Error("Failed to create project '{0}'.", projectFile);
    return PLASMA_FAILURE;
  }

  m_sProjectPath = absPath;
  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTest::OpenProject(const char* path)
{
  PLASMA_PROFILE_SCOPE("OpenProject");
  plStringBuilder relPath;
  relPath = ">sdk";
  relPath.AppendPath(path);

  plStringBuilder absPath;
  if (plFileSystem::ResolveSpecialDirectory(relPath, absPath).Failed())
  {
    plLog::Error("Failed to resolve project path '{0}'.", relPath);
    return PLASMA_FAILURE;
  }

  // Copy project to temp folder
  plStringBuilder projectName = plPathUtils::GetFileName(path);
  plStringBuilder relTempPath;
  relTempPath = ":APPDATA";
  relTempPath.AppendPath(projectName);

  plStringBuilder absTempPath;
  if (plFileSystem::ResolvePath(relTempPath, &absTempPath, nullptr).Failed())
  {
    plLog::Error("Failed to resolve project temp path '{0}'.", relPath);
    return PLASMA_FAILURE;
  }
  if (plOSFile::DeleteFolder(absTempPath).Failed())
  {
    plLog::Error("Failed to delete old project temp folder '{0}'.", absTempPath);
    return PLASMA_FAILURE;
  }
  if (plOSFile::CopyFolder(absPath, absTempPath).Failed())
  {
    plLog::Error("Failed to copy project '{0}' to temp location: '{1}'.", absPath, absTempPath);
    return PLASMA_FAILURE;
  }


  plStringBuilder projectFile = absTempPath;
  projectFile.AppendPath("plProject");
  if (m_pApplication->m_pEditorApp->CreateOrOpenProject(false, projectFile).Failed())
  {
    plLog::Error("Failed to open project '{0}'.", projectFile);
    return PLASMA_FAILURE;
  }

  m_sProjectPath = absTempPath;
  return PLASMA_SUCCESS;
}

plDocument* PlasmaEditorTest::OpenDocument(const char* subpath)
{
  plStringBuilder fullpath;
  fullpath = m_sProjectPath;
  fullpath.AppendPath(subpath);

  plDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(fullpath, plDocumentFlags::RequestWindow);

  if (pDoc)
  {
    ProcessEvents();
  }

  return pDoc;
}

void PlasmaEditorTest::ExecuteDocumentAction(const char* szActionName, plDocument* pDocument, const plVariant& argument /*= plVariant()*/)
{
  PLASMA_TEST_BOOL(plActionManager::ExecuteAction(nullptr, szActionName, pDocument, argument).Succeeded());
}

plResult PlasmaEditorTest::CaptureImage(plQtDocumentWindow* pWindow, const char* szImageName)
{
  plStringBuilder sImgPath = plOSFile::GetUserDataFolder("EditorTests");
  sImgPath.AppendFormat("/{}.tga", szImageName);

  plOSFile::DeleteFile(sImgPath).IgnoreResult();

  pWindow->CreateImageCapture(sImgPath);

  for (int i = 0; i < 10; ++i)
  {
    ProcessEvents();

    if (plOSFile::ExistsFile(sImgPath))
      break;

    plThreadUtils::Sleep(plTime::Milliseconds(100));
  }

  if (!plOSFile::ExistsFile(sImgPath))
    return PLASMA_FAILURE;

  PLASMA_SUCCEED_OR_RETURN(m_CapturedImage.LoadFrom(sImgPath));

  return PLASMA_SUCCESS;
}

void PlasmaEditorTest::CloseCurrentProject()
{
  PLASMA_PROFILE_SCOPE("CloseCurrentProject");
  m_sProjectPath.Clear();
  m_pApplication->m_pEditorApp->CloseProject();
}

void PlasmaEditorTest::SafeProfilingData()
{
  plFileWriter fileWriter;
  if (fileWriter.Open(":appdata/profiling.json") == PLASMA_SUCCESS)
  {
    plProfilingSystem::ProfilingData profilingData;
    plProfilingSystem::Capture(profilingData);
    profilingData.Write(fileWriter).IgnoreResult();
  }
}

void PlasmaEditorTest::ProcessEvents(plUInt32 uiIterations)
{
  PLASMA_PROFILE_SCOPE("ProcessEvents");
  if (qApp)
  {
    for (plUInt32 i = 0; i < uiIterations; i++)
    {
      qApp->processEvents();
    }
  }
}

std::unique_ptr<QMimeData> PlasmaEditorTest::AssetsToDragMimeData(plArrayPtr<plUuid> assetGuids)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  plStringBuilder tmp;

  stream << (int)1;
  for (plUInt32 i = 0; i < assetGuids.GetCount(); ++i)
  {
    QString sGuid(plConversionUtils::ToString(assetGuids[i], tmp).GetData());
    stream << sGuid;
  }

  mimeData->setData("application/PlasmaEditor.AssetGuid", encodedData);
  return std::move(mimeData);
}

std::unique_ptr<QMimeData> PlasmaEditorTest::ObjectsDragMimeData(const plDeque<const plDocumentObject*>& objects)
{
  std::unique_ptr<QMimeData> mimeData(new QMimeData());
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  int iCount = (int)objects.GetCount();
  stream << iCount;

  for (const plDocumentObject* pObject : objects)
  {
    stream.writeRawData((const char*)&pObject, sizeof(void*));
  }

  mimeData->setData("application/PlasmaEditor.ObjectSelection", encodedData);
  return std::move(mimeData);
}

void PlasmaEditorTest::MoveObjectsToLayer(plScene2Document* pDoc, const plDeque<const plDocumentObject*>& objects, const plUuid& layer, plDeque<const plDocumentObject*>& new_objects)
{
  pDoc->GetSelectionManager()->SetSelection(objects);

  plQtLayerAdapter adapter(pDoc);
  auto mimeData = ObjectsDragMimeData(objects);
  plDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = -1;
  info.m_pMimeData = mimeData.get();
  info.m_sTargetContext = "layertree";
  info.m_TargetDocument = pDoc->GetGuid();
  info.m_TargetObject = pDoc->GetLayerObject(layer)->GetGuid();
  info.m_bCtrlKeyDown = false;
  info.m_bShiftKeyDown = false;
  info.m_pAdapter = &adapter;
  if (!PLASMA_TEST_BOOL(plDragDropHandler::DropOnly(&info)))
    return;

  new_objects = pDoc->GetLayerDocument(layer)->GetSelectionManager()->GetSelection();
}

const plDocumentObject* PlasmaEditorTest::DropAsset(plScene2Document* pDoc, const char* szAssetGuidOrPath, bool bShift /*= false*/, bool bCtrl /*= false*/)
{
  const plAssetCurator::plLockedSubAsset asset = plAssetCurator::GetSingleton()->FindSubAsset(szAssetGuidOrPath);
  if (PLASMA_TEST_BOOL(asset.isValid()))
  {
    plUuid assetGuid = asset->m_Data.m_Guid;
    plArrayPtr<plUuid> assets(&assetGuid, 1);
    auto mimeData = AssetsToDragMimeData(assets);

    plDragDropInfo info;
    info.m_pMimeData = mimeData.get();
    info.m_TargetDocument = pDoc->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_iTargetObjectSubID = 0;
    info.m_bShiftKeyDown = bShift;
    info.m_bCtrlKeyDown = bCtrl;

    if (PLASMA_TEST_BOOL(plDragDropHandler::DropOnly(&info)))
    {
      return pDoc->GetSelectionManager()->GetCurrentObject();
    }
  }
  return {};
}

const plDocumentObject* PlasmaEditorTest::CreateGameObject(plScene2Document* pDoc)
{
  auto pAccessor = pDoc->GetObjectAccessor();
  pAccessor->StartTransaction("Add Game Object");

  plUuid guid;
  PLASMA_TEST_STATUS(pAccessor->AddObject(pDoc->GetObjectManager()->GetRootObject(), "Children", -1, plRTTI::FindTypeByName("plGameObject"), guid));
    pAccessor->FinishTransaction();

  return pAccessor->GetObject(guid);
}
