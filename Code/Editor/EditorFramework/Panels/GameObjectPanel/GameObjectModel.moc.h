#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class plSceneDocument;

class PL_EDITORFRAMEWORK_DLL plQtGameObjectAdapter : public plQtNameableAdapter
{
  Q_OBJECT;

public:
  plQtGameObjectAdapter(plDocumentObjectManager* pObjectManager, plObjectMetaData<plUuid, plDocumentObjectMetaData>* pObjectMetaData = nullptr, plObjectMetaData<plUuid, plGameObjectMetaData>* pGameObjectMetaData = nullptr);
  ~plQtGameObjectAdapter();
  virtual QVariant data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool setData(const plDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

public:
  void DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plGameObjectMetaData>::EventData& e);

protected:
  plDocumentObjectManager* m_pObjectManager = nullptr;
  plGameObjectDocument* m_pGameObjectDocument = nullptr;
  plObjectMetaData<plUuid, plDocumentObjectMetaData>* m_pObjectMetaData = nullptr;
  plObjectMetaData<plUuid, plGameObjectMetaData>* m_pGameObjectMetaData = nullptr;
  plEventSubscriptionID m_GameObjectMetaDataSubscription;
  plEventSubscriptionID m_DocumentObjectMetaDataSubscription;
};

class PL_EDITORFRAMEWORK_DLL plQtGameObjectModel : public plQtDocumentTreeModel
{
  Q_OBJECT

public:
  plQtGameObjectModel(const plDocumentObjectManager* pObjectManager, const plUuid& root = plUuid());
  ~plQtGameObjectModel();
};

