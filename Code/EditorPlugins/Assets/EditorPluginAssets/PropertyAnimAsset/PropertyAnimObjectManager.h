#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plPropertyAnimObjectManager : public plDocumentObjectManager
{
public:
  plPropertyAnimObjectManager();
  ~plPropertyAnimObjectManager();

  bool GetAllowStructureChangeOnTemporaries() const { return m_bAllowStructureChangeOnTemporaries; }
  void SetAllowStructureChangeOnTemporaries(bool bVal) { m_bAllowStructureChangeOnTemporaries = bVal; }

private:
  virtual plStatus InternalCanAdd(
    const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const override;
  virtual plStatus InternalCanRemove(const plDocumentObject* pObject) const override;
  virtual plStatus InternalCanMove(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, plStringView sParentProperty, const plVariant& index) const override;

private:
  bool m_bAllowStructureChangeOnTemporaries = false;
};
