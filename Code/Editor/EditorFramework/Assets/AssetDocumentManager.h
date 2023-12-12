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
  virtual plStatus ReadAssetDocumentInfo(plUniquePtr<plAssetDocumentInfo>& out_pInfo, plStreamReader& stream) const;
  virtual void FillOutSubAssetList(const plAssetDocumentInfo& assetInfo, plDynamicArray<plSubAssetData>& out_SubAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual plStatus GetAdditionalOutputs(plDynamicArray<plString>& files) { return plStatus(PLASMA_SUCCESS); }

  // plDocumentManager overrides:
public:
  virtual plStatus CloneDocument(const char* szPath, const char* szClonePath, plUuid& inout_cloneGuid) override;

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

  virtual void AddEntriesToAssetTable(
    const char* szDataDirectory, const plPlatformProfile* pAssetProfile, plMap<plString, plString>& inout_GuidToPath) const;
  virtual plString GetAssetTableEntry(const plSubAsset* pSubAsset, const char* szDataDirectory, const plPlatformProfile* pAssetProfile) const;

  /// \brief Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  plString GetAbsoluteOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, const char* szDocumentPath, const char* szOutputTag,
    const plPlatformProfile* pAssetProfile = nullptr) const;

  /// \brief Relative to 'AssetCache' folder.
  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDesc, const char* szDataDirectory, const char* szDocumentPath,
    const char* szOutputTag, const plPlatformProfile* pAssetProfile = nullptr) const;
  virtual bool GeneratesProfileSpecificAssets() const = 0;

  bool IsOutputUpToDate(
    const char* szDocumentPath, const plDynamicArray<plString>& outputs, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(
    const char* szDocumentPath, const char* szOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor);

  ///@}


  /// \brief Called by the editor to try to open a document for the matching picking result
  virtual plResult OpenPickedDocument(const plDocumentObject* pPickedComponent, plUInt32 uiPartIndex) { return PLASMA_FAILURE; }

  plResult TryOpenAssetDocument(const char* szPathOrGuid);

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, plUInt64 uiHash, plUInt16 uiTypeVersion);
  static void GenerateOutputFilename(
    plStringBuilder& inout_sRelativeDocumentPath, const plPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
