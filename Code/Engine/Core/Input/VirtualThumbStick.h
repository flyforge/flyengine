#pragma once

#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Math/Vec2.h>

/// \brief A Virtual Thumb-stick is an input device that transforms certain types of input (mouse / touch) into input similar to a
/// thumb-stick on a controller.
///
/// A virtual thumb-stick can be used to provide an 'input device' on a touch screen, that acts like a controller thumb-stick and thus
/// allows easier control over a game. The virtual thumb-stick takes input inside a certain screen area. It tracks the users finger
/// movements inside this area and translates those into input from a controller thumb-stick, which it then feeds back into the input
/// system. That makes it then possible to be mapped to input actions again. This way a game controller type of input is emulated.
class PL_CORE_DLL plVirtualThumbStick final : public plInputDevice
{
  PL_ADD_DYNAMIC_REFLECTION(plVirtualThumbStick, plInputDevice);

public:
  /// \brief Constructor.
  plVirtualThumbStick();

  /// \brief Destructor.
  ~plVirtualThumbStick();

  /// \brief This enum allows to select either some default input mapping or to select 'Custom'.
  struct Input
  {
    enum Enum
    {
      Touchpoint,    ///< The Virtual Thumb-stick will be triggered by touch input events.
      MousePosition, ///< The Virtual Thumb-stick will be triggered by mouse input.
      Custom         ///< The Thumb-stick triggers are specified manually.
    };
  };

  /// \brief Specifies which type of output the thumb-stick shall generate.
  struct Output
  {
    enum Enum
    {
      Controller0_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 0.
      Controller0_RightStick, ///< The Thumb-stick acts like the right stick of controller 0.
      Controller1_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 1.
      Controller1_RightStick, ///< The Thumb-stick acts like the right stick of controller 1.
      Controller2_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 2.
      Controller2_RightStick, ///< The Thumb-stick acts like the right stick of controller 2.
      Controller3_LeftStick,  ///< The Thumb-stick acts like the left stick of controller 3.
      Controller3_RightStick, ///< The Thumb-stick acts like the right stick of controller 3.
      Custom                  ///< The thumb-stick output is specified manually.
    };
  };

  /// \brief Defines whether the thumb-stick center position is locked or relative to where the user started touching it.
  struct CenterMode
  {
    enum Enum
    {
      InputArea,      ///< The center of the thumb-stick is always at the center of the input area.
      ActivationPoint ///< The center of the thumb-stick is always where the user activates the thumb-stick (first touch-point)
    };
  };

  /// \brief Defines the area on screen where the thumb-stick is located and accepts input.
  ///
  /// \param vLowerLeft
  ///   The lower left corner of the input area. Coordinates are in [0; 1] range.
  ///
  /// \param vUpperRight
  ///   The upper right corner of the input area. Coordinates are in [0; 1] range.
  ///
  /// \param fPriority
  ///   The priority of the input area. Defines which thumb-stick or other input action gets priority, if they overlap.
  ///
  /// \param center
  ///   \sa CenterMode.
  void SetInputArea(const plVec2& vLowerLeft, const plVec2& vUpperRight, float fThumbstickRadius, float fPriority,
    CenterMode::Enum center = CenterMode::ActivationPoint);

  /// \brief Returns the input area of the virtual thumb-stick.
  void GetInputArea(plVec2& out_vLowerLeft, plVec2& out_vUpperRight);

  /// \brief Specifies from which input slots the thumb-stick is activated.
  ///
  /// If \a Input is 'Custom' the remaining parameters define the filter axes and up to three input slots that trigger the thumb-stick.
  /// Otherwise the remaining parameters are ignored.
  void SetTriggerInputSlot(Input::Enum input, const plInputActionConfig* pCustomConfig = nullptr);

  /// \brief Specifies which output the thumb-stick generates.
  ///
  /// If \a Output is 'Custom' the remaining parameters define which input slots the thumb-stick triggers for which direction.
  /// Otherwise the remaining parameters are ignored.
  void SetThumbstickOutput(Output::Enum output, plStringView sOutputLeft = {}, plStringView sOutputRight = {}, plStringView sOutputUp = {}, plStringView sOutputDown = {});

  /// \brief Specifies what happens when the input slots that trigger the thumb-stick are active while entering or leaving the input area.
  void SetAreaFocusMode(plInputActionConfig::OnEnterArea onEnter, plInputActionConfig::OnLeaveArea onLeave);

  /// \brief Allows to enable or disable the entire thumb-stick temporarily.
  void SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }

  /// \brief Returns whether the thumb-stick is currently enabled.
  bool IsEnabled() const { return m_bEnabled; }

  /// \brief Returns whether the thumb-stick is currently active (ie. triggered) and generates output.
  bool IsActive() const { return m_bIsActive; }

protected:
  void UpdateActionMapping();

  plVec2 m_vLowerLeft;
  plVec2 m_vUpperRight;
  float m_fRadius;

  plInputActionConfig m_ActionConfig;
  plStringView m_sOutputLeft;
  plStringView m_sOutputRight;
  plStringView m_sOutputUp;
  plStringView m_sOutputDown;

  bool m_bEnabled;
  bool m_bConfigChanged;
  bool m_bIsActive;
  plString m_sName;
  plVec2 m_vCenter;
  CenterMode::Enum m_CenterMode;

  static plInt32 s_iThumbsticks;

private:
  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
};
