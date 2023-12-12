#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>


class plTestDocumentObjectManager : public plDocumentObjectManager
{
public:
  plTestDocumentObjectManager();
  ~plTestDocumentObjectManager();
};


class plTestDocument : public plDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTestDocument, plDocument);

public:
  plTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror = false);
  ~plTestDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  void ApplyNativePropertyChangesToObjectManager(plDocumentObject* pObject);
  virtual plDocumentInfo* CreateDocumentInfo() override;

  plDocumentObjectMirror m_ObjectMirror;
  plRttiConverterContext m_Context;



private:
  bool m_bUseIPCObjectMirror;
};
