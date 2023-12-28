#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ColorAnimationComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plColorAnimationComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    PLASMA_MEMBER_PROPERTY("Duration", m_Duration),
    PLASMA_ENUM_MEMBER_PROPERTY("SetColorMode", plSetColorMode, m_SetColorMode),
    PLASMA_ENUM_MEMBER_PROPERTY("AnimationMode", plPropertyAnimMode, m_AnimationMode),
    PLASMA_ACCESSOR_PROPERTY("RandomStartOffset", GetRandomStartOffset, SetRandomStartOffset)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("ApplyToChildren", GetApplyRecursive, SetApplyRecursive),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plColorAnimationComponent::plColorAnimationComponent() = default;

void plColorAnimationComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hGradient;
  s << m_Duration;

  // version 2
  s << m_SetColorMode;
  s << m_AnimationMode;
  s << GetRandomStartOffset();
  s << GetApplyRecursive();
}

void plColorAnimationComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hGradient;
  s >> m_Duration;

  if (uiVersion >= 2)
  {
    s >> m_SetColorMode;
    s >> m_AnimationMode;
    bool b;
    s >> b;
    SetRandomStartOffset(b);
    s >> b;
    SetApplyRecursive(b);
  }
}

void plColorAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetRandomStartOffset())
  {
    m_CurAnimTime = plTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_Duration.GetSeconds()));
  }
}

void plColorAnimationComponent::SetColorGradientFile(const char* szFile)
{
  plColorGradientResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plColorGradientResource>(szFile);
  }

  SetColorGradient(hResource);
}

const char* plColorAnimationComponent::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void plColorAnimationComponent::SetColorGradient(const plColorGradientResourceHandle& hResource)
{
  m_hGradient = hResource;
}

bool plColorAnimationComponent::GetApplyRecursive() const
{
  return GetUserFlag(0);
}

void plColorAnimationComponent::SetApplyRecursive(bool value)
{
  SetUserFlag(0, value);
}

bool plColorAnimationComponent::GetRandomStartOffset() const
{
  return GetUserFlag(1);
}

void plColorAnimationComponent::SetRandomStartOffset(bool value)
{
  SetUserFlag(1, value);
}

void plColorAnimationComponent::Update()
{
  if (!m_hGradient.IsValid() || m_Duration <= plTime::Zero())
    return;

  plTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  const bool bReverse = GetUserFlag(0);

  if (bReverse)
    m_CurAnimTime -= tDiff;
  else
    m_CurAnimTime += tDiff;

  switch (m_AnimationMode)
  {
    case plPropertyAnimMode::Once:
    {
      m_CurAnimTime = plMath::Min(m_CurAnimTime, m_Duration);
      break;
    }

    case plPropertyAnimMode::Loop:
    {
      if (m_CurAnimTime >= m_Duration)
        m_CurAnimTime -= m_Duration;

      break;
    }

    case plPropertyAnimMode::BackAndForth:
    {
      if (m_CurAnimTime > m_Duration)
      {
        SetUserFlag(0, !bReverse);

        const plTime tOver = m_Duration - m_CurAnimTime;

        m_CurAnimTime = m_Duration - tOver;
      }
      else if (m_CurAnimTime < plTime::Zero())
      {
        SetUserFlag(0, !bReverse);

        m_CurAnimTime = -m_CurAnimTime;
      }

      break;
    }
  }

  plResourceLock<plColorGradientResource> pGradient(m_hGradient, plResourceAcquireMode::AllowLoadingFallback);

  if (pGradient.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  plMsgSetColor msg;
  msg.m_Color = pGradient->Evaluate(m_CurAnimTime.GetSeconds() / m_Duration.GetSeconds());
  msg.m_Mode = m_SetColorMode;

  if (GetApplyRecursive())
    GetOwner()->SendMessageRecursive(msg);
  else
    GetOwner()->SendMessage(msg);
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ColorAnimationComponent);
