#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct PLASMA_EDITORFRAMEWORK_DLL plGameObjectContextEvent
{
  enum class Type
  {
    ContextAboutToBeChanged,
    ContextChanged,
  };
  Type m_Type;
};

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectContextDocument : public plGameObjectDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectContextDocument, plGameObjectDocument);

public:
  plGameObjectContextDocument(const char* szDocumentPath, plDocumentObjectManager* pObjectManager,
    plAssetDocEngineConnection engineConnectionType = plAssetDocEngineConnection::FullObjectMirroring);
  ~plGameObjectContextDocument();

  plStatus SetContext(plUuid documentGuid, plUuid objectGuid);
  plUuid GetContextDocumentGuid() const;
  plUuid GetContextObjectGuid() const;
  const plDocumentObject* GetContextObject() const;

  mutable plEvent<const plGameObjectContextEvent&> m_GameObjectContextEvents;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void ClearContext();

private:
  plUuid m_ContextDocument;
  plUuid m_ContextObject;
};
