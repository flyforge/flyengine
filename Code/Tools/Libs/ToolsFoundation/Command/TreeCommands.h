#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plAddObjectCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAddObjectCommand, plCommand);

public:
  plAddObjectCommand();

public: // Properties
  void SetType(plStringView sType);
  plStringView GetType() const;

  const plRTTI* m_pType = nullptr;
  plUuid m_Parent;
  plString m_sParentProperty;
  plVariant m_Index;
  plUuid m_NewObjectGuid; ///< This is optional. If not filled out, a new guid is assigned automatically.

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  plDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class PLASMA_TOOLSFOUNDATION_DLL plPasteObjectsCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPasteObjectsCommand, plCommand);

public:
  plPasteObjectsCommand();

public: // Properties
  plUuid m_Parent;
  plString m_sGraphTextFormat;
  plString m_sMimeType;
  bool m_bAllowPickedPosition = true;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    plDocumentObject* m_pObject;
    plDocumentObject* m_pParent;
    plString m_sParentProperty;
    plVariant m_Index;
  };

  plHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plInstantiatePrefabCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plInstantiatePrefabCommand, plCommand);

public:
  plInstantiatePrefabCommand();

public: // Properties
  plUuid m_Parent;
  plInt32 m_Index = -1;
  plUuid m_CreateFromPrefab;
  plUuid m_RemapGuid;
  plString m_sBasePrefabGraph;
  plString m_sObjectGraph;
  plUuid m_CreatedRootObject;
  bool m_bAllowPickedPosition;

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    plDocumentObject* m_pObject;
    plDocumentObject* m_pParent;
    plString m_sParentProperty;
    plVariant m_Index;
  };

  // at the moment this array always only holds a single item, the group node for the prefab
  plHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plUnlinkPrefabCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plUnlinkPrefabCommand, plCommand);

public:
  plUnlinkPrefabCommand() = default;

  plUuid m_Object;

private:
  virtual bool HasReturnValues() const override { return false; }
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plUuid m_OldCreateFromPrefab;
  plUuid m_OldRemapGuid;
  plString m_sOldGraphTextFormat;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plRemoveObjectCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRemoveObjectCommand, plCommand);

public:
  plRemoveObjectCommand();

public: // Properties
  plUuid m_Object;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  plDocumentObject* m_pParent = nullptr;
  plString m_sParentProperty;
  plVariant m_Index;
  plDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plMoveObjectCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMoveObjectCommand, plCommand);

public:
  plMoveObjectCommand();

public: // Properties
  plUuid m_Object;
  plUuid m_NewParent;
  plString m_sParentProperty;
  plVariant m_Index;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject;
  plDocumentObject* m_pOldParent;
  plDocumentObject* m_pNewParent;
  plString m_sOldParentProperty;
  plVariant m_OldIndex;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plSetObjectPropertyCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSetObjectPropertyCommand, plCommand);

public:
  plSetObjectPropertyCommand();

public: // Properties
  plUuid m_Object;
  plVariant m_NewValue;
  plVariant m_Index;
  plString m_sProperty;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject;
  plVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plResizeAndSetObjectPropertyCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plResizeAndSetObjectPropertyCommand, plCommand);

public:
  plResizeAndSetObjectPropertyCommand();

public: // Properties
  plUuid m_Object;
  plVariant m_NewValue;
  plVariant m_Index;
  plString m_sProperty;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override { return plStatus(PLASMA_SUCCESS); }
  virtual void CleanupInternal(CommandState state) override {}

  plDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plInsertObjectPropertyCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plInsertObjectPropertyCommand, plCommand);

public:
  plInsertObjectPropertyCommand();

public: // Properties
  plUuid m_Object;
  plVariant m_NewValue;
  plVariant m_Index;
  plString m_sProperty;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plRemoveObjectPropertyCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRemoveObjectPropertyCommand, plCommand);

public:
  plRemoveObjectPropertyCommand();

public: // Properties
  plUuid m_Object;
  plVariant m_Index;
  plString m_sProperty;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject;
  plVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_TOOLSFOUNDATION_DLL plMoveObjectPropertyCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMoveObjectPropertyCommand, plCommand);

public:
  plMoveObjectPropertyCommand();

public: // Properties
  plUuid m_Object;
  plVariant m_OldIndex;
  plVariant m_NewIndex;
  plString m_sProperty;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject;
};
