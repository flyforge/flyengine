#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct plSubAsset;
class plPlatformProfile;

class PLASMA_EDITORFRAMEWORK_DLL plAssetDocumentManager : public plDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetDocumentManager, plDocumentManager);

public:
  plAssetDocumentManager();
  ~plAssetDocumentManager();

  /// \brief Opens the asset file and reads the "Header" into the given plAssetDocumentInfo.
  virtual plStatus ReadAssetDocumentInfo(plUniquePtr<plAssetDocumentInfo>& out_pInfo, plStreamReader& inout_stream) const;
  virtual void FillOutSubAssetList(const plAssetDocumentInfo& assetInfo, plDynamicArray<plSubAssetData>& out_subAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual plStatus GetAdditionalOutputs(plDynamicArray<plString>& ref_files) { return plStatus(PLASMA_SUCCESS); }

  // plDocumentManager overrides:
public:
  virtual plStatus CloneDocument(plStringView sPath, plStringView sClonePath, plUuid& inout_cloneGuid) override;

  /// \name Asset Profile Functions
  ///@{
public:
  /// \brief Called by the plAssetCurator when the active asset profile changes to re-compute m_uiAssetProfileHash.
  void ComputeAssetProfileHash(const plPlatformProfile* pAssetProfile);

  /// \brief Returns the hash that was previously computed through ComputeAssetProfileHash().
  PLASMA_ALWAYS_INLINE plUInt64 GetAssetProfileHash() const { return m_uiAssetProfileHash; }

  /// \brief Returns pAssetProfile, or if that is null, plAssetCurator::GetSingleton()->GetActiveAssetProfile().
  static const plPlatformProfile* DetermineFinalTargetProfile(const plPlatformProfile* pAssetProfile);

private:
  virtual plUInt64 ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const;

  // The hash that is combined with the asset document hash to determine whether the document output is up to date.
  // This hash needs to be computed in ComputeAssetProfileHash() and should reflect all important settings from the givne asset profile that
  // affect the asset output for this manager.
  // However, if GeneratesProfileSpecificAssets() return false, the hash must be zero, as then all outputs must be identical in all
  // profiles.
  plUInt64 m_uiAssetProfileHash = 0;

  ///@}
  /// \name Thumbnail Functions
  ///@{
public:
  /// \brief Returns the absolute path to the thumbnail that belongs to the given document.
  virtual plString GenerateResourceThumbnailPath(plStringView sDocumentPath, plStringView sSubAssetName = plStringView());
  virtual bool IsThumbnailUpToDate(plStringView sDocumentPath, plStringView sSubAssetName, plUInt64 uiThumbnailHash, plUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(plStringView sDataDirectory, const plPlatformProfile* pAssetProfile, plDelegate<void(plStringView sGuid, plStringView sPath, plStringView sType)> addEntry) const;
  virtual plString GetAssetTableEntry(const plSubAsset* pSubAsset, plStringView sDataDirectory, const plPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  plString GetAbsoluteOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, plStringView sDataDirectory, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile = nullptr) const;
  virtual bool GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(plStringView sDocumentPath, const plDynamicArray<plString>& outputs, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(plStringView sDocumentPath, plStringView sOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor);

  /// Describes how likely it is that a generated file is 'corrupted', due to dependency issues and such.
  /// For example a prefab may not work correctly, if it was written with a very different C++ plugin state, but this can't be detected later.
  /// Whereas a texture always produces exactly the same output and is thus perfectly reliable.
  /// This is used to clear asset caches selectively, and keep things that are unlikely to be in a broken state.
  enum OutputReliability : plUInt8
  {
    Unknown = 0,
    Good = 1,
    Perfect = 2,
  };

  /// \see OutputReliability
  virtual OutputReliability GetAssetTypeOutputReliability() const { return OutputReliability::Unknown; }

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual plResult OpenPickedDocument(const plDocumentObject* pPickedComponent, plUInt32 uiPartIndex) { return PLASMA_FAILURE; }

  plResult TryOpenAssetDocument(const char* szPathOrGuid);

  /// In case this manager deals with types that need to be force transformed on scene export, it can add the asset type names to this list.
  /// This is only needed for assets that have such special dependencies for their transform step, that the regular dependency tracking doesn't work for them.
  /// Currently the only known case are Collection assets, because they have to manually go through the Package dependencies transitively, which means
  /// that the asset curator can't know when they need to be updated.
  virtual void GetAssetTypesRequiringTransformForSceneExport(plSet<plTempHashedString>& inout_assetTypes){};

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, plUInt64 uiHash, plUInt16 uiTypeVersion);
  static void GenerateOutputFilename(plStringBuilder& inout_sRelativeDocumentPath, const plPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
