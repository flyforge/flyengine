#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshAssetDocument, 8, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimatedMeshAssetDocument::plAnimatedMeshAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plAnimatedMeshAssetProperties>(szDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

plTransformStatus plAnimatedMeshAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  plAnimatedMeshAssetProperties* pProp = GetProperties();

  if (pProp->m_sDefaultSkeleton.IsEmpty())
  {
    return plStatus("Animated mesh doesn't have a default skeleton assigned.");
  }

  plMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  PLASMA_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));

  // the properties object can get invalidated by the CreateMeshFromFile() call
  pProp = GetProperties();

  range.BeginNextStep("Writing Result");

  if (!pProp->m_sDefaultSkeleton.IsEmpty())
  {
    desc.m_hDefaultSkeleton = plResourceManager::LoadResource<plSkeletonResource>(pProp->m_sDefaultSkeleton);
  }

  desc.Save(stream);

  return plStatus(PLASMA_SUCCESS);
}

plStatus plAnimatedMeshAssetDocument::CreateMeshFromFile(plAnimatedMeshAssetProperties* pProp, plMeshResourceDescriptor& desc)
{
  plProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  plStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return plStatus(plFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  plUniquePtr<plModelImporter2::Importer> pImporter = plModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return plStatus("No known importer for this file type.");

  plModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_bImportSkinningData = true;
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput = &desc;
  opt.m_MeshNormalsPrecision = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  opt.m_MeshBoneWeightPrecision = pProp->m_BoneWeightPrecision;
  opt.m_bOptimize = false;
  // opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pImporter->Import(opt).Failed())
    return plStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    plMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      plMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  plMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

  return plStatus(PLASMA_SUCCESS);
}

plTransformStatus plAnimatedMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}


//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plAnimatedMeshAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAnimatedMeshAssetDocumentGenerator::plAnimatedMeshAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

plAnimatedMeshAssetDocumentGenerator::~plAnimatedMeshAssetDocumentGenerator() {}

void plAnimatedMeshAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  }

  {
    plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  }
}

plStatus plAnimatedMeshAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plAnimatedMeshAssetDocument* pAssetDoc = plDynamicCast<plAnimatedMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plAnimatedMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sDataDirRelativePath);

  if (info.m_sName == "AnimatedMeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "AnimatedMeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return plStatus(PLASMA_SUCCESS);
}
