#include <GameEngine/GameEnginePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Curves/Curve1DResource.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Animation/PropertyAnimComponent.h>

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_COMPONENT_TYPE(plPropertyAnimComponent, 3, plComponentMode::Dynamic)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Property_Animation")),
      PLASMA_MEMBER_PROPERTY("Playing", m_bPlaying)->AddAttributes(new plDefaultValueAttribute(true)),
      PLASMA_ENUM_MEMBER_PROPERTY("Mode", plPropertyAnimMode, m_AnimationMode),
      PLASMA_MEMBER_PROPERTY("RandomOffset", m_RandomOffset)->AddAttributes(new plClampValueAttribute(plTime::Seconds(0), plVariant())),
      PLASMA_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(-10.0f, +10.0f)),
      PLASMA_MEMBER_PROPERTY("RangeLow", m_AnimationRangeLow)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
      PLASMA_MEMBER_PROPERTY("RangeHigh", m_AnimationRangeHigh)->AddAttributes(new plClampValueAttribute(plTime(), plVariant()), new plDefaultValueAttribute(plTime::Seconds(60 * 60))),
    } PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Animation"),
    } PLASMA_END_ATTRIBUTES;
    PLASMA_BEGIN_MESSAGEHANDLERS
    {
      PLASMA_MESSAGE_HANDLER(plMsgSetPlaying, OnMsgSetPlaying),
    } PLASMA_END_MESSAGEHANDLERS;
    PLASMA_BEGIN_MESSAGESENDERS
    {
      PLASMA_MESSAGE_SENDER(m_EventTrackMsgSender),
      PLASMA_MESSAGE_SENDER(m_ReachedEndMsgSender),
    } PLASMA_END_MESSAGESENDERS;
    PLASMA_BEGIN_FUNCTIONS
    {PLASMA_SCRIPT_FUNCTION_PROPERTY(PlayAnimationRange, In, "RangeLow", In, "RangeHigh")} PLASMA_END_FUNCTIONS;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPropertyAnimComponent::plPropertyAnimComponent()
{
  m_AnimationRangeHigh = plTime::Seconds(60.0 * 60.0);
}

plPropertyAnimComponent::~plPropertyAnimComponent() = default;

void plPropertyAnimComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPropertyAnim;
  s << m_AnimationMode;
  s << m_RandomOffset;
  s << m_fSpeed;
  s << m_AnimationTime;
  s << m_bReverse;
  s << m_AnimationRangeLow;
  s << m_AnimationRangeHigh;

  s << m_bPlaying;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void plPropertyAnimComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPropertyAnim;

  if (uiVersion >= 2)
  {
    s >> m_AnimationMode;
    s >> m_RandomOffset;
    s >> m_fSpeed;
    s >> m_AnimationTime;
    s >> m_bReverse;
    s >> m_AnimationRangeLow;
    s >> m_AnimationRangeHigh;
  }

  if (uiVersion >= 3)
  {
    s >> m_bPlaying;
  }
}

void plPropertyAnimComponent::SetPropertyAnimFile(const char* szFile)
{
  plPropertyAnimResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plPropertyAnimResource>(szFile);
  }

  SetPropertyAnim(hResource);
}

const char* plPropertyAnimComponent::GetPropertyAnimFile() const
{
  if (!m_hPropertyAnim.IsValid())
    return "";

  return m_hPropertyAnim.GetResourceID();
}

void plPropertyAnimComponent::SetPropertyAnim(const plPropertyAnimResourceHandle& hPropertyAnim)
{
  m_hPropertyAnim = hPropertyAnim;
}

void plPropertyAnimComponent::PlayAnimationRange(plTime rangeLow, plTime rangeHigh)
{
  m_AnimationRangeLow = rangeLow;
  m_AnimationRangeHigh = rangeHigh;

  m_bPlaying = true;

  StartPlayback();
}


void plPropertyAnimComponent::OnMsgSetPlaying(plMsgSetPlaying& ref_msg)
{
  m_bPlaying = ref_msg.m_bPlay;
}

void plPropertyAnimComponent::CreatePropertyBindings()
{
  m_ColorBindings.Clear();
  m_ComponentFloatBindings.Clear();
  m_GoFloatBindings.Clear();

  m_pAnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  plResourceLock<plPropertyAnimResource> pAnimation(m_hPropertyAnim, plResourceAcquireMode::BlockTillLoaded);

  if (!pAnimation || pAnimation.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
    return;

  m_pAnimDesc = pAnimation->GetDescriptor();

  for (const plFloatPropertyAnimEntry& anim : m_pAnimDesc->m_FloatAnimations)
  {
    plHybridArray<plGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (plGameObject* pTargetObject : targets)
    {
      // allow to animate properties on the plGameObject
      if (anim.m_pComponentRtti == nullptr)
      {
        CreateGameObjectBinding(&anim, plGetStaticRTTI<plGameObject>(), pTargetObject, pTargetObject->GetHandle());
      }
      else
      {
        plComponent* pComp;
        if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
        {
          CreateFloatPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
        }
      }
    }
  }

  for (const plColorPropertyAnimEntry& anim : m_pAnimDesc->m_ColorAnimations)
  {
    plHybridArray<plGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (plGameObject* pTargetObject : targets)
    {
      plComponent* pComp;
      if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
      {
        CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
      }
    }
  }
}

void plPropertyAnimComponent::CreateGameObjectBinding(const plFloatPropertyAnimEntry* pAnim, const plRTTI* pOwnerRtti, void* pObject, const plGameObjectHandle& hGameObject)
{
  if (pAnim->m_Target < plPropertyAnimTarget::Number || pAnim->m_Target > plPropertyAnimTarget::RotationZ)
    return;

  const plAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != plPropertyCategory::Member)
    return;

  auto pMember = static_cast<const plAbstractMemberProperty*>(pAbstract);

  const plRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == plPropertyAnimTarget::Number)
  {
    // Game objects only support to animate Position, Rotation,
    // Non-Uniform Scale, the one single-float Uniform scale value
    // and the active flag
    if (pPropRtti != plGetStaticRTTI<float>() && pPropRtti != plGetStaticRTTI<bool>())
      return;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::RotationX && pAnim->m_Target <= plPropertyAnimTarget::RotationZ)
  {
    if (pPropRtti != plGetStaticRTTI<plQuat>())
      return;
  }
  else
  {
    if (pPropRtti != plGetStaticRTTI<plVec2>() && pPropRtti != plGetStaticRTTI<plVec3>() && pPropRtti != plGetStaticRTTI<plVec4>())
      return;
  }

  GameObjectBinding* binding = nullptr;
  for (plUInt32 i = 0; i < m_GoFloatBindings.GetCount(); ++i)
  {
    auto& b = m_GoFloatBindings[i];

    if (b.m_hObject == hGameObject && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_GoFloatBindings.ExpandAndGetRef();
  }

  binding->m_hObject = hGameObject;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive

  if (pAnim->m_Target >= plPropertyAnimTarget::VectorX && pAnim->m_Target <= plPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)plPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::RotationX && pAnim->m_Target <= plPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)plPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    PLASMA_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}

void plPropertyAnimComponent::CreateFloatPropertyBinding(const plFloatPropertyAnimEntry* pAnim, const plRTTI* pOwnerRtti, void* pObject, const plComponentHandle& hComponent)
{
  if (pAnim->m_Target < plPropertyAnimTarget::Number || pAnim->m_Target > plPropertyAnimTarget::VectorW)
    return;

  const plAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != plPropertyCategory::Member)
    return;

  auto pMember = static_cast<const plAbstractMemberProperty*>(pAbstract);

  const plRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == plPropertyAnimTarget::Number)
  {
    if (pPropRtti != plGetStaticRTTI<float>() && pPropRtti != plGetStaticRTTI<double>() && pPropRtti != plGetStaticRTTI<bool>() && pPropRtti != plGetStaticRTTI<plInt64>() && pPropRtti != plGetStaticRTTI<plInt32>() && pPropRtti != plGetStaticRTTI<plInt16>() &&
        pPropRtti != plGetStaticRTTI<plInt8>() && pPropRtti != plGetStaticRTTI<plUInt64>() && pPropRtti != plGetStaticRTTI<plUInt32>() && pPropRtti != plGetStaticRTTI<plUInt16>() && pPropRtti != plGetStaticRTTI<plUInt8>() && pPropRtti != plGetStaticRTTI<plAngle>() &&
        pPropRtti != plGetStaticRTTI<plTime>())
      return;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::VectorX && pAnim->m_Target <= plPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != plGetStaticRTTI<plVec2>() && pPropRtti != plGetStaticRTTI<plVec3>() && pPropRtti != plGetStaticRTTI<plVec4>())
      return;
  }
  else
  {
    // Quaternions are not supported for regular types
    return;
  }

  ComponentFloatBinding* binding = nullptr;
  for (plUInt32 i = 0; i < m_ComponentFloatBindings.GetCount(); ++i)
  {
    auto& b = m_ComponentFloatBindings[i];

    if (b.m_hComponent == hComponent && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_ComponentFloatBindings.ExpandAndGetRef();
  }

  binding->m_hComponent = hComponent;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  if (pAnim->m_Target >= plPropertyAnimTarget::VectorX && pAnim->m_Target <= plPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)plPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::RotationX && pAnim->m_Target <= plPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)plPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= plPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    PLASMA_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}

void plPropertyAnimComponent::CreateColorPropertyBinding(const plColorPropertyAnimEntry* pAnim, const plRTTI* pOwnerRtti, void* pObject, const plComponentHandle& hComponent)
{
  if (pAnim->m_Target != plPropertyAnimTarget::Color)
    return;

  const plAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != plPropertyCategory::Member)
    return;

  auto pMember = static_cast<const plAbstractMemberProperty*>(pAbstract);

  const plRTTI* pPropRtti = pMember->GetSpecificType();

  if (pPropRtti != plGetStaticRTTI<plColor>() && pPropRtti != plGetStaticRTTI<plColorGammaUB>())
    return;

  ColorBinding& binding = m_ColorBindings.ExpandAndGetRef();
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our SharedPtr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
}

void plPropertyAnimComponent::ApplyAnimations(const plTime& tDiff)
{
  if (m_fSpeed == 0.0f || m_pAnimDesc == nullptr)
    return;

  const plTime fLookupPos = ComputeAnimationLookup(tDiff);

  for (plUInt32 i = 0; i < m_ComponentFloatBindings.GetCount();)
  {
    const auto& binding = m_ComponentFloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      plComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))
      {
        // remove dead references
        m_ComponentFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyFloatAnimation(m_ComponentFloatBindings[i], fLookupPos);

    ++i;
  }

  for (plUInt32 i = 0; i < m_ColorBindings.GetCount();)
  {
    const auto& binding = m_ColorBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      plComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))

      {
        // remove dead references
        m_ColorBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyColorAnimation(m_ColorBindings[i], fLookupPos);

    ++i;
  }

  for (plUInt32 i = 0; i < m_GoFloatBindings.GetCount();)
  {
    const auto& binding = m_GoFloatBindings[i];

    // if we have a game object handle, use it to check that the component is still alive
    if (!binding.m_hObject.IsInvalidated())
    {
      plGameObject* pObject;
      if (!GetWorld()->TryGetObject(binding.m_hObject, pObject))
      {
        // remove dead references
        m_GoFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pObject);
    }

    ApplyFloatAnimation(m_GoFloatBindings[i], fLookupPos);

    ++i;
  }
}

plTime plPropertyAnimComponent::ComputeAnimationLookup(plTime tDiff)
{
  m_AnimationRangeLow = plMath::Clamp(m_AnimationRangeLow, plTime::MakeZero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = plMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  const plTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

  if (duration.IsZero())
  {
    m_bPlaying = false;
    return m_AnimationRangeLow;
  }

  tDiff = m_fSpeed * tDiff;

  plMsgAnimationReachedEnd reachedEndMsg;
  plTime tStart = m_AnimationTime;

  if (m_AnimationMode == plPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_fSpeed > 0 && m_AnimationTime >= m_AnimationRangeHigh)
    {
      m_AnimationTime = m_AnimationRangeHigh;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }
    else if (m_fSpeed < 0 && m_AnimationTime <= m_AnimationRangeLow)
    {
      m_AnimationTime = m_AnimationRangeLow;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == plPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > m_AnimationRangeHigh)
    {
      m_AnimationTime -= duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeHigh);
      tStart = m_AnimationRangeLow;
    }

    while (m_AnimationTime < m_AnimationRangeLow)
    {
      m_AnimationTime += duration;

      m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeLow);
      tStart = m_AnimationRangeHigh;
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == plPropertyAnimMode::BackAndForth)
  {
    const bool bReverse = m_fSpeed < 0 ? !m_bReverse : m_bReverse;

    if (bReverse)
      m_AnimationTime -= tDiff;
    else
      m_AnimationTime += tDiff;

    // ping pong back and forth as long as the current animation time is outside the valid range
    while (true)
    {
      if (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime = m_AnimationRangeHigh - (m_AnimationTime - m_AnimationRangeHigh);
        m_bReverse = true;

        EvaluateEventTrack(tStart, m_AnimationRangeHigh);
        tStart = m_AnimationRangeHigh;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else if (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime = m_AnimationRangeLow + (m_AnimationRangeLow - m_AnimationTime);
        m_bReverse = false;

        EvaluateEventTrack(tStart, m_AnimationRangeLow);
        tStart = m_AnimationRangeLow;

        m_ReachedEndMsgSender.SendEventMessage(reachedEndMsg, this, GetOwner());
      }
      else
      {
        EvaluateEventTrack(tStart, m_AnimationTime);
        break;
      }
    }
  }

  return m_AnimationTime;
}

void plPropertyAnimComponent::EvaluateEventTrack(plTime startTime, plTime endTime)
{
  const plEventTrack& et = m_pAnimDesc->m_EventTrack;

  if (et.IsEmpty())
    return;

  plHybridArray<plHashedString, 8> events;
  et.Sample(startTime, endTime, events);

  for (const plHashedString& sEvent : events)
  {
    plMsgGenericEvent msg;
    msg.m_sMessage = sEvent;
    m_EventTrackMsgSender.SendEventMessage(msg, this, GetOwner());
  }
}

void plPropertyAnimComponent::OnSimulationStarted()
{
  CreatePropertyBindings();

  StartPlayback();
}

void plPropertyAnimComponent::StartPlayback()
{
  if (m_pAnimDesc == nullptr)
    return;

  m_AnimationRangeLow = plMath::Clamp(m_AnimationRangeLow, plTime::MakeZero(), m_pAnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = plMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_pAnimDesc->m_AnimationDuration);

  // when starting with a negative speed, start at the end of the animation and play backwards
  // important for play-once mode
  if (m_fSpeed < 0.0f)
  {
    m_AnimationTime = m_AnimationRangeHigh;
  }
  else
  {
    m_AnimationTime = m_AnimationRangeLow;
  }

  if (!m_RandomOffset.IsZero() && m_pAnimDesc->m_AnimationDuration.IsPositive())
  {
    // should the random offset also be scaled by the speed factor? I guess not
    m_AnimationTime += plMath::Abs(m_fSpeed) * plTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomOffset.GetSeconds()));

    const plTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

    if (duration.IsZeroOrNegative())
    {
      m_AnimationTime = m_AnimationRangeLow;
    }
    else
    {
      // adjust current time to be inside the valid range
      // do not clamp, as that would give a skewed random chance
      while (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime -= duration;
      }

      while (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime += duration;
      }
    }
  }
}

void plPropertyAnimComponent::ApplySingleFloatAnimation(const FloatBinding& binding, plTime lookupTime)
{
  const plRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  double fFinalValue = 0;
  {
    const plCurve1D& curve = binding.m_pAnimation[0]->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = curve.Evaluate(lookupTime.GetSeconds());
  }

  if (pRtti == plGetStaticRTTI<bool>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<bool>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, fFinalValue > 0.99); // this is close to what plVariant does (not identical, that does an int cast != 0), but faster to evaluate
    return;
  }
  else if (pRtti == plGetStaticRTTI<plAngle>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plAngle>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, plAngle::MakeFromDegree((float)fFinalValue));
    return;
  }
  else if (pRtti == plGetStaticRTTI<plTime>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plTime>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, plTime::Seconds(fFinalValue));
    return;
  }

  // this handles float, double, all int types, etc.
  plVariant value = fFinalValue;
  if (pRtti->GetVariantType() != plVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    plReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
  }
}

void plPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, plTime lookupTime)
{
  if (binding.m_pAnimation[0] != nullptr && binding.m_pAnimation[0]->m_Target == plPropertyAnimTarget::Number)
  {
    ApplySingleFloatAnimation(binding, lookupTime);
    return;
  }

  const plRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  float fCurValue[4] = {0, 0, 0, 0};

  if (pRtti == plGetStaticRTTI<plVec2>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec2>*>(binding.m_pMemberProperty);
    const plVec2 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
  }
  else if (pRtti == plGetStaticRTTI<plVec3>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec3>*>(binding.m_pMemberProperty);
    const plVec3 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
  }
  else if (pRtti == plGetStaticRTTI<plVec4>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec4>*>(binding.m_pMemberProperty);
    const plVec4 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
    fCurValue[3] = value.w;
  }
  else if (pRtti == plGetStaticRTTI<plQuat>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plQuat>*>(binding.m_pMemberProperty);
    const plQuat value = pTyped->GetValue(binding.m_pObject);

    plAngle euler[3];
    value.GetAsEulerAngles(euler[0], euler[1], euler[2]);
    fCurValue[0] = euler[0].GetDegree();
    fCurValue[1] = euler[1].GetDegree();
    fCurValue[2] = euler[2].GetDegree();
  }

  // evaluate all available curves
  for (plUInt32 i = 0; i < 4; ++i)
  {
    if (binding.m_pAnimation[i] != nullptr)
    {
      const plCurve1D& curve = binding.m_pAnimation[i]->m_Curve;

      if (!curve.IsEmpty())
      {
        fCurValue[i] = (float)curve.Evaluate(lookupTime.GetSeconds());
      }
    }
  }

  if (pRtti == plGetStaticRTTI<plVec2>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec2>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, plVec2(fCurValue[0], fCurValue[1]));
  }
  else if (pRtti == plGetStaticRTTI<plVec3>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec3>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, plVec3(fCurValue[0], fCurValue[1], fCurValue[2]));
  }
  else if (pRtti == plGetStaticRTTI<plVec4>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plVec4>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, plVec4(fCurValue[0], fCurValue[1], fCurValue[2], fCurValue[3]));
  }
  else if (pRtti == plGetStaticRTTI<plQuat>())
  {
    auto pTyped = static_cast<const plTypedMemberProperty<plQuat>*>(binding.m_pMemberProperty);

    plQuat rot = plQuat::MakeFromEulerAngles(plAngle::MakeFromDegree(fCurValue[0]), plAngle::MakeFromDegree(fCurValue[1]), plAngle::MakeFromDegree(fCurValue[2]));

    pTyped->SetValue(binding.m_pObject, rot);
  }
}

void plPropertyAnimComponent::ApplyColorAnimation(const ColorBinding& binding, plTime lookupTime)
{
  const plRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  if (pRtti == plGetStaticRTTI<plColorGammaUB>())
  {
    plColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  if (pRtti == plGetStaticRTTI<plColor>())
  {
    plColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);

    plColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }
}

void plPropertyAnimComponent::Update()
{
  if (m_bPlaying == false || !m_hPropertyAnim.IsValid())
    return;

  if (m_pAnimDesc == nullptr)
  {
    CreatePropertyBindings();
  }

  ApplyAnimations(GetWorld()->GetClock().GetTimeDiff());
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_PropertyAnimComponent);
