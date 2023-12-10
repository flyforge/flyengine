#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plToolsProject;
class plDocument;

struct plToolsProjectEvent
{
  enum class Type
  {
    ProjectCreated,
    ProjectOpened,
    ProjectSaveState,
    ProjectClosing,
    ProjectClosed,
    ProjectConfigChanged, ///< Sent when global project configuration data was changed and thus certain menus would need to update their content (or
                          ///< just deselect any item, forcing the user to reselect and thus update state)
    SaveAll,              ///< When sent, this shall save all outstanding modifications
  };

  plToolsProject* m_pProject;
  Type m_Type;
};

struct plToolsProjectRequest
{
  plToolsProjectRequest();

  enum class Type
  {
    CanCloseProject,        ///< Can we close the project? Listener needs to set m_bCanClose if not.
    CanCloseDocuments,      ///< Can we close the documents in m_Documents? Listener needs to set m_bCanClose if not.
    SuggestContainerWindow, ///< m_Documents contains one element that a container window should be suggested for and written to
                            ///< m_iContainerWindowUniqueIdentifier.
    GetPathForDocumentGuid,
  };

  Type m_Type;
  bool m_bCanClose;                        ///< When the event is sent, interested code can set this to false to prevent closing.
  plDynamicArray<plDocument*> m_Documents; ///< In case of 'CanCloseDocuments', these will be the documents in question.
  plInt32
    m_iContainerWindowUniqueIdentifier; ///< In case of 'SuggestContainerWindow', the ID of the container to be used for the docs in m_Documents.

  plUuid m_documentGuid;
  plStringBuilder m_sAbsDocumentPath;
};

class PLASMA_TOOLSFOUNDATION_DLL plToolsProject
{
  PLASMA_DECLARE_SINGLETON(plToolsProject);

public:
  static plEvent<const plToolsProjectEvent&> s_Events;
  static plEvent<plToolsProjectRequest&> s_Requests;

public:
  static bool IsProjectOpen() { return GetSingleton() != nullptr; }
  static bool IsProjectClosing() { return (GetSingleton() != nullptr && GetSingleton()->m_bIsClosing); }
  static void CloseProject();
  static void SaveProjectState();
  /// \brief Returns true when the project can be closed. Uses plToolsProjectRequest::Type::CanCloseProject event.
  static bool CanCloseProject();
  /// \brief Returns true when the given list of documents can be closed. Uses plToolsProjectRequest::Type::CanCloseDocuments event.
  static bool CanCloseDocuments(plArrayPtr<plDocument*> documents);
  /// \brief Returns the unique ID of the container window this document should use for its window. Uses
  /// plToolsProjectRequest::Type::SuggestContainerWindow event.
  static plInt32 SuggestContainerWindow(plDocument* pDoc);
  /// \brief Resolve document GUID into an absolute path.
  plStringBuilder GetPathForDocumentGuid(const plUuid& guid);
  static plStatus OpenProject(plStringView sProjectPath);
  static plStatus CreateProject(plStringView sProjectPath);

  /// \brief Broadcasts the SaveAll event, though otherwise has no direct effect.
  static void BroadcastSaveAll();

  /// \brief Sent when global project configuration data was changed and thus certain menus would need to update their content (or just deselect any
  /// item, forcing the user to reselect and thus update state)
  static void BroadcastConfigChanged();

  /// \brief Returns the path to the 'plProject' file
  const plString& GetProjectFile() const { return m_sProjectPath; }

  /// \brief Returns the short name of the project (extracted from the path).
  ///
  /// \param bSanitize Whether to replace whitespace and other problematic characters, such that it can be used in code.
  const plString GetProjectName(bool bSanitize) const;

  /// \brief Returns the path in which the 'plProject' file is stored
  plString GetProjectDirectory() const;

  /// \brief Returns the directory path in which project settings etc. should be stored
  plString GetProjectDataFolder() const;

  /// \brief Starts at the  given document and then searches the tree upwards until it finds an plProject file.
  static plString FindProjectDirectoryForDocument(plStringView sDocumentPath);

  bool IsDocumentInAllowedRoot(plStringView sDocumentPath, plString* out_pRelativePath = nullptr) const;

  void AddAllowedDocumentRoot(plStringView sPath);

  /// \brief Makes sure the given sub-folder exists inside the project directory
  void CreateSubFolder(plStringView sFolder) const;

private:
  static plStatus CreateOrOpenProject(plStringView sProjectPath, bool bCreate);

private:
  plToolsProject(plStringView sProjectPath);
  ~plToolsProject();

  plStatus Create();
  plStatus Open();

private:
  bool m_bIsClosing;
  plString m_sProjectPath;
  plHybridArray<plString, 4> m_AllowedDocumentRoots;
};
