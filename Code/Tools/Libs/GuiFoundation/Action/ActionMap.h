#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDocument;

struct PL_GUIFOUNDATION_DLL plActionMapDescriptor
{
  plActionDescriptorHandle m_hAction; ///< Action to be mapped
  plString m_sPath;                   ///< Path where the action should be mapped excluding the action's name, e.g. "File/New" for a menu item "File -> New -> Project..." .
  float m_fOrder;                     ///< Ordering key to sort actions in the mapping path.
};
PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plActionMapDescriptor);

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

  plTreeNode<T>* InsertChild(const T& data, plUInt32 uiIndex)
  {
    plTreeNode<T>* pNode = PL_DEFAULT_NEW(plTreeNode<T>, data);
    pNode->m_Guid = plUuid::MakeUuid();
    m_Children.Insert(pNode, uiIndex);
    pNode->m_pParent = this;
    return pNode;
  }

  bool RemoveChild(plUInt32 uiIndex)
  {
    if (uiIndex > m_Children.GetCount())
      return false;

    plTreeNode<T>* pChild = m_Children[uiIndex];
    m_Children.RemoveAtAndCopy(uiIndex);
    PL_DEFAULT_DELETE(pChild);
    return true;
  }

  plUInt32 GetParentIndex() const
  {
    PL_ASSERT_DEV(m_pParent != nullptr, "Can't compute parent index if no parent is present!");
    for (plUInt32 i = 0; i < m_pParent->GetChildren().GetCount(); i++)
    {
      if (m_pParent->GetChildren()[i] == this)
        return i;
    }
    PL_REPORT_FAILURE("Couldn't find oneself in own parent!");
    return -1;
  }

  T m_Data;
  plUuid m_Guid;

private:
  plTreeNode<T>* m_pParent;
  plHybridArray<plTreeNode<T>*, 8> m_Children;
};

/// \brief Defines the structure of how actions are organized in a particular context.
///
/// Actions are usually commands that are exposed through UI.
/// For instance a button in a toolbar or a menu entry.
///
/// Actions are unique. Each action only exists once in plActionManager.
///
/// An action map defines where in a menu an action shows up.
/// Actions are usually grouped by categories. So for example all actions related to opening, closing
/// or saving a document may be in one group. Their position within that group is defined through
/// an 'order' value. This allows plugins to insert actions easily.
///
/// A window might use multiple action maps to build different structures.
/// For example, usually there is one action map for a window menu, and another map for a toolbar.
/// These will contain different actions, and they are organized differently.
///
/// Action maps are created through plActionMapManager and are simply identified by name.
class PL_GUIFOUNDATION_DLL plActionMap
{
public:
  using TreeNode = plTreeNode<plActionMapDescriptor>;
  plActionMap();
  ~plActionMap();

  /// \brief Adds the given action to into the category or menu identified by sPath.
  ///
  /// All actions added to the same path will be sorted by 'fOrder' and the ones with the smaller values show up at the top.
  ///
  /// sPath must either be a fully qualified path OR the name of a uniquely named category or menu.
  /// If sPath is empty, the action (which may be a category itself) will be mapped into the root.
  /// This is common for top-level menus and for toolbars.
  ///
  /// If sPath is a fully qualified path, the segments are separated by slashes (/)
  /// and each segment must name either a category (see PL_REGISTER_CATEGORY) or a menu (see PL_REGISTER_MENU).
  ///
  /// sPath may also name a category or menu WITHOUT it being a full path. In this case the name must be unique.
  /// If sPath isn't empty and doesn't contain a slash, the system searches all available actions that are already in the action map.
  /// This allows you to insert an action into a category, without knowing the full path to that category.
  /// By convention, categories that are meant to be used that way are named "G.Something". The idea is, that where that category
  /// really shows up (and whether it is its own menu or just an area somewhere) may change in the future, or may be different
  /// in different contexts.
  ///
  /// To make it easier to use 'global' category names combined with an additional relative path, there is an overload of this function
  /// that takes an additional sSubPath argument.
  void MapAction(plActionDescriptorHandle hAction, plStringView sPath, float fOrder);

  /// \brief An overload of MapAction that takes a dedicated sPath and sSubPath argument for convenience.
  ///
  /// If sPath is a 'global' name of a category, it is searched for (see SearchPathForAction()).
  /// Afterwards sSubPath is appended and the result is forwarded to MapAction() as a single path string.
  void MapAction(plActionDescriptorHandle hAction, plStringView sPath, plStringView sSubPath, float fOrder);

  /// \brief Removes the named action from the action map. The same rules for 'global' names apply as for MapAction().
  plResult UnmapAction(plActionDescriptorHandle hAction, plStringView sPath);

  /// \brief Searches for an action with the given name and returns the full path to it.
  ///
  /// This is mainly meant to be used with (unique) names to categories (or menus).
  plResult SearchPathForAction(plStringView sUniqueName, plStringBuilder& out_sPath) const;

  const TreeNode* GetRootObject() const { return &m_Root; }

  const plActionMapDescriptor* GetDescriptor(const plTreeNode<plActionMapDescriptor>* pObject) const;

private:
  plUuid MapAction(const plActionMapDescriptor& desc);
  plResult UnmapAction(const plActionMapDescriptor& desc);
  plResult UnmapAction(const plUuid& guid);

  const plActionMapDescriptor* GetDescriptor(const plUuid& guid) const;

  bool FindObjectByPath(plStringView sPath, plUuid& out_guid) const;
  bool FindObjectPathByName(const plTreeNode<plActionMapDescriptor>* pObject, plStringView sName, plStringBuilder& out_sPath) const;
  const plTreeNode<plActionMapDescriptor>* GetChildByName(const plTreeNode<plActionMapDescriptor>* pObject, plStringView sName) const;

  TreeNode m_Root;
  plMap<plUuid, plTreeNode<plActionMapDescriptor>*> m_Descriptors;
};
