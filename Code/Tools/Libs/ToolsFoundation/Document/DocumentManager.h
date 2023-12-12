#pragma once

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_TOOLSFOUNDATION_DLL plDocumentManager : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentManager, plReflectedClass);

public:
  virtual ~plDocumentManager() {}

  static const plHybridArray<plDocumentManager*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static plResult FindDocumentTypeFromPath(const char* szPath, bool bForCreation, const plDocumentTypeDescriptor*& out_pTypeDesc);

  plStatus CanOpenDocument(const char* szFilePath) const;

  /// \brief Creates a new document.
  /// \param szDocumentTypeName Document type to create. See plDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be created.
  /// \param out_pDocument Out parameter for the resulting plDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  plStatus CreateDocument(
    const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument, plBitflags<plDocumentFlags> flags = plDocumentFlags::None, const plDocumentObject* pOpenContext = nullptr);

  /// \brief Opens an existing document.
  /// \param szDocumentTypeName Document type to open. See plDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be opened.
  /// \param out_pDocument Out parameter for the resulting plDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext  An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  /// \return Returns the error in case the operations failed.
  plStatus OpenDocument(const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument,
    plBitflags<plDocumentFlags> flags = plDocumentFlags::AddToRecentFilesList | plDocumentFlags::RequestWindow,
    const plDocumentObject* pOpenContext = nullptr);
  virtual plStatus CloneDocument(const char* szPath, const char* szClonePath, plUuid& inout_cloneGuid);
  void CloseDocument(plDocument* pDocument);
  void EnsureWindowRequested(plDocument* pDocument, const plDocumentObject* pOpenContext = nullptr);

  /// \brief Returns a list of all currently open documents that are managed by this document manager
  const plDynamicArray<plDocument*>& GetAllOpenDocuments() const { return m_AllOpenDocuments; }

  plDocument* GetDocumentByPath(const char* szPath) const;

  static plDocument* GetDocumentByGuid(const plUuid& guid);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed.
  static bool EnsureDocumentIsClosedInAllManagers(const char* szPath);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed. This function only operates on documents opened by this manager. Use EnsureDocumentIsClosedInAllManagers() to
  /// close documents of any type.
  bool EnsureDocumentIsClosed(const char* szPath);

  void CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested, ///< Sent when the window for a document is needed. Each plugin should check this and see if it can create the desired
                               ///< window type
      AfterDocumentWindowRequested, ///< Sent after a document window was requested. Can be used to do things after the new window has been opened
      DocumentClosing,
      DocumentClosing2, // sent after DocumentClosing but before removing the document, use this to do stuff that depends on code executed during
                        // DocumentClosing
      DocumentClosed,   // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it
                        // is deleted
    };

    Type m_Type;
    plDocument* m_pDocument = nullptr;
    const plDocumentObject* m_pOpenContext = nullptr;
  };

  struct Request
  {
    enum class Type
    {
      DocumentAllowedToOpen,
    };

    Type m_Type;
    plString m_sDocumentType;
    plString m_sDocumentPath;
    plStatus m_RequestStatus;
  };

  static plCopyOnBroadcastEvent<const Event&> s_Events;
  static plEvent<Request&> s_Requests;

  static const plDocumentTypeDescriptor* GetDescriptorForDocumentType(const char* szDocumentType);
  static const plMap<plString, const plDocumentTypeDescriptor*>& GetAllDocumentDescriptors();

  void GetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const;

  using CustomAction = plVariant (*)(const plDocument*);
  static plMap<plString, CustomAction> s_CustomActions;

protected:
  virtual void InternalCloneDocument(const char* szPath, const char* szClonePath, const plUuid& documentId, const plUuid& seedGuid, const plUuid& cloneGuid, plAbstractObjectGraph* pHeader, plAbstractObjectGraph* pObjects, plAbstractObjectGraph* pTypes);

private:
  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) = 0;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const = 0;

private:
  plStatus CreateOrOpenDocument(bool bCreate, const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument,
    plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext = nullptr);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const plPluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const plPluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  plDynamicArray<plDocument*> m_AllOpenDocuments;

  static plSet<const plRTTI*> s_KnownManagers;
  static plHybridArray<plDocumentManager*, 16> s_AllDocumentManagers;

  static plMap<plString, const plDocumentTypeDescriptor*> s_AllDocumentDescriptors;
};
