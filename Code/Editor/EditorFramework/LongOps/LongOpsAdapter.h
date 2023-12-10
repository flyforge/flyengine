#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct plDocumentObjectStructureEvent;
struct plPhantomRttiManagerEvent;
class plRTTI;

/// \brief This singleton lives in the editor process and monitors all plSceneDocument's for components with the plLongOpAttribute.
///
/// All such components will be automatically registered in the plLongOpControllerManager, such that their functionality
/// is exposed to the user.
///
/// Since this class adapts the components with the plLongOpAttribute to the plLongOpControllerManager, it does not have any public
/// functionality.
class plLongOpsAdapter
{
  PLASMA_DECLARE_SINGLETON(plLongOpsAdapter);

public:
  plLongOpsAdapter();
  ~plLongOpsAdapter();

private:
  void DocumentManagerEventHandler(const plDocumentManager::Event& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e);
  void CheckAllTypes();
  void ObjectAdded(const plDocumentObject* pObject);
  void ObjectRemoved(const plDocumentObject* pObject);

  plHashSet<const plRTTI*> m_TypesWithLongOps;
};
