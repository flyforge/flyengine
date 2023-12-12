#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

class plMaterialAssetDocument;
struct plPropertyMetaStateEvent;
struct PlasmaEditorAppEvent;

struct plMaterialShaderMode
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    BaseMaterial,
    File,
    Custom,

    Default = BaseMaterial
  };
};

struct plMaterialVisualShaderEvent
{
  enum Type
  {
    TransformFailed,
    TransformSucceeded,
    VisualShaderNotUsed,
  };

  Type m_Type;
  plString m_sTransformError;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plMaterialShaderMode);

struct plMaterialAssetPreview
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Ball,
    Sphere,
    Box,
    Plane,

    Default = Ball
  };
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plMaterialAssetPreview);

class plMaterialAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaterialAssetProperties, plReflectedClass);

public:
  plMaterialAssetProperties()
    : m_pDocument(nullptr)
  {
  }

  void SetBaseMaterial(const char* szBaseMaterial);
  const char* GetBaseMaterial() const;

  void SetSurface(const char* szSurface) { m_sSurface = szSurface; }
  const char* GetSurface() const { return m_sSurface; }

  void SetShader(const char* szShader);
  const char* GetShader() const;
  void SetShaderProperties(plReflectedClass* pProperties);
  plReflectedClass* GetShaderProperties() const;
  void SetShaderMode(plEnum<plMaterialShaderMode> mode);
  plEnum<plMaterialShaderMode> GetShaderMode() const { return m_ShaderMode; }

  void SetDocument(plMaterialAssetDocument* pDocument);
  void UpdateShader(bool bForce = false);

  void DeleteProperties();
  void CreateProperties(const char* szShaderPath);

  void SaveOldValues();
  void LoadOldValues();

  plString ResolveRelativeShaderPath() const;
  plString GetAutoGenShaderPathAbs() const;

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

public:
  plString m_sBaseMaterial;
  plString m_sSurface;
  plString m_sShader;

  plMap<plString, plVariant> m_CachedProperties;
  plMaterialAssetDocument* m_pDocument;
  plEnum<plMaterialShaderMode> m_ShaderMode;
};

class plMaterialAssetDocument : public plSimpleAssetDocument<plMaterialAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaterialAssetDocument, plSimpleAssetDocument<plMaterialAssetProperties>);

public:
  plMaterialAssetDocument(const char* szDocumentPath);
  ~plMaterialAssetDocument();

  plDocumentObject* GetShaderPropertyObject();
  const plDocumentObject* GetShaderPropertyObject() const;

  void SetBaseMaterial(const char* szBaseMaterial);

  plStatus WriteMaterialAsset(plStreamWriter& stream, const plPlatformProfile* pAssetProfile, bool bEmbedLowResData) const;

  /// \brief Will make sure that the visual shader is rebuilt.
  /// Typically called during asset transformation, but can be triggered manually to enforce getting visual shader node changes in.
  plStatus RecreateVisualShaderFile(const plAssetFileHeader& AssetHeader);

  /// \brief If shader compilation failed this will modify the output shader file such that transforming it again, will trigger a full
  /// regeneration Otherwise the AssetCurator would early out
  void TagVisualShaderFileInvalid(const plPlatformProfile* pAssetProfile, const char* szError);

  /// \brief Deletes all Visual Shader nodes that are not connected to the output
  void RemoveDisconnectedNodes();

  static plUuid GetLitBaseMaterial();
  static plUuid GetLitAlphaTestBaseMaterial();
  static plUuid GetNeutralNormalMap();

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  plEvent<const plMaterialVisualShaderEvent&> m_VisualShaderEvents;
  plEnum<plMaterialAssetPreview> m_PreviewModel;

protected:
  plUuid GetSeedFromBaseMaterial(const plAbstractObjectGraph* pBaseGraph);
  static plUuid GetMaterialNodeGuid(const plAbstractObjectGraph& graph);
  virtual void UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, const char* szBasePrefab) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;

  void InvalidateCachedShader();
  void EditorEventHandler(const PlasmaEditorAppEvent& e);

private:
  plStringBuilder m_sCheckPermutations;
  static plUuid s_LitBaseMaterial;
  static plUuid s_LitAlphaTextBaseMaterial;
  static plUuid s_NeutralNormalMap;
};

class plMaterialObjectManager : public plVisualShaderNodeManager
{
};
