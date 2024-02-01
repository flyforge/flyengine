#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>

class plProgress;
class plProgressRange;

/// \brief Through these events the state of an plProgress instance is communicated.
///
/// Other code can use this to visualize the progress in different ways.
/// For instance a GUI application can show a progress bar dialog and a game
/// could show a loading screen.
struct PL_FOUNDATION_DLL plProgressEvent
{
  enum class Type
  {
    ProgressStarted, ///< Sent when the the first progress starts
    ProgressEnded,   ///< Sent when progress finishes or is canceled
    ProgressChanged, ///< Sent whenever the progress value changes. Not necessarily in every update step.
    CancelClicked,   ///< The user just clicked cancel (for the first time).
  };

  Type m_Type;
  plProgress* m_pProgressbar;
};

/// \brief Manages the way a progress bar is subdivided and advanced.
///
/// plProgress represents a single progress bar. It can be sub-divided into groups and sub-groups using plProgressbarRange.
/// From the ranges and the current advancement, a final progress percentage is computed. Every time a significant change
/// takes place, an event is broadcast. This allows other code to display the progress, either in a GUI application
/// or in a fullscreen loading screen or in any other way appropriate.
class PL_FOUNDATION_DLL plProgress
{
public:
  plProgress();
  ~plProgress();

  /// \brief Returns the current overall progress in [0; 1] range.
  float GetCompletion() const;

  /// \brief Sets the current overall progress in [0; 1] range. Should not be called directly, typically called by plProgreesRange.
  void SetCompletion(float fCompletion);

  /// \brief Returns the current 'headline' text for the progress bar
  plStringView GetMainDisplayText() const;

  /// \brief Returns the current detail text for the progress bar
  plStringView GetStepDisplayText() const;

  /// \brief Used to inform plProgress of outside user input. May have an effect or not.
  void UserClickedCancel();

  /// \brief Whether the user requested to cancel the operation.
  bool WasCanceled() const;

  /// \brief Returns whether the current operations may be canceled or not.
  bool AllowUserCancel() const;

  /// \brief Returns the currently set default plProgress instance. This will always be valid.
  static plProgress* GetGlobalProgressbar();

  /// \brief Allows to set a custom plProgress instance as the global default instance.
  static void SetGlobalProgressbar(plProgress* pProgress);

  /// \brief Events are sent when the progress changes
  plEvent<const plProgressEvent&> m_Events;

  /// \brief Custom user data.
  void* m_pUserData = nullptr;

private:
  friend class plProgressRange;
  void SetActiveRange(plProgressRange* pRange);

  plProgressRange* m_pActiveRange = nullptr;

  plString m_sCurrentDisplayText;
  bool m_bCancelClicked = false;
  bool m_bEnableCancel = true;

  float m_fLastReportedCompletion = 0.0f;
  float m_fCurrentCompletion = 0.0f;
};

/// \brief plProgressRange is the preferred method to inform the system of progress.
///
/// plProgressRange is a scoped class, ie. upon creation it adds a range to the current progress
/// and upon destruction the entire range is considered to be completed.
/// Ranges can be nested. For instance when a top level range consists of three 'steps',
/// then opening a nested range will sub-divide that first step. When the nested range is closed,
/// the first top-level step is finished and BeginNextStep() should be called on the top-level range.
/// Subsequently the second step is active and can again be further subdivided with another nested plProgressRange.
class PL_FOUNDATION_DLL plProgressRange
{
  PL_DISALLOW_COPY_AND_ASSIGN(plProgressRange);

public:
  /// \brief Creates a progress range scope.
  ///
  /// If any other progress range is currently active, it will become the parent range and the currently active step will be subdivided.
  /// \param szDisplayText is the main display text for this range.
  /// \param uiSteps is the number of steps that this range will be subdivided into
  /// \param bAllowCancel specifies whether the user can cancel this operation
  /// \param pProgressbar can be specified, if available, otherwise the currently active plProgress instance is used.
  plProgressRange(plStringView sDisplayText, plUInt32 uiSteps, bool bAllowCancel, plProgress* pProgressbar = nullptr);

  /// \brief Creates a progress range scope without steps. Use SetCompletion to manually set the completion value.
  plProgressRange(plStringView sDisplayText, bool bAllowCancel, plProgress* pProgressbar = nullptr);

  /// \brief The destructor closes the current range. All progress in this range is assumed to have completed,
  /// even if BeginNextStep() has not been called once for every subdivision step.
  ~plProgressRange();

  /// \brief Returns the plProgress instance that this range uses.
  plProgress* GetProgressbar() const;

  /// \brief Allows to weigh each step differently.
  ///
  /// This makes it possible to divide an operation into two steps, but have one part take up 90% and the other 10%.
  /// \param uiStep The index for the step to set the weight
  /// \param fWeight The weighting in [0; 1] range
  void SetStepWeighting(plUInt32 uiStep, float fWeight);

  /// \brief Should be called whenever a new sub-step is started to advance the progress.
  ///
  /// \param szStepDisplayText The sub-text for the next step to be displayed.
  /// \param uiNumSteps How many steps have been completed.
  /// \return Returns false if the user clicked cancel.
  bool BeginNextStep(plStringView sStepDisplayText, plUInt32 uiNumSteps = 1);

  /// \brief Manually set the completion value between 0..1.
  bool SetCompletion(double fCompletionFactor);

  /// \brief Whether the user requested to cancel the operation.
  bool WasCanceled() const;

private:
  friend class plProgress;

  void Init(plStringView sDisplayText, bool bAllowCancel, plProgress* pProgressbar);
  float GetStepWeight(plUInt32 uiStep) const;
  void ComputeCurStepBaseAndRange(double& out_base, double& out_range);

  plProgressRange* m_pParentRange = nullptr;
  plProgress* m_pProgressbar = nullptr;

  plInt32 m_iCurrentStep = 0;
  plString m_sDisplayText;
  plString m_sStepDisplayText;
  plHashTable<plUInt32, float> m_StepWeights;

  bool m_bAllowCancel = false;
  double m_fPercentageBase = 0.0;
  double m_fPercentageRange = 0.0;
  double m_fWeightedCompletion = 0.0;
  double m_fSummedWeight = 0.0;
};
