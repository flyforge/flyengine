#pragma once

class plDocument;
class plDocumentManager;
class plDocumentObjectManager;
class plAbstractObjectGraph;

struct plDocumentFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    RequestWindow = PL_BIT(0),        ///< Open the document visibly (not just internally)
    AddToRecentFilesList = PL_BIT(1), ///< Add the document path to the recently used list for users
    AsyncSave = PL_BIT(2),            ///<
    EmptyDocument = PL_BIT(3),        ///< Don't populate a new document with default state (templates etc)
    Default = None,
  };

  struct Bits
  {
    StorageType RequestWindow : 1;
    StorageType AddToRecentFilesList : 1;
    StorageType AsyncSave : 1;
    StorageType EmptyDocument : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plDocumentFlags);


struct PL_TOOLSFOUNDATION_DLL plDocumentTypeDescriptor
{
  plString m_sFileExtension;
  plString m_sDocumentTypeName;
  bool m_bCanCreate = true;
  plString m_sIcon;
  const plRTTI* m_pDocumentType = nullptr;
  plDocumentManager* m_pManager = nullptr;
  plStringView m_sAssetCategory; // passed to plColorScheme::GetCategoryColor() with CategoryColorUsage::AssetMenuIcon

  /// This list is used to decide which asset types can be picked from the asset browser for a property.
  /// The strings are arbitrary and don't need to be registered anywhere else.
  /// An asset may be compatible for multiple scenarios, e.g. a skinned mesh may also be used as a static mesh, but not the other way round.
  /// In such a case the skinned mesh is set to be compatible to both "CompatibleAsset_Mesh_Static" and "CompatibleAsset_Mesh_Skinned", but the non-skinned mesh only to "CompatibleAsset_Mesh_Static".
  /// A component then only needs to specify that it takes an "CompatibleAsset_Mesh_Static" as input, and all asset types that are compatible to that will be browseable.
  plHybridArray<plString, 1> m_CompatibleTypes;
};


struct plDocumentEvent
{
  enum class Type
  {
    ModifiedChanged,
    ReadOnlyChanged,
    EnsureVisible,
    DocumentSaved,
    DocumentRenamed,
    DocumentStatusMsg,
  };

  Type m_Type;
  const plDocument* m_pDocument;

  plStringView m_sStatusMsg;
};

class PL_TOOLSFOUNDATION_DLL plDocumentInfo : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plDocumentInfo, plReflectedClass);

public:
  plDocumentInfo();

  plUuid m_DocumentID;
};
