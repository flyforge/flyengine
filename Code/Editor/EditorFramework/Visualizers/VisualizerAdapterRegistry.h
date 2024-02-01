#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct plVisualizerManagerEvent;
class plDocument;

class PL_EDITORFRAMEWORK_DLL plVisualizerAdapterRegistry
{
  PL_DECLARE_SINGLETON(plVisualizerAdapterRegistry);

public:
  plVisualizerAdapterRegistry();
  ~plVisualizerAdapterRegistry();

  plRttiMappedObjectFactory<plVisualizerAdapter> m_Factory;

private:
  void VisualizerManagerEventHandler(const plVisualizerManagerEvent& e);
  void ClearAdapters(const plDocument* pDocument);
  void CreateAdapters(const plDocument* pDocument, const plDocumentObject* pObject);

  struct Data
  {
    plHybridArray<plVisualizerAdapter*, 8> m_Adapters;
  };

  plMap<const plDocument*, Data> m_DocumentAdapters;
};
