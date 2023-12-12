#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshAssetDocument, 12, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static plMat3 CalculateTransformationMatrix(const plMeshAssetProperties* pProp)
{
  const float us = plMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const plBasisAxis::Enum forwardDir = plBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return plBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

plMeshAssetDocument::plMeshAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plMeshAssetProperties>(szDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

plTransformStatus plMeshAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  plMeshAssetProperties* pProp = GetProperties();

  plMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  if (pProp->m_PrimitiveType == plMeshPrimitive::File)
  {
    PLASMA_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc, !transformFlags.IsSet(plTransformFlags::BackgroundProcessing)));
  }
  else
  {
    CreateMeshFromGeom(pProp, desc);
  }

  range.BeginNextStep("Writing Result");
  desc.Save(stream);

  return plStatus(PLASMA_SUCCESS);
}


void plMeshAssetDocument::CreateMeshFromGeom(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc)
{
  const plMat3 mTransformation = CalculateTransformationMatrix(pProp);

  plGeometry geom;
  //const plMat4 mTrans(mTransformation, plVec3::ZeroVector());

  plGeometry::GeoOptions opt;
  opt.m_Transform = plMat4(mTransformation, plVec3::ZeroVector());

  auto detail1 = pProp->m_uiDetail;
  auto detail2 = pProp->m_uiDetail2;

  if (pProp->m_PrimitiveType == plMeshPrimitive::Box)
  {
    geom.AddBox(plVec3(1.0f), true, opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Capsule)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddCapsule(pProp->m_fRadius, plMath::Max(0.0f, pProp->m_fHeight), plMath::Max<plUInt16>(3, detail1), plMath::Max<plUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Cone)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCone(pProp->m_fRadius, pProp->m_fHeight, pProp->m_bCap, plMath::Max<plUInt16>(3, detail1), opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Cylinder)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;

    geom.AddCylinder(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, pProp->m_bCap, pProp->m_bCap2, plMath::Max<plUInt16>(3, detail1), opt, plMath::Clamp(pProp->m_Angle, plAngle::Degree(0.0f), plAngle::Degree(360.0f)));
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::GeodesicSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 2;

    geom.AddGeodesicSphere(pProp->m_fRadius, plMath::Clamp<plUInt16>(detail1, 0, 6), opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::HalfSphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 16;

    geom.AddHalfSphere(pProp->m_fRadius, plMath::Max<plUInt16>(3, detail1), plMath::Max<plUInt16>(1, detail2), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Pyramid)
  {
    geom.AddPyramid(plVec3(1.0f), pProp->m_bCap, opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Rect)
  {
    opt.m_Transform.Element(2, 0) = -opt.m_Transform.Element(2, 0);
    opt.m_Transform.Element(2, 1) = -opt.m_Transform.Element(2, 1);
    opt.m_Transform.Element(2, 2) = -opt.m_Transform.Element(2, 2);

    geom.AddRectXY(plVec2(1.0f), plMath::Max<plUInt16>(1, detail1), plMath::Max<plUInt16>(1, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Sphere)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    geom.AddSphere(pProp->m_fRadius, plMath::Max<plUInt16>(3, detail1), plMath::Max<plUInt16>(2, detail2), opt);
  }
  else if (pProp->m_PrimitiveType == plMeshPrimitive::Torus)
  {
    // use decent default values, if the user hasn't provided anything themselves
    if (detail1 == 0)
      detail1 = 32;
    if (detail2 == 0)
      detail2 = 32;

    float r1 = pProp->m_fRadius;
    float r2 = pProp->m_fRadius2;

    if (r1 == r2)
      r1 = r2 * 0.5f;

    geom.AddTorus(r1, plMath::Max(r1 + 0.01f, r2), plMath::Max<plUInt16>(3, detail1), plMath::Max<plUInt16>(3, detail2), true, opt);
  }

  geom.TriangulatePolygons(4);
  geom.ComputeTangents();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
    }

    // Set material for mesh.
    if (!pProp->m_Slots.IsEmpty())
      desc.SetMaterial(0, pProp->m_Slots[0].m_sResource);
    else
      desc.SetMaterial(0, "");
  }

  auto& mbd = desc.MeshBufferDesc();
  mbd.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
  mbd.AddStream(plGALVertexAttributeSemantic::TexCoord0, plMeshTexCoordPrecision::ToResourceFormat(pProp->m_TexCoordPrecision));
  mbd.AddStream(plGALVertexAttributeSemantic::Normal, plMeshNormalPrecision::ToResourceFormatNormal(pProp->m_NormalPrecision));
  mbd.AddStream(plGALVertexAttributeSemantic::Tangent, plMeshNormalPrecision::ToResourceFormatTangent(pProp->m_NormalPrecision));

  mbd.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);
  desc.AddSubMesh(mbd.GetPrimitiveCount(), 0, 0);
}

plTransformStatus plMeshAssetDocument::CreateMeshFromFile(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc, bool bAllowMaterialImport)
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
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput = &desc;
  opt.m_MeshNormalsPrecision = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);
  opt.m_bOptimize = pProp->m_bOptimize;

  if (pImporter->Import(opt).Failed())
    return plStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  bool bSlotCountMissmatch = pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount();
  if (pProp->m_bImportMaterials || bSlotCountMissmatch)
  {
    if (!bAllowMaterialImport && bSlotCountMissmatch)
    {
      return plTransformStatus(plTransformResult::NeedsImport);
    }

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

plTransformStatus plMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void plMeshAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_PrimitiveType != plMeshPrimitive::File)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_AssetTransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plMeshAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plMeshAssetDocumentGenerator::plMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
  AddSupportedFileType("vox");
}

plMeshAssetDocumentGenerator::~plMeshAssetDocumentGenerator() {}

void plMeshAssetDocumentGenerator::GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  plStringBuilder baseOutputFile = sParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }

  {
    plAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "MeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }
}

plStatus plMeshAssetDocumentGenerator::Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument)
{
  auto pApp = plQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plMeshAssetDocument* pAssetDoc = plDynamicCast<plMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sDataDirRelativePath);

  if (info.m_sName == "MeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "MeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return plStatus(PLASMA_SUCCESS);
}
