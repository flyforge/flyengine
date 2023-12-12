#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorTest/AssetDocument/AssetDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static PlasmaEditorAssetDocumentTest s_EditorAssetDocumentTest;

const char* PlasmaEditorAssetDocumentTest::GetTestName() const
{
  return "Asset Document Tests";
}

void PlasmaEditorAssetDocumentTest::SetupSubTests()
{
  AddSubTest("Async Save", SubTests::ST_AsyncSave);
  AddSubTest("Save on Transform", SubTests::ST_SaveOnTransform);
}

plResult PlasmaEditorAssetDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return PLASMA_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plResult PlasmaEditorAssetDocumentTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

plTestAppRun PlasmaEditorAssetDocumentTest::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_AsyncSave:
      AsyncSave();
      break;
    case SubTests::ST_SaveOnTransform:
      SaveOnTransform();
      break;
  }
  return plTestAppRun::Quit;
}

void PlasmaEditorAssetDocumentTest::AsyncSave()
{
  plAssetDocument* pDoc = nullptr;
  plStringBuilder sName;
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Document")
  {
    sName = m_sProjectPath;
    sName.AppendPath("mesh.plMeshAsset");
    pDoc = static_cast<plAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow));
    PLASMA_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Save Document")
  {
    // Save doc twice in a row without processing messages and then close it.
    plDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    plObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    plInt32 iOrder = 0;
    plTaskGroupID id = pDoc->SaveDocumentAsync(
      [&iOrder](plDocument* pDoc, plStatus res) {
        PLASMA_TEST_INT(iOrder, 0);
        iOrder = 1;
      },
      true);

    pAcc->StartTransaction("Edit Mesh");
    PLASMA_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    // Saving while another save is in progress should block. This ensures the correct state on disk.
    plString sFile = pAcc->Get<plString>(pMeshAsset, "MeshFile");
    plTaskGroupID id2 = pDoc->SaveDocumentAsync([&iOrder](plDocument* pDoc, plStatus res) {
      PLASMA_TEST_INT(iOrder, 1);
      iOrder = 2;
    });

    // Closing the document should wait for the async save to finish.
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    PLASMA_TEST_INT(iOrder, 2);
    PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(id));
    PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(id2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Verify State of Disk")
  {
    pDoc = static_cast<plAssetDocument*>(m_pApplication->m_pEditorApp->OpenDocument(sName, plDocumentFlags::None));
    plDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    plObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    plString sFile = pAcc->Get<plString>(pMeshAsset, "MeshFile");
    PLASMA_TEST_STRING(sFile, "Meshes/Cube.obj");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}

void PlasmaEditorAssetDocumentTest::SaveOnTransform()
{
  plAssetDocument* pDoc = nullptr;
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create Document")
  {
    plStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh2.plMeshAsset");
    pDoc = static_cast<plAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, plDocumentFlags::RequestWindow));
    PLASMA_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  plObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const plDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transform")
  {
    pAcc->StartTransaction("Edit Mesh");
    PLASMA_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    plTransformStatus res = pDoc->SaveDocument();
    PLASMA_TEST_BOOL(res.Succeeded());

    // Transforming an asset in the background should fail and return NeedsImport as the asset needs to be modified which the background is not allowed to do, e.g. materials need to be created.
    res = plAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), plTransformFlags::ForceTransform | plTransformFlags::BackgroundProcessing);
    PLASMA_TEST_BOOL(res.m_Result == plTransformResult::NeedsImport);

    // Transforming a mesh asset with a mesh reference will trigger the material import and update
    // the materials table which requires a save during transform.
    res = plAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), plTransformFlags::ForceTransform | plTransformFlags::TriggeredManually);
    PLASMA_TEST_BOOL(res.Succeeded());
    ProcessEvents();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Verify Transform")
  {
    // Transforming should have update the mesh asset with new material slots.
    plInt32 iCount = 0;
    PLASMA_TEST_BOOL(pAcc->GetCount(pMeshAsset, "Materials", iCount).Succeeded());
    PLASMA_TEST_INT(iCount, 1);

    plUuid subObject = pAcc->Get<plUuid>(pMeshAsset, "Materials", (plInt64)0);
    PLASMA_TEST_BOOL(subObject.IsValid());
    const plDocumentObject* pSubObject = pAcc->GetObject(subObject);

    plString sLabel = pAcc->Get<plString>(pSubObject, "Label");
    PLASMA_TEST_STRING(sLabel, "initialShadingGroup");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}
