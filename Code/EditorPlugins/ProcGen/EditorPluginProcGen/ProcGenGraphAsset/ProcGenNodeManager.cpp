#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodeManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <Foundation/Configuration/Startup.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenPin, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginProcGen, ProcGen)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    const plRTTI* pBaseType = plGetStaticRTTI<plProcGenNodeBase>();

    plQtNodeScene::GetPinFactory().RegisterCreator(plGetStaticRTTI<plProcGenPin>(), [](const plRTTI* pRtti)->plQtPin* { return new plQtProcGenPin(); });
    plQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const plRTTI* pRtti)->plQtNode* { return new plQtProcGenNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const plRTTI* pBaseType = plGetStaticRTTI<plProcGenNodeBase>();

    plQtNodeScene::GetPinFactory().UnregisterCreator(plGetStaticRTTI<plProcGenPin>());
    plQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

bool plProcGenNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(plGetStaticRTTI<plProcGenNodeBase>());
}

void plProcGenNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node)
{
  const plRTTI* pNodeBaseType = plGetStaticRTTI<plProcGenNodeBase>();

  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom(pNodeBaseType))
    return;

  plHybridArray<const plAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    const plRTTI* pPropType = pProp->GetSpecificType();
    if (!pPropType->IsDerivedFrom<plRenderPipelineNodePin>())
      continue;

    plColor pinColor = plColorScheme::DarkUI(plColorScheme::Gray);
    if (const plColorAttribute* pAttr = pProp->GetAttributeByType<plColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pPropType->IsDerivedFrom<plRenderPipelineNodeInputPin>())
    {
      auto pPin = PL_DEFAULT_NEW(plProcGenPin, plPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pPropType->IsDerivedFrom<plRenderPipelineNodeOutputPin>())
    {
      auto pPin = PL_DEFAULT_NEW(plProcGenPin, plPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      ref_node.m_Outputs.PushBack(pPin);
    }
  }
}

void plProcGenNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const
{
  plRTTI::ForEachDerivedType<plProcGenNodeBase>(
    [&](const plRTTI* pRtti) { ref_types.PushBack(pRtti); },
    plRTTI::ForEachOptions::ExcludeAbstract);
}

plStatus plProcGenNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNto1;
  return plStatus(PL_SUCCESS);
}
