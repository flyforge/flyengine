#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPropertyAnimationTrack, 1, plRTTIDefaultAllocator<plPropertyAnimationTrack>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectPath", m_sObjectSearchSequence),
    PLASMA_MEMBER_PROPERTY("ComponentType", m_sComponentType),
    PLASMA_MEMBER_PROPERTY("Property", m_sPropertyPath),
    PLASMA_ENUM_MEMBER_PROPERTY("Target", plPropertyAnimTarget, m_Target),
    PLASMA_MEMBER_PROPERTY("FloatCurve", m_FloatCurve),
    PLASMA_MEMBER_PROPERTY("Gradient", m_ColorGradient),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPropertyAnimationTrackGroup, 1, plRTTIDefaultAllocator<plPropertyAnimationTrackGroup>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new plDefaultValueAttribute(60)),
    PLASMA_MEMBER_PROPERTY("Duration", m_uiCurveDuration)->AddAttributes(new plDefaultValueAttribute(480)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Tracks", m_Tracks)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_MEMBER_PROPERTY("EventTrack", m_EventTrack),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPropertyAnimAssetDocument, 2, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPropertyAnimationTrackGroup::~plPropertyAnimationTrackGroup()
{
  for (plPropertyAnimationTrack* pTrack : m_Tracks)
  {
    PLASMA_DEFAULT_DELETE(pTrack);
  }
}

plPropertyAnimAssetDocument::plPropertyAnimAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plPropertyAnimationTrackGroup, plGameObjectContextDocument>(
      PLASMA_DEFAULT_NEW(plPropertyAnimObjectManager), sDocumentPath, plAssetDocEngineConnection::FullObjectMirroring)
{
  m_GameObjectContextEvents.AddEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::GameObjectContextEventHandler, this));
  m_pObjectAccessor = PLASMA_DEFAULT_NEW(plPropertyAnimObjectAccessor, this, GetCommandHistory());
}

plPropertyAnimAssetDocument::~plPropertyAnimAssetDocument()
{
  m_GameObjectContextEvents.RemoveEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::GameObjectContextEventHandler, this));

  GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::TreePropertyEventHandler, this));
}

void plPropertyAnimAssetDocument::SetAnimationDurationTicks(plUInt64 uiNumTicks)
{
  const plPropertyAnimationTrackGroup* pProp = GetProperties();

  if (pProp->m_uiCurveDuration == uiNumTicks)
    return;

  {
    plCommandHistory* history = GetCommandHistory();
    history->StartTransaction("Set Animation Duration");

    plSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = GetPropertyObject()->GetGuid();
    cmdSet.m_sProperty = "Duration";
    cmdSet.m_NewValue = uiNumTicks;
    history->AddCommand(cmdSet).AssertSuccess();

    history->FinishTransaction();
  }

  {
    plPropertyAnimAssetDocumentEvent e;
    e.m_pDocument = this;
    e.m_Type = plPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged;
    m_PropertyAnimEvents.Broadcast(e);
  }
}

plUInt64 plPropertyAnimAssetDocument::GetAnimationDurationTicks() const
{
  const plPropertyAnimationTrackGroup* pProp = GetProperties();

  return pProp->m_uiCurveDuration;
}


plTime plPropertyAnimAssetDocument::GetAnimationDurationTime() const
{
  const plInt64 ticks = GetAnimationDurationTicks();

  return plTime::MakeFromSeconds(ticks / 4800.0);
}

void plPropertyAnimAssetDocument::AdjustDuration()
{
  plUInt64 uiDuration = 480;

  const plPropertyAnimationTrackGroup* pProp = GetProperties();

  for (plUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const plPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    for (const auto& cp : pTrack->m_FloatCurve.m_ControlPoints)
    {
      uiDuration = plMath::Max(uiDuration, (plUInt64)cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_ColorCPs)
    {
      uiDuration = plMath::Max<plInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_AlphaCPs)
    {
      uiDuration = plMath::Max<plInt64>(uiDuration, cp.m_iTick);
    }

    for (const auto& cp : pTrack->m_ColorGradient.m_IntensityCPs)
    {
      uiDuration = plMath::Max<plInt64>(uiDuration, cp.m_iTick);
    }
  }

  SetAnimationDurationTicks(uiDuration);
}

bool plPropertyAnimAssetDocument::SetScrubberPosition(plUInt64 uiTick)
{
  if (!m_bPlayAnimation)
  {
    const plUInt32 uiTicksPerFrame = 4800 / GetProperties()->m_uiFramesPerSecond;
    uiTick = (plUInt64)plMath::RoundToMultiple((double)uiTick, (double)uiTicksPerFrame);
  }
  uiTick = plMath::Clamp<plUInt64>(uiTick, 0, GetAnimationDurationTicks());

  if (m_uiScrubberTickPos == uiTick)
    return false;

  m_uiScrubberTickPos = uiTick;
  ApplyAnimation();

  plPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged;
  m_PropertyAnimEvents.Broadcast(e);

  return true;
}

plTransformStatus plPropertyAnimAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
  const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const plPropertyAnimationTrackGroup* pProp = GetProperties();

  plPropertyAnimResourceDescriptor desc;
  desc.m_AnimationDuration = GetAnimationDurationTime();

  for (plUInt32 i = 0; i < pProp->m_Tracks.GetCount(); ++i)
  {
    const plPropertyAnimationTrack* pTrack = pProp->m_Tracks[i];

    if (pTrack->m_Target == plPropertyAnimTarget::Color)
    {
      auto& anim = desc.m_ColorAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_ColorGradient.FillGradientData(anim.m_Gradient);
      anim.m_Gradient.SortControlPoints();
    }
    else
    {
      auto& anim = desc.m_FloatAnimations.ExpandAndGetRef();
      anim.m_sObjectSearchSequence = pTrack->m_sObjectSearchSequence;
      anim.m_sComponentType = pTrack->m_sComponentType;
      anim.m_sPropertyPath = pTrack->m_sPropertyPath;
      anim.m_Target = pTrack->m_Target;
      pTrack->m_FloatCurve.ConvertToRuntimeData(anim.m_Curve);
      anim.m_Curve.SortControlPoints();
      anim.m_Curve.ApplyTangentModes();
      anim.m_Curve.ClampTangents();
    }
  }

  // sort animation tracks by object path for better cache reuse at runtime
  {
    desc.m_FloatAnimations.Sort([](const plFloatPropertyAnimEntry& lhs, const plFloatPropertyAnimEntry& rhs) -> bool {
      const plInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });

    desc.m_ColorAnimations.Sort([](const plColorPropertyAnimEntry& lhs, const plColorPropertyAnimEntry& rhs) -> bool {
      const plInt32 res = lhs.m_sObjectSearchSequence.Compare(rhs.m_sObjectSearchSequence);
      if (res < 0)
        return true;
      if (res > 0)
        return false;

      return lhs.m_sComponentType < rhs.m_sComponentType; });
  }

  pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

  desc.Save(stream);

  return plStatus(PLASMA_SUCCESS);
}


void plPropertyAnimAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = PLASMA_DEFAULT_NEW(plIPCObjectMirrorEditor);
  // Filter needs to be set before base class init as that one sends the doc.
  // (Local mirror ignores temporaries, i.e. only mirrors the asset itself)
  m_ObjectMirror.SetFilterFunction([this](const plDocumentObject* pObject, plStringView sProperty) -> bool { return !static_cast<plPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  // (Remote IPC mirror only sends temporaries, i.e. the context)
  m_pMirror->SetFilterFunction([this](const plDocumentObject* pObject, plStringView sProperty) -> bool { return static_cast<plPropertyAnimObjectManager*>(GetObjectManager())->IsTemporary(pObject, sProperty); });
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  // Important to do these after base class init as we want our subscriptions to happen after the mirror of the base class.
  GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::TreeStructureEventHandler, this));
  GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plPropertyAnimAssetDocument::TreePropertyEventHandler, this));
  // Subscribe here as otherwise base init will fire a context changed event when we are not set up yet.
  // RebuildMapping();
}

void plPropertyAnimAssetDocument::GameObjectContextEventHandler(const plGameObjectContextEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectContextEvent::Type::ContextAboutToBeChanged:
      static_cast<plPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(true);
      break;
    case plGameObjectContextEvent::Type::ContextChanged:
      static_cast<plPropertyAnimObjectManager*>(GetObjectManager())->SetAllowStructureChangeOnTemporaries(false);
      RebuildMapping();
      break;
  }
}

void plPropertyAnimAssetDocument::TreeStructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  auto pManager = static_cast<plPropertyAnimObjectManager*>(GetObjectManager());
  if (e.m_pPreviousParent && pManager->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;
  if (e.m_pNewParent && pManager->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;

  if (e.m_pObject->GetType() == plGetStaticRTTI<plPropertyAnimationTrack>())
  {
    switch (e.m_EventType)
    {
      case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
        AddTrack(e.m_pObject->GetGuid());
        return;
      case plDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      case plDocumentObjectStructureEvent::Type::BeforeObjectMoved:
        RemoveTrack(e.m_pObject->GetGuid());
        return;

      default:
        break;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void plPropertyAnimAssetDocument::TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  auto pManager = static_cast<plPropertyAnimObjectManager*>(GetObjectManager());
  if (pManager->IsTemporary(e.m_pObject))
    return;

  if (e.m_pObject->GetType() == plGetStaticRTTI<plPropertyAnimationTrack>())
  {
    if (e.m_EventType == plDocumentObjectPropertyEvent::Type::PropertySet)
    {
      RemoveTrack(e.m_pObject->GetGuid());
      AddTrack(e.m_pObject->GetGuid());
      return;
    }
  }
  else
  {
    ApplyAnimation();
  }
}

void plPropertyAnimAssetDocument::RebuildMapping()
{
  while (!m_TrackTable.IsEmpty())
  {
    RemoveTrack(m_TrackTable.GetIterator().Key());
  }
  PLASMA_ASSERT_DEBUG(m_PropertyTable.IsEmpty() && m_TrackTable.IsEmpty(), "All tracks should be removed.");

  const plAbstractProperty* pTracksProp = plGetStaticRTTI<plPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks");
  PLASMA_ASSERT_DEBUG(pTracksProp, "Name of property plPropertyAnimationTrackGroup::m_Tracks has changed.");
  plHybridArray<plVariant, 16> values;
  m_pObjectAccessor->GetValues(GetPropertyObject(), pTracksProp, values).AssertSuccess();
  for (const plVariant& value : values)
  {
    AddTrack(value.Get<plUuid>());
  }
}

void plPropertyAnimAssetDocument::RemoveTrack(const plUuid& track)
{
  auto& keys = *m_TrackTable.GetValue(track);
  for (const plPropertyReference& key : keys)
  {
    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.RemoveAndSwap(track);
    ApplyAnimation(key, value);
    if (value.m_Tracks.IsEmpty())
      m_PropertyTable.Remove(key);
  }
  m_TrackTable.Remove(track);
}

void plPropertyAnimAssetDocument::AddTrack(const plUuid& track)
{
  PLASMA_ASSERT_DEV(!m_TrackTable.Contains(track), "Track already exists.");
  auto& keys = m_TrackTable[track];
  const plDocumentObject* pContext = GetContextObject();
  if (!pContext)
    return;

  auto pTrack = GetTrack(track);
  FindTrackKeys(pTrack->m_sObjectSearchSequence.GetData(), pTrack->m_sComponentType.GetData(), pTrack->m_sPropertyPath.GetData(), keys).IgnoreResult();

  for (const plPropertyReference& key : keys)
  {
    if (!m_PropertyTable.Contains(key))
    {
      PropertyValue value;
      PLASMA_VERIFY(m_pObjectAccessor->GetValue(GetObjectManager()->GetObject(key.m_Object), key.m_pProperty, value.m_InitialValue, key.m_Index).Succeeded(),
        "Computed key invalid, does not resolve to a value.");
      m_PropertyTable.Insert(key, value);
    }

    PropertyValue& value = *m_PropertyTable.GetValue(key);
    value.m_Tracks.PushBack(track);
    ApplyAnimation(key, value);
  }
}


plStatus plPropertyAnimAssetDocument::FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, plHybridArray<plPropertyReference, 1>& keys) const
{
  plObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};

  keys.Clear();
  return plObjectPropertyPath::ResolvePath(context, keys, szObjectSearchSequence, szComponentType, szPropertyPath);
}


void plPropertyAnimAssetDocument::GenerateTrackInfo(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index,
  plStringBuilder& sObjectSearchSequence, plStringBuilder& sComponentType, plStringBuilder& sPropertyPath) const
{
  plObjectPropertyPathContext context = {GetContextObject(), m_pObjectAccessor.Borrow(), "TempObjects"};
  plPropertyReference propertyRef = {pObject->GetGuid(), pProp, index};
  plObjectPropertyPath::CreatePath(context, propertyRef, sObjectSearchSequence, sComponentType, sPropertyPath).AssertSuccess();
}

void plPropertyAnimAssetDocument::ApplyAnimation()
{
  for (auto it = m_PropertyTable.GetIterator(); it.IsValid(); ++it)
  {
    ApplyAnimation(it.Key(), it.Value());
  }
}

void plPropertyAnimAssetDocument::ApplyAnimation(const plPropertyReference& key, const PropertyValue& value)
{
  plVariant animValue = value.m_InitialValue;
  plAngle euler[3];
  bool bIsRotation = false;

  for (const plUuid& track : value.m_Tracks)
  {
    auto pTrack = GetTrack(track);
    const plRTTI* pPropRtti = key.m_pProperty->GetSpecificType();

    // #TODO apply pTrack to animValue
    switch (pTrack->m_Target)
    {
      case plPropertyAnimTarget::Number:
      {
        if (pPropRtti->GetVariantType() >= plVariantType::Bool && pPropRtti->GetVariantType() <= plVariantType::Double)
        {
          plVariant value2 = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
        }
      }
      break;

      case plPropertyAnimTarget::VectorX:
      case plPropertyAnimTarget::VectorY:
      case plPropertyAnimTarget::VectorZ:
      case plPropertyAnimTarget::VectorW:
      {
        if (pPropRtti->GetVariantType() >= plVariantType::Vector2 && pPropRtti->GetVariantType() <= plVariantType::Vector4U)
        {
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          plReflectionUtils::SetComponent(animValue, (plUInt32)pTrack->m_Target - plPropertyAnimTarget::VectorX, fValue);
        }
      }
      break;

      case plPropertyAnimTarget::RotationX:
      case plPropertyAnimTarget::RotationY:
      case plPropertyAnimTarget::RotationZ:
      {
        if (pPropRtti->GetVariantType() == plVariantType::Quaternion)
        {
          bIsRotation = true;
          const double fValue = pTrack->m_FloatCurve.Evaluate(m_uiScrubberTickPos);

          euler[(plUInt32)pTrack->m_Target - plPropertyAnimTarget::RotationX] = plAngle::MakeFromDegree(fValue);
        }
      }
      break;

      case plPropertyAnimTarget::Color:
      {
        if (pPropRtti->GetVariantType() == plVariantType::Color || pPropRtti->GetVariantType() == plVariantType::ColorGamma)
        {
          plVariant value2 = pTrack->m_ColorGradient.Evaluate(m_uiScrubberTickPos);
          animValue = value2.ConvertTo(animValue.GetType());
        }
      }
      break;
    }
  }

  if (bIsRotation)
  {
    plQuat qRotation;
    qRotation = plQuat::MakeFromEulerAngles(euler[0], euler[1], euler[2]);
    animValue = qRotation;
  }

  plDocumentObject* pObj = GetObjectManager()->GetObject(key.m_Object);
  plVariant oldValue;
  PLASMA_VERIFY(m_pObjectAccessor->GetValue(pObj, key.m_pProperty, oldValue, key.m_Index).Succeeded(), "Retrieving old value failed.");

  if (oldValue != animValue)
    GetObjectManager()->SetValue(pObj, key.m_pProperty->GetPropertyName(), animValue, key.m_Index).AssertSuccess();

  // tell the gizmos and manipulators that they should update their transform
  // usually they listen to the command history and selection events, but in this case no commands are executed
  {
    plGameObjectEvent e;
    e.m_Type = plGameObjectEvent::Type::GizmoTransformMayBeInvalid;
    m_GameObjectEvents.Broadcast(e);
  }
}

void plPropertyAnimAssetDocument::SetPlayAnimation(bool bPlay)
{
  if (m_bPlayAnimation == bPlay)
    return;

  if (m_uiScrubberTickPos >= GetAnimationDurationTicks())
    m_uiScrubberTickPos = 0;

  m_bPlayAnimation = bPlay;
  if (!m_bPlayAnimation)
  {
    // During playback we do not round to frames, so we need to round it again on stop.
    SetScrubberPosition(GetScrubberPosition());
  }
  m_LastFrameTime = plTime::Now();

  plPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void plPropertyAnimAssetDocument::SetRepeatAnimation(bool bRepeat)
{
  if (m_bRepeatAnimation == bRepeat)
    return;

  m_bRepeatAnimation = bRepeat;

  plPropertyAnimAssetDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plPropertyAnimAssetDocumentEvent::Type::PlaybackChanged;
  m_PropertyAnimEvents.Broadcast(e);
}

void plPropertyAnimAssetDocument::ExecuteAnimationPlaybackStep()
{
  const plTime currentTime = plTime::Now();
  const plTime tDiff = (currentTime - m_LastFrameTime) * GetSimulationSpeed();
  const plUInt64 uiTicks = (plUInt64)(tDiff.GetSeconds() * 4800.0);
  // Accumulate further if we render too fast and round ticks to zero.
  if (uiTicks == 0)
    return;

  m_LastFrameTime = currentTime;
  const plUInt64 uiNewPos = GetScrubberPosition() + uiTicks;
  SetScrubberPosition(uiNewPos);

  if (uiNewPos > GetAnimationDurationTicks())
  {
    SetPlayAnimation(false);

    if (m_bRepeatAnimation)
      SetPlayAnimation(true);
  }
}

const plPropertyAnimationTrack* plPropertyAnimAssetDocument::GetTrack(const plUuid& track) const
{
  return const_cast<plPropertyAnimAssetDocument*>(this)->GetTrack(track);
}

plPropertyAnimationTrack* plPropertyAnimAssetDocument::GetTrack(const plUuid& track)
{
  auto obj = m_Context.GetObjectByGUID(track);
  PLASMA_ASSERT_DEBUG(obj.m_pType == plGetStaticRTTI<plPropertyAnimationTrack>(),
    "Track guid does not resolve to a track, "
    "either the track is not yet created in the mirror or already destroyed. Make sure callbacks are executed in the right order.");
  auto pTrack = static_cast<plPropertyAnimationTrack*>(obj.m_pObject);
  return pTrack;
}


plStatus plPropertyAnimAssetDocument::CanAnimate(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target) const
{
  if (!pObject)
    return plStatus("Object is null.");
  if (!pProp)
    return plStatus("Property is null.");
  if (index.IsValid())
    return plStatus("Property indices not supported.");

  if (!GetContextObject())
    return plStatus("No context set.");

  {
    const plDocumentObject* pNode = pObject;
    while (pNode && pNode != GetContextObject())
    {
      pNode = pNode->GetParent();
    }
    if (!pNode)
    {
      return plStatus("Object not below context sub-tree.");
    }
  }
  plPropertyReference key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;

  plStringBuilder sObjectSearchSequence;
  plStringBuilder sComponentType;
  plStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  const plAbstractProperty* pName = plGetStaticRTTI<plGameObject>()->FindPropertyByName("Name");
  const plDocumentObject* pNode = pObject;
  while (pNode != GetContextObject() && pNode->GetType() != plGetStaticRTTI<plGameObject>())
  {
    pNode = pNode->GetParent();
  }
  plString sName = m_pObjectAccessor->Get<plString>(pNode, pName);

  if (sName.IsEmpty() && pNode != GetContextObject())
  {
    return plStatus("Empty node name only allowed on context root object animations.");
  }

  plHybridArray<plPropertyReference, 1> keys;
  return FindTrackKeys(sObjectSearchSequence.GetData(), sComponentType.GetData(), sPropertyPath.GetData(), keys);
}

plUuid plPropertyAnimAssetDocument::FindTrack(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target) const
{
  plPropertyReference key;
  key.m_Object = pObject->GetGuid();
  key.m_pProperty = pProp;
  key.m_Index = index;
  if (const PropertyValue* value = m_PropertyTable.GetValue(key))
  {
    for (const plUuid& track : value->m_Tracks)
    {
      auto pTrack = GetTrack(track);
      if (pTrack->m_Target == target)
        return track;
    }
  }
  return plUuid();
}

static plColorGammaUB g_CurveColors[10][3] = {
  {plColorGammaUB(255, 102, 0), plColorGammaUB(76, 255, 0), plColorGammaUB(0, 255, 255)},
  {plColorGammaUB(239, 35, 0), plColorGammaUB(127, 255, 0), plColorGammaUB(0, 0, 255)},
  {plColorGammaUB(205, 92, 92), plColorGammaUB(120, 158, 39), plColorGammaUB(81, 120, 188)},
  {plColorGammaUB(255, 105, 180), plColorGammaUB(0, 250, 154), plColorGammaUB(0, 191, 255)},
  {plColorGammaUB(220, 20, 60), plColorGammaUB(0, 255, 127), plColorGammaUB(30, 144, 255)},
  {plColorGammaUB(240, 128, 128), plColorGammaUB(60, 179, 113), plColorGammaUB(135, 206, 250)},
  {plColorGammaUB(178, 34, 34), plColorGammaUB(46, 139, 87), plColorGammaUB(65, 105, 225)},
  {plColorGammaUB(211, 122, 122), plColorGammaUB(144, 238, 144), plColorGammaUB(135, 206, 235)},
  {plColorGammaUB(219, 112, 147), plColorGammaUB(0, 128, 0), plColorGammaUB(70, 130, 180)},
  {plColorGammaUB(255, 182, 193), plColorGammaUB(102, 205, 170), plColorGammaUB(100, 149, 237)},
};

static plColorGammaUB g_FloatColors[10] = {
  plColorGammaUB(138, 43, 226),
  plColorGammaUB(139, 0, 139),
  plColorGammaUB(153, 50, 204),
  plColorGammaUB(148, 0, 211),
  plColorGammaUB(218, 112, 214),
  plColorGammaUB(221, 160, 221),
  plColorGammaUB(128, 0, 128),
  plColorGammaUB(102, 51, 153),
  plColorGammaUB(106, 90, 205),
  plColorGammaUB(238, 130, 238),
};

plUuid plPropertyAnimAssetDocument::CreateTrack(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target)
{
  plStringBuilder sObjectSearchSequence;
  plStringBuilder sComponentType;
  plStringBuilder sPropertyPath;
  GenerateTrackInfo(pObject, pProp, index, sObjectSearchSequence, sComponentType, sPropertyPath);

  plObjectCommandAccessor accessor(GetCommandHistory());
  const plRTTI* pTrackType = plGetStaticRTTI<plPropertyAnimationTrack>();
  plUuid newTrack;
  PLASMA_VERIFY(
    accessor.AddObject(GetPropertyObject(), plGetStaticRTTI<plPropertyAnimationTrackGroup>()->FindPropertyByName("Tracks"), -1, pTrackType, newTrack)
      .Succeeded(),
    "Adding track failed.");
  const plDocumentObject* pTrackObj = accessor.GetObject(newTrack);
  plVariant value = sObjectSearchSequence.GetData();
  PLASMA_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ObjectPath"), value).Succeeded(), "Adding track failed.");
  value = sComponentType.GetData();
  PLASMA_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("ComponentType"), value).Succeeded(), "Adding track failed.");
  value = sPropertyPath.GetData();
  PLASMA_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Property"), value).Succeeded(), "Adding track failed.");
  value = (int)target;
  PLASMA_VERIFY(accessor.SetValue(pTrackObj, pTrackType->FindPropertyByName("Target"), value).Succeeded(), "Adding track failed.");

  {
    const plAbstractProperty* pFloatCurveProp = pTrackType->FindPropertyByName("FloatCurve");
    plUuid floatCurveGuid = accessor.Get<plUuid>(pTrackObj, pFloatCurveProp);
    const plDocumentObject* pFloatCurveObject = GetObjectManager()->GetObject(floatCurveGuid);

    const plAbstractProperty* pColorProp = plGetStaticRTTI<plSingleCurveData>()->FindPropertyByName("Color");

    plColorGammaUB color = plColor::White;

    const plUInt32 uiNameHash = plHashingUtils::xxHash32(sObjectSearchSequence.GetData(), sObjectSearchSequence.GetElementCount());
    const plUInt32 uiColorIdx = uiNameHash % PLASMA_ARRAY_SIZE(g_CurveColors);

    switch (target)
    {
      case plPropertyAnimTarget::Number:
        color = g_FloatColors[uiColorIdx];
        break;
      case plPropertyAnimTarget::VectorX:
      case plPropertyAnimTarget::RotationX:
        color = g_CurveColors[uiColorIdx][0];
        break;
      case plPropertyAnimTarget::VectorY:
      case plPropertyAnimTarget::RotationY:
        color = g_CurveColors[uiColorIdx][1];
        break;
      case plPropertyAnimTarget::VectorZ:
      case plPropertyAnimTarget::RotationZ:
        color = g_CurveColors[uiColorIdx][2];
        break;
      case plPropertyAnimTarget::VectorW:
        color = plColor::Beige;
        break;
      default:
        break;
    }

    accessor.SetValue(pFloatCurveObject, pColorProp, color).AssertSuccess();
  }

  return newTrack;
}

plUuid plPropertyAnimAssetDocument::FindCurveCp(const plUuid& trackGuid, plInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  plInt32 iIndex = -1;
  for (plUInt32 i = 0; i < pTrack->m_FloatCurve.m_ControlPoints.GetCount(); i++)
  {
    if (pTrack->m_FloatCurve.m_ControlPoints[i].m_iTick == iTickX)
    {
      iIndex = (plInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return plUuid();

  const plAbstractProperty* pCurveProp = plGetStaticRTTI<plPropertyAnimationTrack>()->FindPropertyByName("FloatCurve");
  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  plUuid curveGuid = m_pObjectAccessor->Get<plUuid>(trackObject, pCurveProp);
  const plAbstractProperty* pControlPointsProp = plGetStaticRTTI<plSingleCurveData>()->FindPropertyByName("ControlPoints");
  const plDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  plUuid cpGuid = m_pObjectAccessor->Get<plUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

plUuid plPropertyAnimAssetDocument::InsertCurveCpAt(const plUuid& track, plInt64 iTickX, double fNewPosY)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Control Point");

  const plDocumentObject* trackObject = GetObjectManager()->GetObject(track);
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  plUuid newObjectGuid;
  PLASMA_VERIFY(acc.AddObject(accessor.GetObject(curveGuid.Get<plUuid>()), "ControlPoints", -1, plGetStaticRTTI<plCurveControlPointData>(), newObjectGuid)
              .Succeeded(),
    "");
  auto curveCPObj = accessor.GetObject(newObjectGuid);
  PLASMA_VERIFY(acc.SetValue(curveCPObj, "Tick", iTickX).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(curveCPObj, "Value", fNewPosY).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(curveCPObj, "LeftTangent", plVec2(-0.1f, 0.0f)).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(curveCPObj, "RightTangent", plVec2(+0.1f, 0.0f)).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

plUuid plPropertyAnimAssetDocument::FindGradientColorCp(const plUuid& trackGuid, plInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  plInt32 iIndex = -1;
  for (plUInt32 i = 0; i < pTrack->m_ColorGradient.m_ColorCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_ColorCPs[i].m_iTick == iTickX)
    {
      iIndex = (plInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return plUuid();

  const plAbstractProperty* pCurveProp = plGetStaticRTTI<plPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  plUuid curveGuid = m_pObjectAccessor->Get<plUuid>(trackObject, pCurveProp);
  const plAbstractProperty* pControlPointsProp = plGetStaticRTTI<plColorGradientAssetData>()->FindPropertyByName("ColorCPs");
  const plDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  plUuid cpGuid = m_pObjectAccessor->Get<plUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

plUuid plPropertyAnimAssetDocument::InsertGradientColorCpAt(const plUuid& trackGuid, plInt64 iTickX, const plColorGammaUB& color)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;

  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Color Control Point");
  plUuid newObjectGuid;
  PLASMA_VERIFY(acc.AddObject(gradientObject, "ColorCPs", -1, plGetStaticRTTI<plColorControlPoint>(), newObjectGuid).Succeeded(), "");
  const plDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  PLASMA_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(cpObject, "Red", color.r).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(cpObject, "Green", color.g).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(cpObject, "Blue", color.b).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

plUuid plPropertyAnimAssetDocument::FindGradientAlphaCp(const plUuid& trackGuid, plInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  plInt32 iIndex = -1;
  for (plUInt32 i = 0; i < pTrack->m_ColorGradient.m_AlphaCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_AlphaCPs[i].m_iTick == iTickX)
    {
      iIndex = (plInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return plUuid();

  const plAbstractProperty* pCurveProp = plGetStaticRTTI<plPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  plUuid curveGuid = m_pObjectAccessor->Get<plUuid>(trackObject, pCurveProp);
  const plAbstractProperty* pControlPointsProp = plGetStaticRTTI<plColorGradientAssetData>()->FindPropertyByName("AlphaCPs");
  const plDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  plUuid cpGuid = m_pObjectAccessor->Get<plUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

plUuid plPropertyAnimAssetDocument::InsertGradientAlphaCpAt(const plUuid& trackGuid, plInt64 iTickX, plUInt8 uiAlpha)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;

  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Alpha Control Point");
  plUuid newObjectGuid;
  PLASMA_VERIFY(acc.AddObject(gradientObject, "AlphaCPs", -1, plGetStaticRTTI<plAlphaControlPoint>(), newObjectGuid).Succeeded(), "");
  const plDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  PLASMA_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(cpObject, "Alpha", uiAlpha).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

plUuid plPropertyAnimAssetDocument::FindGradientIntensityCp(const plUuid& trackGuid, plInt64 iTickX)
{
  auto pTrack = GetTrack(trackGuid);
  plInt32 iIndex = -1;
  for (plUInt32 i = 0; i < pTrack->m_ColorGradient.m_IntensityCPs.GetCount(); i++)
  {
    if (pTrack->m_ColorGradient.m_IntensityCPs[i].m_iTick == iTickX)
    {
      iIndex = (plInt32)i;
      break;
    }
  }
  if (iIndex == -1)
    return plUuid();

  const plAbstractProperty* pCurveProp = plGetStaticRTTI<plPropertyAnimationTrack>()->FindPropertyByName("Gradient");
  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  plUuid curveGuid = m_pObjectAccessor->Get<plUuid>(trackObject, pCurveProp);
  const plAbstractProperty* pControlPointsProp = plGetStaticRTTI<plColorGradientAssetData>()->FindPropertyByName("IntensityCPs");
  const plDocumentObject* curveObject = GetObjectManager()->GetObject(curveGuid);
  plUuid cpGuid = m_pObjectAccessor->Get<plUuid>(curveObject, pControlPointsProp, iIndex);
  return cpGuid;
}

plUuid plPropertyAnimAssetDocument::InsertGradientIntensityCpAt(const plUuid& trackGuid, plInt64 iTickX, float fIntensity)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;

  const plDocumentObject* trackObject = GetObjectManager()->GetObject(trackGuid);
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = GetObjectManager()->GetObject(gradientGuid);

  acc.StartTransaction("Add Intensity Control Point");
  plUuid newObjectGuid;
  PLASMA_VERIFY(acc.AddObject(gradientObject, "IntensityCPs", -1, plGetStaticRTTI<plIntensityControlPoint>(), newObjectGuid).Succeeded(), "");
  const plDocumentObject* cpObject = GetObjectManager()->GetObject(newObjectGuid);
  PLASMA_VERIFY(acc.SetValue(cpObject, "Tick", iTickX).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(cpObject, "Intensity", fIntensity).Succeeded(), "");
  acc.FinishTransaction();
  return newObjectGuid;
}

plUuid plPropertyAnimAssetDocument::InsertEventTrackCpAt(plInt64 iTickX, const char* szValue)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  plUuid trackGuid = accessor.Get<plUuid>(GetPropertyObject(), pTrackProp);

  plUuid newObjectGuid;
  PLASMA_VERIFY(
    acc.AddObject(accessor.GetObject(trackGuid), "ControlPoints", -1, plGetStaticRTTI<plEventTrackControlPointData>(), newObjectGuid).Succeeded(),
    "");
  const plDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  PLASMA_VERIFY(acc.SetValue(pCPObj, "Tick", iTickX).Succeeded(), "");
  PLASMA_VERIFY(acc.SetValue(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}
