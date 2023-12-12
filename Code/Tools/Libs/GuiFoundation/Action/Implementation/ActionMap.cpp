#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMap.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plActionMapDescriptor, plNoBase, 0, plRTTINoAllocator);
//  PLASMA_BEGIN_PROPERTIES
//  PLASMA_END_PROPERTIES;
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plActionMap public functions
////////////////////////////////////////////////////////////////////////

plActionMap::plActionMap()
{
  // plReflectedTypeDescriptor desc;
  // plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(plGetStaticRTTI<plActionMapDescriptor>(), desc);
  // m_pRtti = plPhantomRttiManager::RegisterType(desc);
}

plActionMap::~plActionMap()
{
  // DestroyAllObjects();
}

void plActionMap::MapAction(plActionDescriptorHandle hAction, const char* szPath, float fOrder)
{
  plStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.Trim("/");
  plActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sPath;
  d.m_fOrder = fOrder;

  // Ignore for now as this will be resolved when new menu setup in finished
  //PLASMA_VERIFY(MapAction(d).IsValid(), "Mapping Failed");
  MapAction(d);
}

plUuid plActionMap::MapAction(const plActionMapDescriptor& desc)
{
  plUuid ParentGUID;
  if (!FindObjectByPath(desc.m_sPath, ParentGUID))
  {
    return plUuid();
  }

  auto it = m_Descriptors.Find(ParentGUID);

  plTreeNode<plActionMapDescriptor>* pParent = nullptr;
  if (it.IsValid())
  {
    pParent = it.Value();
  }

  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    const plActionMapDescriptor* pDesc = GetDescriptor(pParent);
    if (pDesc->m_hAction.GetDescriptor()->m_Type == plActionType::Action)
    {
      plLog::Error("Can't map descriptor '{0}' as its parent is an action itself and thus can't have any children.",
        desc.m_hAction.GetDescriptor()->m_sActionName);
      return plUuid();
    }
  }

  if (GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName) != nullptr)
  {
    plLog::Error("Can't map descriptor as its name is already present: {0}", desc.m_hAction.GetDescriptor()->m_sActionName);
    return plUuid();
  }

  plInt32 iIndex = 0;
  for (iIndex = 0; iIndex < (plInt32)pParent->GetChildren().GetCount(); ++iIndex)
  {
    const plTreeNode<plActionMapDescriptor>* pChild = pParent->GetChildren()[iIndex];
    const plActionMapDescriptor* pDesc = GetDescriptor(pChild);

    if (desc.m_fOrder < pDesc->m_fOrder)
      break;
  }

  plTreeNode<plActionMapDescriptor>* pChild = pParent->InsertChild(desc, iIndex);

  m_Descriptors.Insert(pChild->GetGuid(), pChild);

  return pChild->GetGuid();
}

plResult plActionMap::UnmapAction(const plUuid& guid)
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return PLASMA_FAILURE;

  plTreeNode<plActionMapDescriptor>* pNode = it.Value();
  if (plTreeNode<plActionMapDescriptor>* pParent = pNode->GetParent())
  {
    pParent->RemoveChild(pNode->GetParentIndex());
  }
  m_Descriptors.Remove(it);
  return PLASMA_SUCCESS;
}

plResult plActionMap::UnmapAction(plActionDescriptorHandle hAction, const char* szPath)
{
  plStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  sPath.Trim("/");
  plActionMapDescriptor d;
  d.m_hAction = hAction;
  d.m_sPath = sPath;
  d.m_fOrder = 0.0f; // unused.
  return UnmapAction(d);
}

plResult plActionMap::UnmapAction(const plActionMapDescriptor& desc)
{
  plTreeNode<plActionMapDescriptor>* pParent = nullptr;
  if (desc.m_sPath.IsEmpty())
  {
    pParent = &m_Root;
  }
  else
  {
    plUuid ParentGUID;
    if (!FindObjectByPath(desc.m_sPath, ParentGUID))
      return PLASMA_FAILURE;

    auto it = m_Descriptors.Find(ParentGUID);
    if (!it.IsValid())
      return PLASMA_FAILURE;

    pParent = it.Value();
  }

  if (auto* pChild = GetChildByName(pParent, desc.m_hAction.GetDescriptor()->m_sActionName))
  {
    return UnmapAction(pChild->GetGuid());
  }
  return PLASMA_FAILURE;
}

bool plActionMap::FindObjectByPath(const plStringView& sPath, plUuid& out_guid) const
{
  out_guid = plUuid();
  if (sPath.IsEmpty())
    return true;

  plStringBuilder sPathBuilder(sPath);
  plHybridArray<plStringView, 8> parts;
  sPathBuilder.Split(false, parts, "/");

  const plTreeNode<plActionMapDescriptor>* pParent = &m_Root;
  for (const plStringView& name : parts)
  {
    pParent = GetChildByName(pParent, name);
    if (pParent == nullptr)
      return false;
  }

  out_guid = pParent->GetGuid();
  return true;
}

const plActionMapDescriptor* plActionMap::GetDescriptor(const plUuid& guid) const
{
  auto it = m_Descriptors.Find(guid);
  if (!it.IsValid())
    return nullptr;
  return GetDescriptor(it.Value());
}

const plActionMapDescriptor* plActionMap::GetDescriptor(const plTreeNode<plActionMapDescriptor>* pObject) const
{
  if (pObject == nullptr)
    return nullptr;

  return &pObject->m_Data;
}

const plTreeNode<plActionMapDescriptor>* plActionMap::GetChildByName(
  const plTreeNode<plActionMapDescriptor>* pObject, const plStringView& sName) const
{
  for (const plTreeNode<plActionMapDescriptor>* pChild : pObject->GetChildren())
  {
    const plActionMapDescriptor& pDesc = pChild->m_Data;
    if (sName.IsEqual_NoCase(pDesc.m_hAction.GetDescriptor()->m_sActionName.GetData()))
    {
      return pChild;
    }
  }
  return nullptr;
}
