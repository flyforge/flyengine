#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

using namespace AE_NS_FOUNDATION;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetDocument, 4, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plKrautTreeAssetDocument::plKrautTreeAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plKrautTreeAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

//////////////////////////////////////////////////////////////////////////

class KrautStreamIn : public aeStreamIn
{
public:
  plStreamReader* m_pStream = nullptr;

private:
  virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_pStream->ReadBytes(pData, uiSize); }
};

static void GetMaterialLabel(plStringBuilder& ref_sOut, plKrautBranchType branchType, plKrautMaterialType materialType)
{
  ref_sOut.Clear();

  switch (branchType)
  {
    case plKrautBranchType::Trunk1:
    case plKrautBranchType::Trunk2:
    case plKrautBranchType::Trunk3:
      ref_sOut.SetFormat("Trunk {}", (int)branchType - (int)plKrautBranchType::Trunk1 + 1);
      break;

    case plKrautBranchType::MainBranches1:
    case plKrautBranchType::MainBranches2:
    case plKrautBranchType::MainBranches3:
      ref_sOut.SetFormat("Branch {}", (int)branchType - (int)plKrautBranchType::MainBranches1 + 1);
      break;

    case plKrautBranchType::SubBranches1:
    case plKrautBranchType::SubBranches2:
    case plKrautBranchType::SubBranches3:
      ref_sOut.SetFormat("Twig {}", (int)branchType - (int)plKrautBranchType::SubBranches1 + 1);
      break;

    case plKrautBranchType::Twigs1:
    case plKrautBranchType::Twigs2:
    case plKrautBranchType::Twigs3:
      ref_sOut.SetFormat("Twigy {}", (int)branchType - (int)plKrautBranchType::Twigs1 + 1);
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  switch (materialType)
  {
    case plKrautMaterialType::Branch:
      ref_sOut.Append(" - Stem");
      break;
    case plKrautMaterialType::Frond:
      ref_sOut.Append(" - Frond");
      break;
    case plKrautMaterialType::Leaf:
      ref_sOut.Append(" - Leaf");
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

plTransformStatus plKrautTreeAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  plKrautTreeAssetProperties* pProp = GetProperties();

  if (!plPathUtils::HasExtension(pProp->m_sKrautFile, ".tree"))
    return plStatus("Unsupported file format");

  plKrautGeneratorResourceDescriptor desc;

  // read the input data
  {
    plFileReader krautFile;
    if (krautFile.Open(pProp->m_sKrautFile).Failed())
      return plStatus(plFmt("Could not open Kraut file '{0}'", pProp->m_sKrautFile));

    KrautStreamIn kstream;
    kstream.m_pStream = &krautFile;

    plUInt32 uiKrautEditorVersion = 0;
    krautFile >> uiKrautEditorVersion;

    Kraut::Deserializer ts;
    ts.m_pTreeStructure = &desc.m_TreeStructureDesc;
    ts.m_LODs[0] = &desc.m_LodDesc[0];
    ts.m_LODs[1] = &desc.m_LodDesc[1];
    ts.m_LODs[2] = &desc.m_LodDesc[2];
    ts.m_LODs[3] = &desc.m_LodDesc[3];
    ts.m_LODs[4] = &desc.m_LodDesc[4];

    if (!ts.Deserialize(kstream))
    {
      return plStatus(plFmt("Reading the Kraut file failed: '{}'", pProp->m_sKrautFile));
    }
  }

  // find materials
  {
    desc.m_Materials.Clear();

    plStringBuilder materialLabel;

    for (plUInt32 bt = 0; bt < Kraut::BranchType::ENUM_COUNT; ++bt)
    {
      const auto& type = desc.m_TreeStructureDesc.m_BranchTypes[bt];

      if (!type.m_bUsed)
        continue;

      for (plUInt32 gt = 0; gt < Kraut::BranchGeometryType::ENUM_COUNT; ++gt)
      {
        if (!type.m_bEnable[gt])
          continue;

        auto& m = desc.m_Materials.ExpandAndGetRef();

        m.m_MaterialType = static_cast<plKrautMaterialType>((int)plKrautMaterialType::Branch + gt);
        m.m_BranchType = static_cast<plKrautBranchType>((int)plKrautBranchType::Trunk1 + bt);

        GetMaterialLabel(materialLabel, m.m_BranchType, m.m_MaterialType);

        // find the matching material from the user input (don't want to guess an index, in case the list size changed)
        for (const auto& mat : pProp->m_Materials)
        {
          if (mat.m_sLabel == materialLabel)
          {
            m.m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(mat.m_sMaterial);
            break;
          }
        }
      }
    }
  }

  // write the output data
  {
    desc.m_sSurfaceResource = pProp->m_sSurface;
    desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    desc.m_fUniformScaling = pProp->m_fUniformScaling;
    desc.m_fLodDistanceScale = pProp->m_fLodDistanceScale;
    desc.m_GoodRandomSeeds = pProp->m_GoodRandomSeeds;
    desc.m_uiDefaultDisplaySeed = pProp->m_uiRandomSeedForDisplay;
    desc.m_fTreeStiffness = pProp->m_fTreeStiffness;

    if (desc.Serialize(stream).Failed())
    {
      return plStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  SyncBackAssetProperties(pProp, desc);

  return plStatus(PL_SUCCESS);
}

void plKrautTreeAssetDocument::SyncBackAssetProperties(plKrautTreeAssetProperties*& pProp, const plKrautGeneratorResourceDescriptor& desc)
{
  bool bModified = pProp->m_Materials.GetCount() != desc.m_Materials.GetCount();

  pProp->m_Materials.SetCount(desc.m_Materials.GetCount());

  plStringBuilder newLabel;

  // TODO: match up old and new materials by label name

  for (plUInt32 m = 0; m < pProp->m_Materials.GetCount(); ++m)
  {
    auto& mat = pProp->m_Materials[m];

    GetMaterialLabel(newLabel, desc.m_Materials[m].m_BranchType, desc.m_Materials[m].m_MaterialType);

    if (newLabel != mat.m_sLabel)
    {
      mat.m_sLabel = newLabel;
      bModified = true;
    }
  }

  if (bModified)
  {
    GetObjectAccessor()->StartTransaction("Update Kraut Material Info");
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }
}

plTransformStatus plKrautTreeAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plKrautTreeAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plKrautTreeAssetDocumentGenerator::plKrautTreeAssetDocumentGenerator()
{
  AddSupportedFileType("tree");
}

plKrautTreeAssetDocumentGenerator::~plKrautTreeAssetDocumentGenerator() = default;

void plKrautTreeAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "KrautTreeImport.Tree";
    info.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  }
}

plStatus plKrautTreeAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  out_pGeneratedDocument = pApp->CreateDocument(sInputFileAbs, plDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plKrautTreeAssetDocument* pAssetDoc = plDynamicCast<plKrautTreeAssetDocument*>(out_pGeneratedDocument);

  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", sInputFileRel.GetView());

  return plStatus(PL_SUCCESS);
}
