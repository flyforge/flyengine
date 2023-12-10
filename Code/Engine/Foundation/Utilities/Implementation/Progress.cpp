#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Progress.h>

static plProgress* s_pGlobal = nullptr;

plProgress::plProgress() = default;

plProgress::~plProgress()
{
  if (s_pGlobal == this)
  {
    s_pGlobal = nullptr;
  }
}

float plProgress::GetCompletion() const
{
  return m_fCurrentCompletion;
}

void plProgress::SetCompletion(float fCompletion)
{
  PLASMA_ASSERT_DEV(fCompletion >= 0.0f && fCompletion <= 1.0f, "Completion value {0} is out of valid range", fCompletion);

  m_fCurrentCompletion = fCompletion;

  if (fCompletion > m_fLastReportedCompletion + 0.001f)
  {
    m_fLastReportedCompletion = fCompletion;

    plProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = plProgressEvent::Type::ProgressChanged;

    m_Events.Broadcast(e, 1);
  }
}

void plProgress::SetActiveRange(plProgressRange* pRange)
{
  if (m_pActiveRange == nullptr && pRange != nullptr)
  {
    m_fLastReportedCompletion = 0.0;
    m_fCurrentCompletion = 0.0;
    m_bCancelClicked = false;
    m_bEnableCancel = pRange->m_bAllowCancel;

    plProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = plProgressEvent::Type::ProgressStarted;

    m_Events.Broadcast(e);
  }

  if (m_pActiveRange != nullptr && pRange == nullptr)
  {
    plProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = plProgressEvent::Type::ProgressEnded;

    m_Events.Broadcast(e);
  }

  m_pActiveRange = pRange;
}

plStringView plProgress::GetMainDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return {};

  return m_pActiveRange->m_sDisplayText;
}

plStringView plProgress::GetStepDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return {};

  return m_pActiveRange->m_sStepDisplayText;
}

void plProgress::UserClickedCancel()
{
  if (m_bCancelClicked)
    return;

  m_bCancelClicked = true;

  plProgressEvent e;
  e.m_Type = plProgressEvent::Type::CancelClicked;
  e.m_pProgressbar = this;

  m_Events.Broadcast(e, 1);
}

bool plProgress::WasCanceled() const
{
  return m_bCancelClicked;
}

bool plProgress::AllowUserCancel() const
{
  return m_bEnableCancel;
}

plProgress* plProgress::GetGlobalProgressbar()
{
  if (!s_pGlobal)
  {
    static plProgress s_Global;
    return &s_Global;
  }

  return s_pGlobal;
}

void plProgress::SetGlobalProgressbar(plProgress* pProgress)
{
  s_pGlobal = pProgress;
}

//////////////////////////////////////////////////////////////////////////

plProgressRange::plProgressRange(plStringView sDisplayText, plUInt32 uiSteps, bool bAllowCancel, plProgress* pProgressbar /*= nullptr*/)
{
  PLASMA_ASSERT_DEV(uiSteps > 0, "Every progress range must have at least one step to complete");

  m_iCurrentStep = -1;
  m_fWeightedCompletion = -1.0;
  m_fSummedWeight = (double)uiSteps;

  Init(sDisplayText, bAllowCancel, pProgressbar);
}

plProgressRange::plProgressRange(plStringView sDisplayText, bool bAllowCancel, plProgress* pProgressbar /*= nullptr*/)
{
  Init(sDisplayText, bAllowCancel, pProgressbar);
}

void plProgressRange::Init(plStringView sDisplayText, bool bAllowCancel, plProgress* pProgressbar)
{
  if (pProgressbar == nullptr)
    m_pProgressbar = plProgress::GetGlobalProgressbar();
  else
    m_pProgressbar = pProgressbar;

  PLASMA_ASSERT_DEV(m_pProgressbar != nullptr, "No global progress-bar context available.");

  m_bAllowCancel = bAllowCancel;
  m_sDisplayText = sDisplayText;

  m_pParentRange = m_pProgressbar->m_pActiveRange;

  if (m_pParentRange == nullptr)
  {
    m_fPercentageBase = 0.0;
    m_fPercentageRange = 1.0;
  }
  else
  {
    m_pParentRange->ComputeCurStepBaseAndRange(m_fPercentageBase, m_fPercentageRange);
  }

  m_pProgressbar->SetActiveRange(this);
}

plProgressRange::~plProgressRange()
{
  m_pProgressbar->SetCompletion((float)(m_fPercentageBase + m_fPercentageRange));
  m_pProgressbar->SetActiveRange(m_pParentRange);
}

plProgress* plProgressRange::GetProgressbar() const
{
  return m_pProgressbar;
}

void plProgressRange::SetStepWeighting(plUInt32 uiStep, float fWeight)
{
  PLASMA_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_fSummedWeight -= GetStepWeight(uiStep);
  m_fSummedWeight += fWeight;
  m_StepWeights[uiStep] = fWeight;
}

float plProgressRange::GetStepWeight(plUInt32 uiStep) const
{
  const float* pOldWeight = m_StepWeights.GetValue(uiStep);
  return pOldWeight != nullptr ? *pOldWeight : 1.0f;
}

void plProgressRange::ComputeCurStepBaseAndRange(double& out_base, double& out_range)
{
  const double internalBase = plMath::Max(m_fWeightedCompletion, 0.0) / m_fSummedWeight;
  const double internalRange = GetStepWeight(plMath::Max(m_iCurrentStep, 0)) / m_fSummedWeight;

  out_range = internalRange * m_fPercentageRange;
  out_base = m_fPercentageBase + (internalBase * m_fPercentageRange);

  PLASMA_ASSERT_DEBUG(out_base <= 1.0f, "Invalid range");
  PLASMA_ASSERT_DEBUG(out_range <= 1.0f, "Invalid range");
  PLASMA_ASSERT_DEBUG(out_base + out_range <= 1.0f, "Invalid range");
}

bool plProgressRange::BeginNextStep(plStringView sStepDisplayText, plUInt32 uiNumSteps)
{
  PLASMA_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_sStepDisplayText = sStepDisplayText;

  for (plUInt32 i = 0; i < uiNumSteps; ++i)
  {
    m_fWeightedCompletion += GetStepWeight(m_iCurrentStep + i);
  }
  m_iCurrentStep += uiNumSteps;

  const double internalCompletion = m_fWeightedCompletion / m_fSummedWeight;
  const double finalCompletion = m_fPercentageBase + internalCompletion * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool plProgressRange::SetCompletion(double fCompletionFactor)
{
  PLASMA_ASSERT_DEV(m_fSummedWeight == 0.0, "This function is only supported if ProgressRange was initialized without steps");

  const double finalCompletion = m_fPercentageBase + fCompletionFactor * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool plProgressRange::WasCanceled() const
{
  if (!m_pProgressbar->m_bCancelClicked)
    return false;

  const plProgressRange* pCur = this;

  // if there is any action in the stack above, that cannot be canceled
  // all sub actions should be fully executed, even if they could be canceled
  while (pCur)
  {
    if (!pCur->m_bAllowCancel)
      return false;

    pCur = pCur->m_pParentRange;
  }

  return true;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Progress);
