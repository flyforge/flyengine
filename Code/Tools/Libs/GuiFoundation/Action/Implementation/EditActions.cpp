#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// plEditActions
////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plEditActions::s_hEditCategory;
plActionDescriptorHandle plEditActions::s_hCopy;
plActionDescriptorHandle plEditActions::s_hPaste;
plActionDescriptorHandle plEditActions::s_hPasteAsChild;
plActionDescriptorHandle plEditActions::s_hPasteAtOriginalLocation;
plActionDescriptorHandle plEditActions::s_hDelete;

void plEditActions::RegisterActions()
{
  s_hEditCategory = PLASMA_REGISTER_CATEGORY("EditCategory");
  s_hCopy = PLASMA_REGISTER_ACTION_1("Selection.Copy", plActionScope::Document, "Document", "Ctrl+C", plEditAction, plEditAction::ButtonType::Copy);
  s_hPaste = PLASMA_REGISTER_ACTION_1("Selection.Paste", plActionScope::Document, "Document", "Ctrl+V", plEditAction, plEditAction::ButtonType::Paste);
  s_hPasteAsChild = PLASMA_REGISTER_ACTION_1("Selection.PasteAsChild", plActionScope::Document, "Document", "", plEditAction, plEditAction::ButtonType::PasteAsChild);
  s_hPasteAtOriginalLocation = PLASMA_REGISTER_ACTION_1("Selection.PasteAtOriginalLocation", plActionScope::Document, "Document", "", plEditAction, plEditAction::ButtonType::PasteAtOriginalLocation);
  s_hDelete = PLASMA_REGISTER_ACTION_1("Selection.Delete", plActionScope::Document, "Document", "", plEditAction, plEditAction::ButtonType::Delete);
}

void plEditActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hEditCategory);
  plActionManager::UnregisterAction(s_hCopy);
  plActionManager::UnregisterAction(s_hPaste);
  plActionManager::UnregisterAction(s_hPasteAsChild);
  plActionManager::UnregisterAction(s_hPasteAtOriginalLocation);
  plActionManager::UnregisterAction(s_hDelete);
}

void plEditActions::MapActions(const char* szMapping, const char* szPath, bool bDeleteAction, bool bAdvancedPasteActions)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 3.5f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPaste, sSubPath, 2.0f);

  if (bAdvancedPasteActions)
  {
    pMap->MapAction(s_hPasteAsChild, sSubPath, 2.5f);
    pMap->MapAction(s_hPasteAtOriginalLocation, sSubPath, 2.7f);
  }

  if (bDeleteAction)
    pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}


void plEditActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 10.0f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPasteAsChild, sSubPath, 2.0f);
  pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}


void plEditActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/EditCategory");

  pMap->MapAction(s_hEditCategory, szPath, 10.0f);

  pMap->MapAction(s_hCopy, sSubPath, 1.0f);
  pMap->MapAction(s_hPasteAsChild, sSubPath, 2.0f);
  pMap->MapAction(s_hPasteAtOriginalLocation, sSubPath, 2.5f);
  pMap->MapAction(s_hDelete, sSubPath, 3.0f);
}

////////////////////////////////////////////////////////////////////////
// plEditAction
////////////////////////////////////////////////////////////////////////

plEditAction::plEditAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case plEditAction::ButtonType::Copy:
      SetIconPath(":/GuiFoundation/Icons/Copy.svg");
      break;
    case plEditAction::ButtonType::Paste:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case plEditAction::ButtonType::PasteAsChild:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg"); /// \todo Icon
      break;
    case plEditAction::ButtonType::PasteAtOriginalLocation:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case plEditAction::ButtonType::Delete:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plEditAction::SelectionEventHandler, this));

  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}

plEditAction::~plEditAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plEditAction::SelectionEventHandler, this));
  }
}

void plEditAction::Execute(const plVariant& value)
{
  switch (m_ButtonType)
  {
    case plEditAction::ButtonType::Copy:
    {
      plStringBuilder sMimeType;

      plAbstractObjectGraph graph;
      if (!m_Context.m_pDocument->CopySelectedObjects(graph, sMimeType))
        break;

      // Serialize to string
      plContiguousMemoryStreamStorage streamStorage;
      plMemoryStreamWriter memoryWriter(&streamStorage);
      plAbstractGraphDdlSerializer::Write(memoryWriter, &graph, nullptr, false);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(sMimeType.GetData(), encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    }
    break;

    case plEditAction::ButtonType::Paste:
    case plEditAction::ButtonType::PasteAsChild:
    case plEditAction::ButtonType::PasteAtOriginalLocation:
    {
      // Check for clipboard data of the correct type.
      QClipboard* clipboard = QApplication::clipboard();
      auto mimedata = clipboard->mimeData();

      plHybridArray<plString, 4> MimeTypes;
      m_Context.m_pDocument->GetSupportedMimeTypesForPasting(MimeTypes);

      plInt32 iFormat = -1;
      {
        for (plUInt32 i = 0; i < MimeTypes.GetCount(); ++i)
        {
          if (mimedata->hasFormat(MimeTypes[i].GetData()))
          {
            iFormat = i;
            break;
          }
        }

        if (iFormat < 0)
          break;
      }

      // Paste at current selected object.
      plPasteObjectsCommand cmd;
      cmd.m_sMimeType = MimeTypes[iFormat];

      QByteArray ba = mimedata->data(MimeTypes[iFormat].GetData());
      cmd.m_sGraphTextFormat = ba.data();

      if (m_ButtonType == ButtonType::PasteAsChild)
      {
        if (!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty())
          cmd.m_Parent = m_Context.m_pDocument->GetSelectionManager()->GetSelection().PeekBack()->GetGuid();
      }
      else if (m_ButtonType == ButtonType::PasteAtOriginalLocation)
      {
        cmd.m_bAllowPickedPosition = false;
      }

      auto history = m_Context.m_pDocument->GetCommandHistory();

      history->StartTransaction("Paste");

      if (history->AddCommand(cmd).Failed())
        history->CancelTransaction();
      else
        history->FinishTransaction();
    }
    break;

    case plEditAction::ButtonType::Delete:
    {
      m_Context.m_pDocument->DeleteSelectedObjects();
    }
    break;
  }
}

void plEditAction::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
