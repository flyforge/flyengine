#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDocument;

struct PLASMA_GUIFOUNDATION_DLL plActionMapDescriptor
{
  plActionDescriptorHandle m_hAction; ///< Action to be mapped
  plString
    m_sPath; ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder; ///< Ordering key to sort actions in the mapping path.
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plActionMapDescriptor);

template <typename T>
class plTreeNode
{
public:
  plTreeNode()
    : m_pParent(nullptr)
  {
  }
  plTreeNode(const T& data)
    : m_Data(data)
    , m_pParent(nullptr)
  {
  }
  ~plTreeNode()
  {
    while (!m_Children.IsEmpty())
    {
      RemoveChild(0);
    }
  }

  const plUuid& GetGuid() const { return m_Guid; }
  const plTreeNode<T>* GetParent() const { return m_pParent; }
  plTreeNode<T>* GetParent() { return m_pParent; }
  const plHybridArray<plTreeNode<T>*, 8>& GetChildren() const { return m_Children; }
  plHybridArray<plTreeNode<T>*, 8>& GetChildren() { return m_Children; }

  plTreeNode<T>* InsertChild(const T& data, plUInt32 iIndex)
  {
    plTreeNode<T>* pNode = PLASMA_DEFAULT_NEW(plTreeNode<T>, data);
    pNode->m_Guid.CreateNewUuid();
    m_Children.Insert(pNode, iIndex);
    pNode->m_pParent = this;
    return pNode;
  }

  bool RemoveChild(plUInt32 iIndex)
  {
    if (iIndex > m_Children.GetCount())
      return false;

    plTreeNode<T>* pChild = m_Children[iIndex];
    m_Children.RemoveAtAndCopy(iIndex);
    PLASMA_DEFAULT_DELETE(pChild);
    return true;
  }

  plUInt32 GetParentIndex() const
  {
    PLASMA_ASSERT_DEV(m_pParent != nullptr, "Can't compute parent index if no parent is present!");
    for (plUInt32 i = 0; i < m_pParent->GetChildren().GetCount(); i++)
    {
      if (m_pParent->GetChildren()[i] == this)
        return i;
    }
    PLASMA_REPORT_FAILURE("Couldn't find oneself in own parent!");
    return -1;
  }

  T m_Data;
  plUuid m_Guid;

private:
  plTreeNode<T>* m_pParent;
  plHybridArray<plTreeNode<T>*, 8> m_Children;
};


class PLASMA_GUIFOUNDATION_DLL plActionMap
{
public:
  typedef plTreeNode<plActionMapDescriptor> TreeNode;
  plActionMap();
  ~plActionMap();

  void MapAction(plActionDescriptorHandle hAction, const char* szPath, float m_fOrder);
  plUuid MapAction(const plActionMapDescriptor& desc);
  plResult UnmapAction(plActionDescriptorHandle hAction, const char* szPath);
  plResult UnmapAction(const plActionMapDescriptor& desc);
  plResult UnmapAction(const plUuid& guid);

  const TreeNode* GetRootObject() const { return &m_Root; }
  const plActionMapDescriptor* GetDescriptor(const plUuid& guid) const;
  const plActionMapDescriptor* GetDescriptor(const plTreeNode<plActionMapDescriptor>* pObject) const;

private:
  bool FindObjectByPath(const plStringView& sPath, plUuid& out_guid) const;
  const plTreeNode<plActionMapDescriptor>* GetChildByName(const plTreeNode<plActionMapDescriptor>* pObject, const plStringView& sName) const;

private:
  TreeNode m_Root;
  plMap<plUuid, plTreeNode<plActionMapDescriptor>*> m_Descriptors;
};
