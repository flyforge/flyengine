#include <EditorTest/EditorTestPCH.h>

#include "Project.h"
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static PlasmaEditorTestProject s_EditorTestProject;

const char* PlasmaEditorTestProject::GetTestName() const
{
  return "Project Tests";
}

void PlasmaEditorTestProject::SetupSubTests()
{
  AddSubTest("Create Documents", SubTests::ST_CreateDocuments);
}

plResult PlasmaEditorTestProject::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return PLASMA_FAILURE;

  if (SUPER::CreateAndLoadProject("TestProject").Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plResult PlasmaEditorTestProject::DeInitializeTest()
{
  // For profiling the doc creation.
  // SafeProfilingData();
  if (SUPER::DeInitializeTest().Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plTestAppRun PlasmaEditorTestProject::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  const auto& allDesc = plDocumentManager::GetAllDocumentDescriptors();
  for (auto it : allDesc)
  {
    auto pDesc = it.Value();

    if (pDesc->m_bCanCreate)
    {
      plStringBuilder sName = m_sProjectPath;
      sName.AppendPath(pDesc->m_sDocumentTypeName);
      sName.ChangeFileExtension(pDesc->m_sFileExtension);
      plDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow);
      PLASMA_TEST_BOOL(pDoc);
      ProcessEvents();
    }
  }
  // Make sure the engine process did not crash after creating every kind of document.
  PLASMA_TEST_BOOL(!PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  // TODO: Newly created assets actually do not transform cleanly.
  if (false)
  {
    plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::TriggeredManually).IgnoreResult();

    plUInt32 uiNumAssets;
    plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT> sections;
    plAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

    PLASMA_TEST_INT(sections[plAssetInfo::TransformState::TransformError], 0);
    PLASMA_TEST_INT(sections[plAssetInfo::TransformState::MissingDependency], 0);
    PLASMA_TEST_INT(sections[plAssetInfo::TransformState::MissingReference], 0);
  }
  return plTestAppRun::Quit;
}
