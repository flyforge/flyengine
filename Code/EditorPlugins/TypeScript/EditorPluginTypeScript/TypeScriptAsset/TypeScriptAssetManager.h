#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

struct plGameObjectDocumentEvent;

class plTypeScriptAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeScriptAssetDocumentManager, plAssetDocumentManager);

public:
  plTypeScriptAssetDocumentManager();
  ~plTypeScriptAssetDocumentManager();

  plTypeScriptTranspiler& GetTranspiler() { return m_Transpiler; }

  void SetupProjectForTypeScript(bool bForce);
  plResult GenerateScriptCompendium(plBitflags<plTransformFlags> transformFlags);

  virtual plStatus GetAdditionalOutputs(plDynamicArray<plString>& files) override;

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  void ToolsProjectEventHandler(const plToolsProjectEvent& e);
  void GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e);

  static void ModifyTsBeforeTranspilation(plStringBuilder& source);

  void InitializeTranspiler();
  void ShutdownTranspiler();

  bool m_bTranspilerLoaded = false;
  bool m_bProjectSetUp = false;
  plTypeScriptTranspiler m_Transpiler;

  plAssetDocumentTypeDescriptor m_DocTypeDesc;

  plMap<plString, plTimestamp> m_CheckedTsFiles;
};

//////////////////////////////////////////////////////////////////////////

class plTypeScriptPreferences : public plPreferences
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeScriptPreferences, plPreferences);

public:
  plTypeScriptPreferences();

  bool m_bAutoUpdateScriptsForSimulation = true;
  bool m_bAutoUpdateScriptsForPlayTheGame = true;
};
