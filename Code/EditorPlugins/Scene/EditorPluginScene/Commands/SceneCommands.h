#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plRandomGauss;

class plDuplicateObjectsCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDuplicateObjectsCommand, plCommand);

public:
  plDuplicateObjectsCommand();

public: // Properties
  plString m_sGraphTextFormat;
  plString m_sParentNodes; /// A stringyfied map in format "uuidObj1=uuidParent1;..." that defines the previous parents of all top level objects

  plUInt32 m_uiNumberOfCopies; /// if set to 0 (the default), all the 'advanced' duplication code is skipped and only a single straight copy is made

  plVec3 m_vAccumulativeTranslation;
  plVec3 m_vAccumulativeRotation;
  plVec3 m_vRandomRotation;
  plVec3 m_vRandomTranslation;
  bool m_bGroupDuplicates;

  plInt8 m_iRevolveAxis; ///< 0 = disabled, 1 = x, 2 = y, 3 = z
  float m_fRevolveRadius;
  plAngle m_RevolveStartAngle;
  plAngle m_RevolveAngleStep;

private:
  virtual plStatus DoInternal(bool bRedo) override;

  void SetAsSelection();

  void DeserializeGraph(plAbstractObjectGraph& graph);

  void CreateOneDuplicate(plAbstractObjectGraph& graph, plHybridArray<plDocument::PasteInfo, 16>& ToBePasted);
  void AdjustObjectPositions(plHybridArray<plDocument::PasteInfo, 16>& Duplicates, plUInt32 uiNumDuplicate, plRandomGauss& rngRotX,
    plRandomGauss& rngRotY, plRandomGauss& rngRotZ, plRandomGauss& rngTransX, plRandomGauss& rngTransY, plRandomGauss& rngTransZ);

  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    plDocumentObject* m_pObject;
    plDocumentObject* m_pParent;
    plString m_sParentProperty;
    plVariant m_Index;
  };

  plDeque<const plDocumentObject*> m_OriginalSelection;
  plHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};
