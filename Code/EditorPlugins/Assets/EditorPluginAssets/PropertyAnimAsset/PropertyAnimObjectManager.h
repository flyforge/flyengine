#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plPropertyAnimObjectManager : public plDocumentObjectManager
{
public:
  plPropertyAnimObjectManager();
  ~plPropertyAnimObjectManager();

  bool GetAllowStructureChangeOnTemporaries() const { return m_bAllowStructureChangeOnTemporaries; }
  void SetAllowStructureChangeOnTemporaries(bool val) { m_bAllowStructureChangeOnTemporaries = val; }

private:
  virtual plStatus InternalCanAdd(
    const plRTTI* pRtti, const plDocumentObject* pParent, const char* szParentProperty, const plVariant& index) const override;
  virtual plStatus InternalCanRemove(const plDocumentObject* pObject) const override;
  virtual plStatus InternalCanMove(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, const char* szParentProperty, const plVariant& index) const override;

private:
  bool m_bAllowStructureChangeOnTemporaries = false;
};
