#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace plMeshImportUtils
{
  void FillFileFilter(plDynamicArray<plString>& out_list, plStringView sSeparated)
  {
    sSeparated.Split(false, out_list, ";", "*", ".");
  }

  plString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, plStringView sTexturePath, plModelImporter2::TextureSemantic hint, bool bTextureClamp, const plModelImporter2::Importer* pImporter)
  {
    if (!plUnicodeUtils::IsValidUtf8(sTexturePath.GetStartPointer(), sTexturePath.GetEndPointer()))
    {
      plLog::Error("Texture to resolve is not a valid UTF-8 string.");
      return plString();
    }

    plHybridArray<plString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, plFileBrowserAttribute::ImagesLdrAndHdr);

    plStringBuilder sFinalTextureName;
    plPathUtils::MakeValidFilename(sTexturePath.GetFileName(), '_', sFinalTextureName);

    plStringBuilder relTexturePath = szImportSourceFolder;
    relTexturePath.AppendPath(sFinalTextureName);

    if (auto itTex = pImporter->m_OutputTextures.Find(sTexturePath); itTex.IsValid())
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        plLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        return plString();
      }

      itTex.Value().GenerateFileName(sFinalTextureName);

      plStringBuilder sEmbededFile;
      sEmbededFile = szImportTargetFolder;
      sEmbededFile.AppendPath(sFinalTextureName);

      relTexturePath = sEmbededFile;
    }

    plStringBuilder newAssetPathAbs = szImportTargetFolder;
    newAssetPathAbs.AppendPath(sFinalTextureName);
    newAssetPathAbs.ChangeFileExtension("plTextureAsset");

    if (auto textureAssetInfo = plAssetCurator::GetSingleton()->FindSubAsset(newAssetPathAbs))
    {
      // Try to resolve.

      plStringBuilder guidString;
      return plConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, guidString);
    }
    else
    {
      // Import otherwise

      plTextureAssetDocument* textureDocument = plDynamicCast<plTextureAssetDocument*>(plQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, plDocumentFlags::None));
      if (!textureDocument)
      {
        plLog::Error("Failed to create new texture asset '{0}'", sTexturePath);
        return sFinalTextureName;
      }

      plObjectAccessorBase* pAccessor = textureDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Import Texture");
      plDocumentObject* pTextureAsset = textureDocument->GetPropertyObject();

      if (plAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions).Failed())
      {
        relTexturePath = sFinalTextureName;
      }

      pAccessor->SetValue(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

      plEnum<plTexture2DChannelMappingEnum> channelMapping;

      // Try to map usage.
      plEnum<plTexConvUsage> usage;
      switch (hint)
      {
        case plModelImporter2::TextureSemantic::DiffuseMap:
          usage = plTexConvUsage::Color;
          break;

        case plModelImporter2::TextureSemantic::DiffuseAlphaMap:
          usage = plTexConvUsage::Color;
          channelMapping = plTexture2DChannelMappingEnum::RGBA1;
          break;
        case plModelImporter2::TextureSemantic::OcclusionMap: // Making wild guesses here.
        case plModelImporter2::TextureSemantic::EmissiveMap:
          usage = plTexConvUsage::Color;
          break;

        case plModelImporter2::TextureSemantic::RoughnessMap:
        case plModelImporter2::TextureSemantic::MetallicMap:
          channelMapping = plTexture2DChannelMappingEnum::R1;
          usage = plTexConvUsage::Linear;
          break;

        case plModelImporter2::TextureSemantic::OrmMap:
          channelMapping = plTexture2DChannelMappingEnum::RGB1;
          usage = plTexConvUsage::Linear;
          break;

        case plModelImporter2::TextureSemantic::NormalMap:
          usage = plTexConvUsage::NormalMap;
          break;

        case plModelImporter2::TextureSemantic::DisplacementMap:
          usage = plTexConvUsage::Linear;
          channelMapping = plTexture2DChannelMappingEnum::R1;
          break;

        default:
          usage = plTexConvUsage::Auto;
      }

      pAccessor->SetValue(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValue(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValue(pTextureAsset, "AddressModeU", (int)plImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeV", (int)plImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValue(pTextureAsset, "AddressModeW", (int)plImageAddressMode::Clamp).LogFailure();
      }

      // TODO: Set... something else?

      pAccessor->FinishTransaction();
      textureDocument->SaveDocument().LogFailure();

      plStringBuilder guid;
      plConversionUtils::ToString(textureDocument->GetGuid(), guid);
      textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

      return guid;
    }
  };

  void SetMeshAssetMaterialSlots(plHybridArray<plMaterialResourceSlot, 8>& inout_materialSlots, const plModelImporter2::Importer* pImporter)
  {
    const auto& opt = pImporter->GetImportOptions();

    const plUInt32 uiNumSubmeshes = opt.m_pMeshOutput->GetSubMeshes().GetCount();

    inout_materialSlots.SetCount(uiNumSubmeshes);

    for (const auto& material : pImporter->m_OutputMaterials)
    {
      if (material.m_iReferencedByMesh < 0)
        continue;

      inout_materialSlots[material.m_iReferencedByMesh].m_sLabel = material.m_sName;
    }
  }

  void CopyMeshAssetMaterialSlotToResource(plMeshResourceDescriptor& ref_desc, const plHybridArray<plMaterialResourceSlot, 8>& materialSlots)
  {
    for (plUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    {
      ref_desc.SetMaterial(i, materialSlots[i].m_sResource);
    }
  }

  static void ImportMeshAssetMaterialProperties(plMaterialAssetDocument* pMaterialDoc, const plModelImporter2::OutputMaterial& material, const char* szImportSourceFolder, const char* szImportTargetFolder, const plModelImporter2::Importer* pImporter)
  {
    plStringBuilder materialName = plPathUtils::GetFileName(pMaterialDoc->GetDocumentPath());

    PLASMA_LOG_BLOCK("Apply Material Settings", materialName.GetData());

    plObjectAccessorBase* pAccessor = pMaterialDoc->GetObjectAccessor();
    pAccessor->StartTransaction("Apply Material Settings");
    plDocumentObject* pMaterialAsset = pMaterialDoc->GetPropertyObject();

    plStringBuilder tmp;

    // Set base material.
    plStatus res = pAccessor->SetValue(pMaterialAsset, "BaseMaterial", plConversionUtils::ToString(plMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
    res.LogFailure();
    if (res.Failed())
      return;

    // From now on we're setting shader properties.
    plDocumentObject* pMaterialProperties = pMaterialDoc->GetShaderPropertyObject();

    plVariant propertyValue;

    plString textureAo, textureRoughness, textureMetallic;
    material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::OcclusionMap, textureAo);
    material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::RoughnessMap, textureRoughness);
    material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::MetallicMap, textureMetallic);

    const bool bHasOrmTexture = !textureRoughness.IsEmpty() && ((textureAo == textureRoughness) || (textureMetallic == textureRoughness));


    // Set base texture.
    {
      plString textureDiffuse;

      if (material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::DiffuseMap, textureDiffuse))
      {
        pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "BaseTexture", plVariant(plMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureDiffuse, plModelImporter2::TextureSemantic::DiffuseMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "UseBaseTexture", false).LogFailure();
      }
    }

    // Set Normal Texture / Roughness Texture
    {
      plString textureNormal;

      if (!material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::NormalMap, textureNormal))
      {
        // Due to the lack of options in stuff like obj files, people stuff normals into the bump slot.
        material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::DisplacementMap, textureNormal);
      }

      if (!textureNormal.IsEmpty())
      {
        pAccessor->SetValue(pMaterialProperties, "UseNormalTexture", true).LogFailure();

        pAccessor->SetValue(pMaterialProperties, "NormalTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureNormal, plModelImporter2::TextureSemantic::NormalMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "NormalTexture", plConversionUtils::ToString(plMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData()).LogFailure();
      }
    }

    if (!bHasOrmTexture)
    {
      if (!textureRoughness.IsEmpty())
      {
        pAccessor->SetValue(pMaterialProperties, "UseRoughnessTexture", true).LogFailure();

        pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, plModelImporter2::TextureSemantic::RoughnessMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
      }
    }

    // Set metallic texture
    if (!bHasOrmTexture)
    {
      if (!textureMetallic.IsEmpty())
      {
        pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "MetallicTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureMetallic, plModelImporter2::TextureSemantic::MetallicMap, false, pImporter))).LogFailure();
      }
    }

    // Set emissive texture
    {
      plString textureEmissive;

      if (material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::EmissiveMap, textureEmissive))
      {
        pAccessor->SetValue(pMaterialProperties, "UseEmissiveTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "EmissiveTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureEmissive, plModelImporter2::TextureSemantic::EmissiveMap, false, pImporter))).LogFailure();
      }
    }

    // Set AO texture
    if (!bHasOrmTexture)
    {
      plString textureAo;

      if (material.m_TextureReferences.TryGetValue(plModelImporter2::TextureSemantic::OcclusionMap, textureAo))
      {
        pAccessor->SetValue(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
        pAccessor->SetValue(pMaterialProperties, "OcclusionTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureAo, plModelImporter2::TextureSemantic::OcclusionMap, false, pImporter))).LogFailure();
      }
    }

    // TODO: ambient occlusion texture

    // Set base color property
    if (material.m_Properties.TryGetValue(plModelImporter2::PropertySemantic::DiffuseColor, propertyValue) && propertyValue.IsA<plColor>())
    {
      pAccessor->SetValue(pMaterialProperties, "BaseColor", propertyValue).LogFailure();
    }

    // Set emissive color property
    if (material.m_Properties.TryGetValue(plModelImporter2::PropertySemantic::EmissiveColor, propertyValue) && propertyValue.IsA<plColor>())
    {
      pAccessor->SetValue(pMaterialProperties, "EmissiveColor", propertyValue).LogFailure();
    }

    // Set two-sided property
    if (material.m_Properties.TryGetValue(plModelImporter2::PropertySemantic::TwosidedValue, propertyValue) && propertyValue.IsNumber())
    {
      pAccessor->SetValue(pMaterialProperties, "TWO_SIDED", propertyValue.ConvertTo<bool>()).LogFailure();
    }

    // Set metallic property
    if (material.m_Properties.TryGetValue(plModelImporter2::PropertySemantic::MetallicValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value >= 1.0f)
        value = 1.0f;
      else
        value = 0.0f;

      pAccessor->SetValue(pMaterialProperties, "MetallicValue", value).LogFailure();
    }

    // Set roughness property
    if (material.m_Properties.TryGetValue(plModelImporter2::PropertySemantic::RoughnessValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value > 1.0f)
        value /= 255.0f;

      value = plMath::Clamp(value, 0.0f, 1.0f);
      value = plMath::Lerp(0.4f, 1.0f, value);

      // the extracted roughness value is really just a guess to get started

      pAccessor->SetValue(pMaterialProperties, "RoughnessValue", value).LogFailure();
    }

    // Set ORM Texture
    if (bHasOrmTexture)
    {
      pAccessor->SetValue(pMaterialProperties, "UseOrmTexture", true).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseOcclusionTexture", false).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseRoughnessTexture", false).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "UseMetallicTexture", false).LogFailure();

      pAccessor->SetValue(pMaterialProperties, "MetallicTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "OcclusionTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "RoughnessTexture", "").LogFailure();
      pAccessor->SetValue(pMaterialProperties, "RoughnessValue", 1.0f).LogFailure();
      pAccessor->SetValue(pMaterialProperties, "MetallicValue", 0.0f).LogFailure();

      pAccessor->SetValue(pMaterialProperties, "OrmTexture", plVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, plModelImporter2::TextureSemantic::OrmMap, false, pImporter))).LogFailure();
    }

    // Todo:
    // * Shading Mode
    // * Mask Threshold

    pAccessor->FinishTransaction();
  }

  void ImportMeshAssetMaterials(plHybridArray<plMaterialResourceSlot, 8>& inout_materialSlots, plStringView sDocumentDirectory, const plModelImporter2::Importer* pImporter)
  {
    PLASMA_PROFILE_SCOPE("ImportMeshAssetMaterials");

    plStringBuilder targetDirectory = sDocumentDirectory;
    targetDirectory.RemoveFileExtension();
    targetDirectory.Append("_data/");
    const plStringBuilder sourceDirectory = plPathUtils::GetFileDirectory(pImporter->GetImportOptions().m_sSourceFile);

    plStringBuilder tmp;
    plStringBuilder newResourcePathAbs;

    const plUInt32 uiNumSubmeshes = inout_materialSlots.GetCount();

    plProgressRange range("Importing Materials", uiNumSubmeshes, false);

    plHashTable<const plModelImporter2::OutputMaterial*, plString> importMatToGuid;

    plHybridArray<plDocument*, 32> pendingSaveTasks;

    auto WaitForPendingTasks = [&pendingSaveTasks]()
    {
      PLASMA_PROFILE_SCOPE("WaitForPendingTasks");
      for (plDocument* pDoc : pendingSaveTasks)
      {
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
      }
      pendingSaveTasks.Clear();
    };

    plHybridArray<plString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, plFileBrowserAttribute::ImagesLdrAndHdr);

    for (const auto& itTex : pImporter->m_OutputTextures)
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        plLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        continue;
      }

      plStringBuilder sFinalTextureName;
      itTex.Value().GenerateFileName(sFinalTextureName);

      plStringBuilder sEmbededFile;
      sEmbededFile = targetDirectory;
      sEmbededFile.AppendPath(sFinalTextureName);

      plDeferredFileWriter out;
      out.SetOutput(sEmbededFile, true);
      out.WriteBytes(itTex.Value().m_RawData.GetPtr(), itTex.Value().m_RawData.GetCount()).AssertSuccess();
      out.Close().IgnoreResult();
    }

    for (const auto& impMaterial : pImporter->m_OutputMaterials)
    {
      if (impMaterial.m_iReferencedByMesh < 0)
        continue;

      const plUInt32 subMeshIdx = impMaterial.m_iReferencedByMesh;

      range.BeginNextStep("Importing Material");

      // Didn't find currently set resource, create new imported material.
      if (!plAssetCurator::GetSingleton()->FindSubAsset(inout_materialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToGuid.TryGetValue(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource))
          continue;

        // Put the new asset in the data folder.
        newResourcePathAbs = targetDirectory;
        newResourcePathAbs.AppendPath(impMaterial.m_sName);
        newResourcePathAbs.Append(".plMaterialAsset");

        // Does the generated path already exist? Use it.
        if (const auto assetInfo = plAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs))
        {
          inout_materialSlots[subMeshIdx].m_sResource = plConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
          continue;
        }

        plMaterialAssetDocument* pMaterialDoc = plDynamicCast<plMaterialAssetDocument*>(plQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, plDocumentFlags::AsyncSave));
        if (!pMaterialDoc)
        {
          plLog::Error("Failed to create new material '{0}'", impMaterial.m_sName);
          continue;
        }

        ImportMeshAssetMaterialProperties(pMaterialDoc, impMaterial, sourceDirectory, targetDirectory, pImporter);
        inout_materialSlots[subMeshIdx].m_sResource = plConversionUtils::ToString(pMaterialDoc->GetGuid(), tmp);

        pMaterialDoc->SaveDocumentAsync({});
        pendingSaveTasks.PushBack(pMaterialDoc);

        // we have to flush because materials create worlds in the engine process and there is a world limit of 64
        if (pendingSaveTasks.GetCount() >= 16)
          WaitForPendingTasks();
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material as previous slots.
      if (!inout_materialSlots[subMeshIdx].m_sResource.IsEmpty())
      {
        importMatToGuid.Insert(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource);
      }
    }

    WaitForPendingTasks();
  }

} // namespace plMeshImportUtils
