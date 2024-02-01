#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

plPropertyAnimObjectAccessor::plPropertyAnimObjectAccessor(plPropertyAnimAssetDocument* pDoc, plCommandHistory* pHistory)
  : plObjectCommandAccessor(pHistory)
  , m_pDocument(pDoc)
  , m_pObjectManager(static_cast<plPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
  m_pObjAccessor = PL_DEFAULT_NEW(plObjectCommandAccessor, pHistory);
}

plStatus plPropertyAnimObjectAccessor::GetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index /*= plVariant()*/)
{
  return plObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
}

plStatus plPropertyAnimObjectAccessor::SetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index)
{
  if (IsTemporary(pObject))
  {
    plVariant oldValue;
    PL_VERIFY(m_pObjAccessor->GetValue(pObject, pProp, oldValue, index).Succeeded(), "Property does not exist, can't animate");

    plVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
    if (type >= plVariantType::Bool && type <= plVariantType::Double)
    {
      return SetCurveCp(pObject, pProp, index, plPropertyAnimTarget::Number, oldValue.ConvertTo<double>(), newValue.ConvertTo<double>());
    }
    else if (type >= plVariantType::Vector2 && type <= plVariantType::Vector4U)
    {
      const plUInt32 uiComponents = plReflectionUtils::GetComponentCount(type);
      for (plUInt32 c = 0; c < uiComponents; c++)
      {
        const double fOldValue = plReflectionUtils::GetComponent(oldValue, c);
        const double fValue = plReflectionUtils::GetComponent(newValue, c);

        if (plMath::IsEqual(fOldValue, fValue, plMath::SmallEpsilon<double>()))
          continue;

        PL_SUCCEED_OR_RETURN(
          SetCurveCp(pObject, pProp, index, static_cast<plPropertyAnimTarget::Enum>((int)plPropertyAnimTarget::VectorX + c), fOldValue, fValue));
      }

      return plStatus(PL_SUCCESS);
    }
    else if (type == plVariantType::Color)
    {
      auto oldColor = oldValue.Get<plColor>();
      plColorGammaUB oldColorGamma;
      plUInt8 oldAlpha;
      float oldIntensity;
      SeparateColor(oldColor, oldColorGamma, oldAlpha, oldIntensity);
      auto newColor = newValue.Get<plColor>();
      plColorGammaUB newColorGamma;
      plUInt8 newAlpha;
      float newIntensity;
      SeparateColor(newColor, newColorGamma, newAlpha, newIntensity);

      plStatus res(PL_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      if (!plMath::IsEqual(oldIntensity, newIntensity, plMath::SmallEpsilon<float>()) && res.Succeeded())
      {
        res = SetIntensityCurveCp(pObject, pProp, index, oldIntensity, newIntensity);
      }
      return res;
    }
    else if (type == plVariantType::ColorGamma)
    {
      auto oldColorGamma = oldValue.Get<plColorGammaUB>();
      plUInt8 oldAlpha = oldColorGamma.a;
      oldColorGamma.a = 255;

      auto newColorGamma = newValue.Get<plColorGammaUB>();
      plUInt8 newAlpha = newColorGamma.a;
      newColorGamma.a = 255;

      plStatus res(PL_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      return res;
    }
    else if (type == plVariantType::Quaternion)
    {

      const plQuat qOldRot = oldValue.Get<plQuat>();
      const plQuat qNewRot = newValue.Get<plQuat>();

      plAngle oldEuler[3];
      qOldRot.GetAsEulerAngles(oldEuler[0], oldEuler[1], oldEuler[2]);
      plAngle newEuler[3];
      qNewRot.GetAsEulerAngles(newEuler[0], newEuler[1], newEuler[2]);

      for (plUInt32 c = 0; c < 3; c++)
      {
        PL_SUCCEED_OR_RETURN(
          m_pDocument->CanAnimate(pObject, pProp, index, static_cast<plPropertyAnimTarget::Enum>((int)plPropertyAnimTarget::RotationX + c)));
        float oldValue = oldEuler[c].GetDegree();
        plUuid track = FindOrAddTrack(pObject, pProp, index, static_cast<plPropertyAnimTarget::Enum>((int)plPropertyAnimTarget::RotationX + c),
          [this, oldValue](const plUuid& trackGuid) {
            // add a control point at the start of the curve with the original value
            m_pDocument->InsertCurveCpAt(trackGuid, 0, oldValue);
          });
        const auto* pTrack = m_pDocument->GetTrack(track);
        oldEuler[c] = plAngle::MakeFromDegree(pTrack->m_FloatCurve.Evaluate(m_pDocument->GetScrubberPosition()));
      }

      for (plUInt32 c = 0; c < 3; c++)
      {
        // We assume the change is less than 180 degrees from the old value
        float fDiff = (newEuler[c] - oldEuler[c]).GetDegree();
        float iRounds = plMath::RoundToMultiple(fDiff, 360.0f);
        fDiff -= iRounds;
        newEuler[c] = oldEuler[c] + plAngle::MakeFromDegree(fDiff);
        if (oldEuler[c].IsEqualSimple(newEuler[c], plAngle::MakeFromDegree(0.01f)))
          continue;

        PL_SUCCEED_OR_RETURN(SetCurveCp(pObject, pProp, index, static_cast<plPropertyAnimTarget::Enum>((int)plPropertyAnimTarget::RotationX + c),
          oldEuler[c].GetDegree(), newEuler[c].GetDegree()));
      }

      return plStatus(PL_SUCCESS);
    }

    return plStatus(plFmt("The property '{0}' cannot be animated.", pProp->GetPropertyName()));
  }
  else
  {
    return plObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

plStatus plPropertyAnimObjectAccessor::InsertValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

plStatus plPropertyAnimObjectAccessor::RemoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

plStatus plPropertyAnimObjectAccessor::MoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  if (IsTemporary(pObject))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

plStatus plPropertyAnimObjectAccessor::AddObject(
  const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  if (IsTemporary(pParent, pParentProp))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

plStatus plPropertyAnimObjectAccessor::RemoveObject(const plDocumentObject* pObject)
{
  if (IsTemporary(pObject))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::RemoveObject(pObject);
  }
}

plStatus plPropertyAnimObjectAccessor::MoveObject(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index)
{
  if (IsTemporary(pObject))
  {
    return plStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return plObjectCommandAccessor::MoveObject(pObject, pNewParent, pParentProp, index);
  }
}

bool plPropertyAnimObjectAccessor::IsTemporary(const plDocumentObject* pObject) const
{
  return m_pObjectManager->IsTemporary(pObject);
}

bool plPropertyAnimObjectAccessor::IsTemporary(const plDocumentObject* pParent, const plAbstractProperty* pParentProp) const
{
  return m_pObjectManager->IsTemporary(pParent, pParentProp->GetPropertyName());
}


plStatus plPropertyAnimObjectAccessor::SetCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index,
  plPropertyAnimTarget::Enum target, double fOldValue, double fNewValue)
{
  PL_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, target));
  plUuid track = FindOrAddTrack(pObject, pProp, index, target, [this, fOldValue](const plUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertCurveCpAt(trackGuid, 0, fOldValue); });
  return SetOrInsertCurveCp(track, fNewValue);
}

plUuid plPropertyAnimObjectAccessor::FindOrAddTrack(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target, OnAddTrack onAddTrack)
{
  plUuid track = m_pDocument->FindTrack(pObject, pProp, index, target);
  if (!track.IsValid())
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    track = m_pDocument->CreateTrack(pObject, pProp, index, target);
    onAddTrack(track);

    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  PL_ASSERT_DEBUG(track.IsValid(), "Creating track failed.");
  return track;
}

plStatus plPropertyAnimObjectAccessor::SetOrInsertCurveCp(const plUuid& track, double fValue)
{
  const plInt64 iScrubberPos = (plInt64)m_pDocument->GetScrubberPosition();
  plUuid cpGuid = m_pDocument->FindCurveCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Value", fValue).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertCurveCpAt(track, iScrubberPos, fValue);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetColorCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, const plColorGammaUB& oldValue, const plColorGammaUB& newValue)
{
  PL_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, plPropertyAnimTarget::Color));
  plUuid track = FindOrAddTrack(pObject, pProp, index, plPropertyAnimTarget::Color, [this, &oldValue](const plUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientColorCpAt(trackGuid, 0, oldValue);
    //
  });

  PL_SUCCEED_OR_RETURN(SetOrInsertColorCurveCp(track, newValue));

  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetOrInsertColorCurveCp(const plUuid& track, const plColorGammaUB& value)
{
  const plInt64 iScrubberPos = (plInt64)m_pDocument->GetScrubberPosition();
  plUuid cpGuid = m_pDocument->FindGradientColorCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Red", value.r).Succeeded(), "");
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Green", value.g).Succeeded(), "");
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Blue", value.b).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientColorCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetAlphaCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plUInt8 oldValue, plUInt8 newValue)
{
  PL_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, plPropertyAnimTarget::Color));
  plUuid track = FindOrAddTrack(pObject, pProp, index, plPropertyAnimTarget::Color, [this, &oldValue](const plUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientAlphaCpAt(trackGuid, 0, oldValue);
    //
  });

  PL_SUCCEED_OR_RETURN(SetOrInsertAlphaCurveCp(track, newValue));
  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetOrInsertAlphaCurveCp(const plUuid& track, plUInt8 value)
{
  const plInt64 iScrubberPos = (plInt64)m_pDocument->GetScrubberPosition();
  plUuid cpGuid = m_pDocument->FindGradientAlphaCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Alpha", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientAlphaCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetIntensityCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, float oldValue, float newValue)
{
  plUuid track = FindOrAddTrack(pObject, pProp, index, plPropertyAnimTarget::Color, [this, &oldValue](const plUuid& trackGuid) {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertGradientIntensityCpAt(trackGuid, 0, oldValue);
    //
  });

  PL_SUCCEED_OR_RETURN(SetOrInsertIntensityCurveCp(track, newValue));
  return plStatus(PL_SUCCESS);
}

plStatus plPropertyAnimObjectAccessor::SetOrInsertIntensityCurveCp(const plUuid& track, float value)
{
  const plInt64 iScrubberPos = (plInt64)m_pDocument->GetScrubberPosition();
  plUuid cpGuid = m_pDocument->FindGradientIntensityCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    PL_VERIFY(m_pObjAccessor->SetValue(pCP, "Intensity", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientIntensityCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return plStatus(PL_SUCCESS);
}

void plPropertyAnimObjectAccessor::SeparateColor(const plColor& color, plColorGammaUB& gamma, plUInt8& alpha, float& intensity)
{
  alpha = static_cast<plColorGammaUB>(color).a;
  intensity = plMath::Max(color.r, color.g, color.b);
  if (intensity > 1.0f)
  {
    gamma = (color / intensity);
    gamma.a = 255;
  }
  else
  {
    intensity = 1.0f;
    gamma = color;
    gamma.a = 255;
  }
}
