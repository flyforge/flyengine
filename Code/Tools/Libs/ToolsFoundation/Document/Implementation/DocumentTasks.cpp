#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/DocumentTasks.h>

plSaveDocumentTask::plSaveDocumentTask()
{
  ConfigureTask("plSaveDocumentTask", plTaskNesting::Maybe);
}

plSaveDocumentTask::~plSaveDocumentTask() = default;

void plSaveDocumentTask::Execute()
{
  plAbstractGraphDdlSerializer::WriteDocument(file, &headerGraph, &objectGraph, &typesGraph, false);

  if (file.Close() == PLASMA_FAILURE)
  {
    m_document->m_LastSaveResult = plStatus(plFmt("Unable to open file '{0}' for writing!", m_document->m_sDocumentPath));
  }
  else
  {
    m_document->m_LastSaveResult = plStatus(PLASMA_SUCCESS);
  }
}

plAfterSaveDocumentTask::plAfterSaveDocumentTask()
{
  ConfigureTask("plAfterSaveDocumentTask", plTaskNesting::Maybe);
}

plAfterSaveDocumentTask::~plAfterSaveDocumentTask() = default;

void plAfterSaveDocumentTask::Execute()
{
  if (m_document->m_LastSaveResult.Succeeded())
  {
    plDocumentEvent e;
    e.m_pDocument = m_document;
    e.m_Type = plDocumentEvent::Type::DocumentSaved;
    m_document->m_EventsOne.Broadcast(e);
    m_document->s_EventsAny.Broadcast(e);

    m_document->SetModified(false);

    // after saving once, this information is pointless
    m_document->m_uiUnknownObjectTypeInstances = 0;
    m_document->m_UnknownObjectTypes.Clear();
  }

  if (m_document->m_LastSaveResult.Succeeded())
  {
    m_document->InternalAfterSaveDocument();
  }
  if (m_callback.IsValid())
  {
    m_callback(m_document, m_document->m_LastSaveResult);
  }
  m_document->m_ActiveSaveTask.Invalidate();
}
