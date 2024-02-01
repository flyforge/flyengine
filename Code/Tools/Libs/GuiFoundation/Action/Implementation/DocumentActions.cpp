#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Project/ToolsProject.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// plDocumentActions
////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plDocumentActions::s_hSaveCategory;
plActionDescriptorHandle plDocumentActions::s_hSave;
plActionDescriptorHandle plDocumentActions::s_hSaveAs;
plActionDescriptorHandle plDocumentActions::s_hSaveAll;
plActionDescriptorHandle plDocumentActions::s_hClose;
plActionDescriptorHandle plDocumentActions::s_hCloseAll;
plActionDescriptorHandle plDocumentActions::s_hCloseAllButThis;
plActionDescriptorHandle plDocumentActions::s_hOpenContainingFolder;
plActionDescriptorHandle plDocumentActions::s_hCopyAssetGuid;
plActionDescriptorHandle plDocumentActions::s_hUpdatePrefabs;

void plDocumentActions::RegisterActions()
{
  s_hSaveCategory = PL_REGISTER_CATEGORY("SaveCategory");
  s_hSave = PL_REGISTER_ACTION_1("Document.Save", plActionScope::Document, "Document", "Ctrl+S", plDocumentAction, plDocumentAction::ButtonType::Save);
  s_hSaveAll = PL_REGISTER_ACTION_1("Document.SaveAll", plActionScope::Document, "Document", "Ctrl+Shift+S", plDocumentAction, plDocumentAction::ButtonType::SaveAll);
  s_hSaveAs = PL_REGISTER_ACTION_1("Document.SaveAs", plActionScope::Document, "Document", "", plDocumentAction, plDocumentAction::ButtonType::SaveAs);
  s_hClose = PL_REGISTER_ACTION_1("Document.Close", plActionScope::Document, "Document", "Ctrl+W", plDocumentAction, plDocumentAction::ButtonType::Close);
  s_hCloseAll = PL_REGISTER_ACTION_1("Document.CloseAll", plActionScope::Document, "Document", "Ctrl+Shift+W", plDocumentAction, plDocumentAction::ButtonType::CloseAll);
  s_hCloseAllButThis = PL_REGISTER_ACTION_1("Document.CloseAllButThis", plActionScope::Document, "Document", "Shift+Alt+W", plDocumentAction, plDocumentAction::ButtonType::CloseAllButThis);
  s_hOpenContainingFolder = PL_REGISTER_ACTION_1("Document.OpenContainingFolder", plActionScope::Document, "Document", "", plDocumentAction, plDocumentAction::ButtonType::OpenContainingFolder);
  s_hCopyAssetGuid = PL_REGISTER_ACTION_1("Document.CopyAssetGuid", plActionScope::Document, "Document", "", plDocumentAction, plDocumentAction::ButtonType::CopyAssetGuid);
  s_hUpdatePrefabs = PL_REGISTER_ACTION_1("Prefabs.UpdateAll", plActionScope::Document, "Scene", "Ctrl+Shift+P", plDocumentAction, plDocumentAction::ButtonType::UpdatePrefabs);
}

void plDocumentActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hSaveCategory);
  plActionManager::UnregisterAction(s_hSave);
  plActionManager::UnregisterAction(s_hSaveAs);
  plActionManager::UnregisterAction(s_hSaveAll);
  plActionManager::UnregisterAction(s_hClose);
  plActionManager::UnregisterAction(s_hCloseAll);
  plActionManager::UnregisterAction(s_hCloseAllButThis);
  plActionManager::UnregisterAction(s_hOpenContainingFolder);
  plActionManager::UnregisterAction(s_hCopyAssetGuid);
  plActionManager::UnregisterAction(s_hUpdatePrefabs);
}

void plDocumentActions::MapMenuActions(plStringView sMapping, plStringView sTargetMenu)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSave, sTargetMenu, 5.0f);
  pMap->MapAction(s_hSaveAs, sTargetMenu, 6.0f);
  pMap->MapAction(s_hSaveAll, sTargetMenu, 7.0f);
  pMap->MapAction(s_hClose, sTargetMenu, 8.0f);
  pMap->MapAction(s_hCloseAll, sTargetMenu, 9.0f);
  pMap->MapAction(s_hCloseAllButThis, sTargetMenu, 10.0f);
  pMap->MapAction(s_hOpenContainingFolder, sTargetMenu, 11.0f);

  pMap->MapAction(s_hCopyAssetGuid, sTargetMenu, 12.0f);
}

void plDocumentActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hSaveCategory, "", 1.0f);
  plStringView sSubPath = "SaveCategory";

  pMap->MapAction(s_hSave, sSubPath, 1.0f);
  pMap->MapAction(s_hSaveAll, sSubPath, 3.0f);
}


void plDocumentActions::MapToolsActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hUpdatePrefabs, "G.Tools.Document", 1.0f);
}

////////////////////////////////////////////////////////////////////////
// plDocumentAction
////////////////////////////////////////////////////////////////////////

plDocumentAction::plDocumentAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case plDocumentAction::ButtonType::Save:
      SetIconPath(":/GuiFoundation/Icons/Save.svg");
      break;
    case plDocumentAction::ButtonType::SaveAs:
      SetIconPath("");
      break;
    case plDocumentAction::ButtonType::SaveAll:
      SetIconPath(":/GuiFoundation/Icons/SaveAll.svg");
      break;
    case plDocumentAction::ButtonType::Close:
      SetIconPath("");
      break;
    case plDocumentAction::ButtonType::CloseAll:
      SetIconPath("");
      break;
    case plDocumentAction::ButtonType::CloseAllButThis:
      SetIconPath("");
      break;
    case plDocumentAction::ButtonType::OpenContainingFolder:
      SetIconPath(":/GuiFoundation/Icons/OpenFolder.svg");
      break;
    case plDocumentAction::ButtonType::CopyAssetGuid:
      SetIconPath(":/GuiFoundation/Icons/Guid.svg");
      break;
    case plDocumentAction::ButtonType::UpdatePrefabs:
      SetIconPath(":/EditorPluginScene/Icons/PrefabUpdate.svg");
      break;
  }

  if (context.m_pDocument == nullptr)
  {
    if (button == ButtonType::Save || button == ButtonType::SaveAs)
    {
      // for actions that require a document, hide them
      SetVisible(false);
    }
  }
  else
  {
    m_Context.m_pDocument->m_EventsOne.AddEventHandler(plMakeDelegate(&plDocumentAction::DocumentEventHandler, this));

    if (m_ButtonType == ButtonType::Save)
    {
      SetVisible(!m_Context.m_pDocument->IsReadOnly());
      SetEnabled(m_Context.m_pDocument->IsModified());
    }
  }
}

plDocumentAction::~plDocumentAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->m_EventsOne.RemoveEventHandler(plMakeDelegate(&plDocumentAction::DocumentEventHandler, this));
  }
}

void plDocumentAction::DocumentEventHandler(const plDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case plDocumentEvent::Type::DocumentSaved:
    case plDocumentEvent::Type::ModifiedChanged:
    {
      if (m_ButtonType == ButtonType::Save)
      {
        SetEnabled(m_Context.m_pDocument->IsModified());
      }
    }
    break;

    default:
      break;
  }
}

void plDocumentAction::Execute(const plVariant& value)
{
  switch (m_ButtonType)
  {
    case plDocumentAction::ButtonType::Save:
    {
      plQtDocumentWindow* pWnd = plQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      pWnd->SaveDocument().LogFailure();
    }
    break;

    case plDocumentAction::ButtonType::SaveAs:
    {
      plQtDocumentWindow* pWnd = plQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);
      if (pWnd->SaveDocument().Succeeded())
      {
        auto* desc = m_Context.m_pDocument->GetDocumentTypeDescriptor();
        plStringBuilder sAllFilters;
        sAllFilters.Append(desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
        QString sSelectedExt;
        plString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"),
          plMakeQString(m_Context.m_pDocument->GetDocumentPath()), QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                           .toUtf8()
                           .data();

        if (!sFile.IsEmpty())
        {
          plUuid newDoc = plUuid::MakeUuid();
          plStatus res = m_Context.m_pDocument->GetDocumentManager()->CloneDocument(m_Context.m_pDocument->GetDocumentPath(), sFile, newDoc);

          if (res.Failed())
          {
            plStringBuilder s;
            s.SetFormat("Failed to save document: \n'{0}'", sFile);
            plQtUiServices::MessageBoxStatus(res, s);
          }
          else
          {
            const plDocumentTypeDescriptor* pTypeDesc = nullptr;
            if (plDocumentManager::FindDocumentTypeFromPath(sFile, false, pTypeDesc).Succeeded())
            {
              plDocument* pDocument = nullptr;
              m_Context.m_pDocument->GetDocumentManager()->OpenDocument(pTypeDesc->m_sDocumentTypeName, sFile, pDocument).LogFailure();
            }
          }
        }
      }
    }
    break;

    case plDocumentAction::ButtonType::SaveAll:
    {
      plToolsProject::GetSingleton()->BroadcastSaveAll();
    }
    break;

    case plDocumentAction::ButtonType::Close:
    {
      plQtDocumentWindow* pWindow = plQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      if (!pWindow->CanCloseWindow())
        return;

      pWindow->CloseDocumentWindow();
    }
    break;

    case plDocumentAction::ButtonType::CloseAll:
    {
      auto& documentWindows = plQtDocumentWindow::GetAllDocumentWindows();
      for (plQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow())
          continue;

        // Prevent closing the document root window.
        if (plStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
    }
    break;

    case plDocumentAction::ButtonType::CloseAllButThis:
    {
      plQtDocumentWindow* pThisWindow = plQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument);

      auto& documentWindows = plQtDocumentWindow::GetAllDocumentWindows();
      for (plQtDocumentWindow* pWindow : documentWindows)
      {
        if (!pWindow->CanCloseWindow() || pWindow == pThisWindow)
          continue;

        // Prevent closing the document root window.
        if (plStringUtils::Compare(pWindow->GetUniqueName(), "Settings") == 0)
          continue;

        pWindow->CloseDocumentWindow();
      }
    }
    break;

    case plDocumentAction::ButtonType::OpenContainingFolder:
    {
      plString sPath;

      if (!m_Context.m_pDocument)
      {
        if (plToolsProject::IsProjectOpen())
          sPath = plToolsProject::GetSingleton()->GetProjectFile();
        else
          sPath = plOSFile::GetApplicationDirectory();
      }
      else
        sPath = m_Context.m_pDocument->GetDocumentPath();

      plQtUiServices::OpenInExplorer(sPath, true);
    }
    break;

    case plDocumentAction::ButtonType::CopyAssetGuid:
    {
      plStringBuilder sGuid;
      plConversionUtils::ToString(m_Context.m_pDocument->GetGuid(), sGuid);

      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(sGuid.GetData());
      clipboard->setMimeData(mimeData);

      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Copied asset GUID: {}", sGuid), plTime::MakeFromSeconds(5));
    }
    break;

    case plDocumentAction::ButtonType::UpdatePrefabs:
      // TODO const cast
      const_cast<plDocument*>(m_Context.m_pDocument)->UpdatePrefabs();
      return;
  }
}
