#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

struct plManipulatorManagerEvent;
class plDocument;

class PL_EDITORFRAMEWORK_DLL plManipulatorAdapterRegistry
{
  PL_DECLARE_SINGLETON(plManipulatorAdapterRegistry);

public:
  plManipulatorAdapterRegistry();
  ~plManipulatorAdapterRegistry();

  plRttiMappedObjectFactory<plManipulatorAdapter> m_Factory;

  void QueryGridSettings(const plDocument* pDocument, plGridSettingsMsgToEngine& out_gridSettings);

private:
  void ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e);
  void ClearAdapters(const plDocument* pDocument);

  struct Data
  {
    plHybridArray<plManipulatorAdapter*, 8> m_Adapters;
  };

  plMap<const plDocument*, Data> m_DocumentAdapters;
};
