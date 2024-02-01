#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct PL_TOOLSFOUNDATION_DLL plActiveDocumentChange
{
  const plDocument* m_pOldDocument;
  const plDocument* m_pNewDocument;
};

/// \brief Tracks existing and active plDocument.
///
/// While the IDocumentManager manages documents of a certain context,
/// this class simply keeps track of the overall number of documents and the currently active one.
class PL_TOOLSFOUNDATION_DLL plDocumentRegistry
{
public:
  static bool RegisterDocument(const plDocument* pDocument);
  static bool UnregisterDocument(const plDocument* pDocument);

  static plArrayPtr<const plDocument*> GetDocuments() { return s_Documents; }

  static void SetActiveDocument(const plDocument* pDocument);
  static const plDocument* GetActiveDocument();

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, DocumentRegistry);

  static void Startup();
  static void Shutdown();

public:
  // static plEvent<plDocumentChange&> m_DocumentAddedEvent;
  // static plEvent<plDocumentChange&> m_DocumentRemovedEvent;
  static plEvent<plActiveDocumentChange&> m_ActiveDocumentChanged;

private:
  static plHybridArray<const plDocument*, 16> s_Documents;
  static plDocument* s_pActiveDocument;
};
