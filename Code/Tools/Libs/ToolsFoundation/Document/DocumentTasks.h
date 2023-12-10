#pragma once

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>

class plSaveDocumentTask final : public plTask
{
public:
  plSaveDocumentTask();
  ~plSaveDocumentTask();

  plDeferredFileWriter file;
  plAbstractObjectGraph headerGraph;
  plAbstractObjectGraph objectGraph;
  plAbstractObjectGraph typesGraph;
  plDocument* m_document = nullptr;

  virtual void Execute() override;
};

class plAfterSaveDocumentTask final : public plTask
{
public:
  plAfterSaveDocumentTask();
  ~plAfterSaveDocumentTask();

  plDocument* m_document = nullptr;
  plDocument::AfterSaveCallback m_callback;

  virtual void Execute() override;
};
