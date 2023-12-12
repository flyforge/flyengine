#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class plSceneDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneDocumentManager, plAssetDocumentManager);

public:
  plSceneDocumentManager();

private:
  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;
  virtual void InternalCloneDocument(const char* szPath, const char* szClonePath, const plUuid& documentId, const plUuid& seedGuid, const plUuid& cloneGuid, plAbstractObjectGraph* pHeader, plAbstractObjectGraph* pObjects, plAbstractObjectGraph* pTypes) override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  void SetupDefaultScene(plDocument* pDocument);


  plStaticArray<plAssetDocumentTypeDescriptor, 4> m_DocTypeDescs;
};
