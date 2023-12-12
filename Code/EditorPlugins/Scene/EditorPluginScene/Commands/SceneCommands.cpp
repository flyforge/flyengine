#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDuplicateObjectsCommand, 1, plRTTIDefaultAllocator<plDuplicateObjectsCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GraphText", m_sGraphTextFormat),
    PLASMA_MEMBER_PROPERTY("ParentNodes", m_sParentNodes),
    PLASMA_MEMBER_PROPERTY("NumCopies", m_uiNumberOfCopies),
    PLASMA_MEMBER_PROPERTY("Translate", m_vAccumulativeTranslation),
    PLASMA_MEMBER_PROPERTY("Rotate", m_vAccumulativeRotation),
    PLASMA_MEMBER_PROPERTY("RandomRotation", m_vRandomRotation),
    PLASMA_MEMBER_PROPERTY("RandomTranslation", m_vRandomTranslation),
    PLASMA_MEMBER_PROPERTY("Group", m_bGroupDuplicates),
    PLASMA_MEMBER_PROPERTY("RevolveAxis", m_iRevolveAxis),
    PLASMA_MEMBER_PROPERTY("RevoleStartAngle", m_RevolveStartAngle),
    PLASMA_MEMBER_PROPERTY("RevolveAngleStep", m_RevolveAngleStep),
    PLASMA_MEMBER_PROPERTY("RevolveRadius", m_fRevolveRadius),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


////////////////////////////////////////////////////////////////////////
// plDuplicateObjectsCommand
////////////////////////////////////////////////////////////////////////

plDuplicateObjectsCommand::plDuplicateObjectsCommand()
{
  m_uiNumberOfCopies = 0;
  m_vAccumulativeTranslation.SetZero();
  m_vAccumulativeRotation.SetZero();
  m_vRandomRotation.SetZero();
  m_vRandomTranslation.SetZero();
  m_iRevolveAxis = 0;
  m_fRevolveRadius = 0.0f;
  m_bGroupDuplicates = false;
}

plStatus plDuplicateObjectsCommand::DoInternal(bool bRedo)
{
  plSceneDocument* pDocument = static_cast<plSceneDocument*>(GetDocument());

  if (!bRedo)
  {
    PLASMA_ASSERT_DEV(!m_bGroupDuplicates, "Not yet implemented");

    plAbstractObjectGraph graph;
    DeserializeGraph(graph);

    if (m_uiNumberOfCopies == 0)
    {
      plHybridArray<plDocument::PasteInfo, 16> ToBePasted;
      CreateOneDuplicate(graph, ToBePasted);
    }
    else
    {
      // store original selection
      m_OriginalSelection = m_pDocument->GetSelectionManager()->GetSelection();

      plHybridArray<plHybridArray<plDocument::PasteInfo, 16>, 8> ToBePasted;
      ToBePasted.SetCount(m_uiNumberOfCopies);

      for (plUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        CreateOneDuplicate(graph, ToBePasted[copies]);
      }

      plRandomGauss rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ;

      if (m_vRandomRotation.x > 0)
        rngRotX.Initialize((plUInt64)plTime::Now().GetNanoseconds(), (plUInt32)(m_vRandomRotation.x));
      if (m_vRandomRotation.y > 0)
        rngRotY.Initialize((plUInt64)plTime::Now().GetNanoseconds() + 1, (plUInt32)(m_vRandomRotation.y));
      if (m_vRandomRotation.z > 0)
        rngRotZ.Initialize((plUInt64)plTime::Now().GetNanoseconds() + 2, (plUInt32)(m_vRandomRotation.z));

      if (m_vRandomTranslation.x > 0)
        rngTransX.Initialize((plUInt64)plTime::Now().GetNanoseconds() + 3, (plUInt32)(m_vRandomTranslation.x * 100));
      if (m_vRandomTranslation.y > 0)
        rngTransY.Initialize((plUInt64)plTime::Now().GetNanoseconds() + 4, (plUInt32)(m_vRandomTranslation.y * 100));
      if (m_vRandomTranslation.z > 0)
        rngTransZ.Initialize((plUInt64)plTime::Now().GetNanoseconds() + 5, (plUInt32)(m_vRandomTranslation.z * 100));

      for (plUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        AdjustObjectPositions(ToBePasted[copies], copies, rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ);
      }
    }


    if (m_DuplicatedObjects.IsEmpty())
      return plStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_DuplicatedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  SetAsSelection();

  return plStatus(PLASMA_SUCCESS);
}

void plDuplicateObjectsCommand::SetAsSelection()
{
  if (!m_DuplicatedObjects.IsEmpty())
  {
    auto pSelMan = m_pDocument->GetSelectionManager();

    plDeque<const plDocumentObject*> NewSelection = m_OriginalSelection;

    for (const DuplicatedObject& pi : m_DuplicatedObjects)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }
}

void plDuplicateObjectsCommand::DeserializeGraph(plAbstractObjectGraph& graph)
{
  plRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
  plAbstractGraphDdlSerializer::Read(memoryReader, &graph).IgnoreResult();
}

void plDuplicateObjectsCommand::CreateOneDuplicate(plAbstractObjectGraph& graph, plHybridArray<plDocument::PasteInfo, 16>& ToBePasted)
{
  plSceneDocument* pDocument = static_cast<plSceneDocument*>(GetDocument());

  // Remap
  plUuid seed;
  seed.CreateNewUuid();
  graph.ReMapNodeGuids(seed);

  plDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateOnly);


  plStringBuilder sParentGuids = m_sParentNodes;
  plStringBuilder sNextParentGuid;

  plMap<plUuid, plUuid> ParentGuids;

  while (!sParentGuids.IsEmpty())
  {
    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    plUuid guidObj = plConversionUtils::ConvertStringToUuid(sNextParentGuid);
    guidObj.CombineWithSeed(seed);

    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    ParentGuids[guidObj] = plConversionUtils::ConvertStringToUuid(sNextParentGuid);
  }

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    if (pNode->GetNodeName() == "root")
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = nullptr;

        const plUuid guidParent = ParentGuids[pNode->GetGuid()];

        if (guidParent.IsValid())
          ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
      }
    }
  }

  if (pDocument->DuplicateSelectedObjects(ToBePasted, graph, false))
  {
    for (const auto& item : ToBePasted)
    {
      auto& po = m_DuplicatedObjects.ExpandAndGetRef();
      po.m_pObject = item.m_pObject;
      po.m_Index = item.m_pObject->GetPropertyIndex();
      po.m_pParent = item.m_pParent;
      po.m_sParentProperty = item.m_pObject->GetParentProperty();
    }
  }
  else
  {
    for (const auto& item : ToBePasted)
    {
      pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
    }
  }

  // undo uuid changes, so that we can do this again with another seed
  graph.ReMapNodeGuids(seed, true);
}


void plDuplicateObjectsCommand::AdjustObjectPositions(plHybridArray<plDocument::PasteInfo, 16>& Duplicates, plUInt32 uiNumDuplicate, plRandomGauss& rngRotX, plRandomGauss& rngRotY, plRandomGauss& rngRotZ, plRandomGauss& rngTransX, plRandomGauss& rngTransY, plRandomGauss& rngTransZ)
{
  plSceneDocument* pScene = static_cast<plSceneDocument*>(m_pDocument);

  const float fStep = uiNumDuplicate;

  plVec3 vRandT(0.0f);
  plVec3 vRandR(0.0f);

  if (m_vRandomRotation.x != 0)
    vRandR.x = rngRotX.SignedValue();
  if (m_vRandomRotation.y != 0)
    vRandR.y = rngRotY.SignedValue();
  if (m_vRandomRotation.z != 0)
    vRandR.z = rngRotZ.SignedValue();

  if (m_vRandomTranslation.x != 0)
    vRandT.x = rngTransX.SignedValue() / 100.0f;
  if (m_vRandomTranslation.y != 0)
    vRandT.y = rngTransY.SignedValue() / 100.0f;
  if (m_vRandomTranslation.z != 0)
    vRandT.z = rngTransZ.SignedValue() / 100.0f;

  plVec3 vPosOffset(0.0f);

  if (m_iRevolveAxis > 0 && m_fRevolveRadius != 0.0f && m_RevolveAngleStep != plAngle())
  {
    plVec3 vRevolveAxis(0.0f);
    plAngle revolve = m_RevolveStartAngle;

    switch (m_iRevolveAxis)
    {
      case 1:
        vRevolveAxis.Set(1, 0, 0);
        vPosOffset.Set(0, 0, m_fRevolveRadius);
        break;
      case 2:
        vRevolveAxis.Set(0, 1, 0);
        vPosOffset.Set(m_fRevolveRadius, 0, 0);
        break;
      case 3:
        vRevolveAxis.Set(0, 0, 1);
        vPosOffset.Set(0, m_fRevolveRadius, 0);
        break;
    }

    revolve += fStep * m_RevolveAngleStep;

    plMat3 mRevolve;
    mRevolve.SetRotationMatrix(vRevolveAxis, revolve);

    vPosOffset = mRevolve * vPosOffset;
  }

  plQuat qRot;
  qRot.SetFromEulerAngles(plAngle::Degree(fStep * m_vAccumulativeRotation.x + vRandR.x), plAngle::Degree(fStep * m_vAccumulativeRotation.y + vRandR.y), plAngle::Degree(fStep * m_vAccumulativeRotation.z + vRandR.z));

  for (const auto& pi : Duplicates)
  {
    plTransform tGlobal = pScene->GetGlobalTransform(pi.m_pObject);

    tGlobal.m_vScale.Set(1.0f);
    tGlobal.m_vPosition += vPosOffset + (1.0f + fStep) * m_vAccumulativeTranslation + vRandT;
    tGlobal.m_qRotation = qRot * tGlobal.m_qRotation;

    /// \todo Christopher: Modifying the position through a command after creating the object seems to destroy the undo-ability of this
    /// operation Duplicating multiple objects (with some translation) and then undoing that will crash the editor process

    pScene->SetGlobalTransform(pi.m_pObject, tGlobal, TransformationChanges::Translation | TransformationChanges::Rotation);
  }
}

plStatus plDuplicateObjectsCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  plDocument* pDocument = GetDocument();

  for (auto& po : m_DuplicatedObjects)
  {
    PLASMA_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return plStatus(PLASMA_SUCCESS);
}

void plDuplicateObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_DuplicatedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_DuplicatedObjects.Clear();
  }
}
