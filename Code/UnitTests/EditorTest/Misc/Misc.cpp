#include <EditorTest/EditorTestPCH.h>

#include "Misc.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static PlasmaEditorTestMisc s_EditorTestMisc;

const char* PlasmaEditorTestMisc::GetTestName() const
{
  return "Misc Tests";
}

void PlasmaEditorTestMisc::SetupSubTests()
{
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
}

plResult PlasmaEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return PLASMA_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return PLASMA_FAILURE;

  if (plStatus res = plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::None); res.Failed())
  {
    plLog::Error("Asset transform failed: {}", res.m_sMessage);
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTestMisc::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plTestAppRun PlasmaEditorTestMisc::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::GameObjectReferences)
  {
    m_pDocument = SUPER::OpenDocument("Scenes/GameObjectReferences.plScene");

    if (!PLASMA_TEST_BOOL(m_pDocument != nullptr))
      return plTestAppRun::Quit;

    plAssetCurator::GetSingleton()->TransformAsset(m_pDocument->GetGuid(), plTransformFlags::Default);

    plQtEngineDocumentWindow* pWindow = qobject_cast<plQtEngineDocumentWindow*>(plQtDocumentWindow::FindWindowByDocument(m_pDocument));

    if (!PLASMA_TEST_BOOL(pWindow != nullptr))
      return plTestAppRun::Quit;

    auto viewWidgets = pWindow->GetViewWidgets();

    if (!PLASMA_TEST_BOOL(!viewWidgets.IsEmpty()))
      return plTestAppRun::Quit;

    plQtEngineViewWidget::InteractionContext ctxt;
    ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
    plQtEngineViewWidget::SetInteractionContext(ctxt);

    viewWidgets[0]->m_pViewConfig->m_RenderMode = plViewRenderMode::Default;
    viewWidgets[0]->m_pViewConfig->m_Perspective = plSceneViewPerspective::Perspective;
    viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

    ExecuteDocumentAction("Scene.Camera.JumpTo.0", m_pDocument, true);

    for (int i = 0; i < 10; ++i)
    {
      plThreadUtils::Sleep(plTime::Milliseconds(100));
      ProcessEvents();
    }

    PLASMA_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

    PLASMA_TEST_IMAGE(1, 100);

    // Move everything to the layer and repeat the test.
    plScene2Document* pScene = plDynamicCast<plScene2Document*>(m_pDocument);
    plHybridArray<plUuid, 2> layerGuids;
    pScene->GetAllLayers(layerGuids);
    PLASMA_TEST_INT(layerGuids.GetCount(), 2);
    plUuid layerGuid = layerGuids[0] == pScene->GetGuid() ? layerGuids[1] : layerGuids[0];

    auto pAccessor = pScene->GetObjectAccessor();
    auto pRoot = pScene->GetObjectManager()->GetRootObject();
    plHybridArray<plVariant, 16> values;
    pAccessor->GetValues(pRoot, "Children", values).AssertSuccess();

    plDeque<const plDocumentObject*> assets;
    for (auto& value : values)
    {
      assets.PushBack(pAccessor->GetObject(value.Get<plUuid>()));
    }
    plDeque<const plDocumentObject*> newObjects;
    MoveObjectsToLayer(pScene, assets, layerGuid, newObjects);

    PLASMA_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

    PLASMA_TEST_IMAGE(1, 100);
  }


  // const auto& allDesc = plDocumentManager::GetAllDocumentDescriptors();
  // for (auto* pDesc : allDesc)
  //{
  //  if (pDesc->m_bCanCreate)
  //  {
  //    plStringBuilder sName = m_sProjectPath;
  //    sName.AppendPath(pDesc->m_sDocumentTypeName);
  //    sName.ChangeFileExtension(pDesc->m_sFileExtension);
  //    plDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow);
  //    PLASMA_TEST_BOOL(pDoc);
  //    ProcessEvents();
  //  }
  //}
  //// Make sure the engine process did not crash after creating every kind of document.
  // PLASMA_TEST_BOOL(!PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  ////TODO: Newly created assets actually do not transform cleanly.
  // if (false)
  //{
  //  plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::TriggeredManually);

  //  plUInt32 uiNumAssets;
  //  plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT> sections;
  //  plAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  //  PLASMA_TEST_INT(sections[plAssetInfo::TransformState::TransformError], 0);
  //  PLASMA_TEST_INT(sections[plAssetInfo::TransformState::MissingDependency], 0);
  //  PLASMA_TEST_INT(sections[plAssetInfo::TransformState::MissingReference], 0);
  //}
  return plTestAppRun::Quit;
}

plResult PlasmaEditorTestMisc::InitializeSubTest(plInt32 iIdentifier)
{
  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTestMisc::DeInitializeSubTest(plInt32 iIdentifier)
{
  plDocumentManager::CloseAllDocuments();
  return PLASMA_SUCCESS;
}
