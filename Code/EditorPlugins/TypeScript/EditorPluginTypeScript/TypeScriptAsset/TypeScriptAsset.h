#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Communication/Event.h>

class plTypeScriptAssetDocument;

struct plTypeScriptAssetDocumentEvent
{
  enum class Type
  {
    None,
    ScriptCreated,
    ScriptOpened,
    ScriptTransformed,
  };

  Type m_Type = Type::None;
  plTypeScriptAssetDocument* m_pDocument = nullptr;
};

class plTypeScriptAssetDocument : public plSimpleAssetDocument<plTypeScriptAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeScriptAssetDocument, plSimpleAssetDocument<plTypeScriptAssetProperties>);

public:
  plTypeScriptAssetDocument(const char* szDocumentPath);

  void EditScript();

  const plEvent<const plTypeScriptAssetDocumentEvent&>& GetEvent() const { return m_Events; }

protected:
  void CreateComponentFile(const char* szFile);
  void CreateTsConfigFiles();
  plResult CreateTsConfigFile(const char* szDirectory);

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus ValidateScriptCode();
  plStatus AutoGenerateVariablesCode();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  plEvent<const plTypeScriptAssetDocumentEvent&> m_Events;
};
