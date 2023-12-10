#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Document/Document.h>

class PLASMA_EDITORFRAMEWORK_DLL plAssetDocumentInfo final : public plDocumentInfo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetDocumentInfo, plDocumentInfo);

public:
  plAssetDocumentInfo();
  virtual ~plAssetDocumentInfo();
  plAssetDocumentInfo(plAssetDocumentInfo&& rhs);
  void operator=(plAssetDocumentInfo&& rhs);
  /// \brief Creates a clone without meta data.
  void CreateShallowClone(plAssetDocumentInfo& out_docInfo) const;
  void ClearMetaData();

  plUInt64 m_uiSettingsHash; ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.

  plSet<plString> m_TransformDependencies; ///< [Data dir relative path or GUID] Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  plSet<plString> m_ThumbnailDependencies; ///< [Data dir relative path or GUID] Files that are used to generate the thumbnail.
  plSet<plString> m_PackageDependencies;   ///< [Data dir relative path or GUID] Files that are needed at runtime and should be packaged with the game.

  plSet<plString> m_Outputs; ///< Additional output this asset produces besides the default one. These are tags like VISUAL_SHADER that are resolved
                             ///< by the plAssetDocumentManager into paths.
  plHashedString m_sAssetsDocumentTypeName;
  plDynamicArray<plReflectedClass*>
    m_MetaInfo; ///< Holds arbitrary objects that store meta-data for the asset document. Mainly used for exposed parameters, but can be any reflected
                ///< type. This array takes ownership of all objects and deallocates them on shutdown.

  const char* GetAssetsDocumentTypeName() const;
  void SetAssetsDocumentTypeName(const char* szSz);

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  const plReflectedClass* GetMetaInfo(const plRTTI* pType) const;

  /// \brief Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  template <typename T>
  const T* GetMetaInfo() const
  {
    return static_cast<const T*>(GetMetaInfo(plGetStaticRTTI<T>()));
  }

private:
  plAssetDocumentInfo(const plAssetDocumentInfo&);
  void operator=(const plAssetDocumentInfo&) = delete;
};
