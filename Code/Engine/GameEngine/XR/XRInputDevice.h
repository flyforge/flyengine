#pragma once

#include <Core/Input/InputDevice.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/Declarations.h>

#define plInputSlot_XR_Hand_Left_Trigger "xr_hand_left_trigger"
#define plInputSlot_XR_Hand_Left_Select_Click "xr_hand_left_select_click"
#define plInputSlot_XR_Hand_Left_Menu_Click "xr_hand_left_menu_click"
#define plInputSlot_XR_Hand_Left_Squeple_Click "xr_hand_left_squeple_click"

#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_NegX "xr_hand_left_primary_analog_stick_negx"
#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_PosX "xr_hand_left_primary_analog_stick_posx"
#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_NegY "xr_hand_left_primary_analog_stick_negy"
#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_PosY "xr_hand_left_primary_analog_stick_posy"
#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_Click "xr_hand_left_primary_analog_stick_click"
#define plInputSlot_XR_Hand_Left_Primary_Analog_Stick_Touch "xr_hand_left_primary_analog_stick_touch"

#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_NegX "xr_hand_left_secondary_analog_stick_negx"
#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_PosX "xr_hand_left_secondary_analog_stick_posx"
#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_NegY "xr_hand_left_secondary_analog_stick_negy"
#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_PosY "xr_hand_left_secondary_analog_stick_posy"
#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_Click "xr_hand_left_secondary_analog_stick_click"
#define plInputSlot_XR_Hand_Left_Secondary_Analog_Stick_Touch "xr_hand_left_secondary_analog_stick_touch"


#define plInputSlot_XR_Hand_Right_Trigger "xr_hand_right_trigger"
#define plInputSlot_XR_Hand_Right_Select_Click "xr_hand_right_select_click"
#define plInputSlot_XR_Hand_Right_Menu_Click "xr_hand_right_menu_click"
#define plInputSlot_XR_Hand_Right_Squeple_Click "xr_hand_right_squeple_click"

#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_NegX "xr_hand_right_primary_analog_stick_negx"
#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_PosX "xr_hand_right_primary_analog_stick_posx"
#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_NegY "xr_hand_right_primary_analog_stick_negy"
#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_PosY "xr_hand_right_primary_analog_stick_posy"
#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_Click "xr_hand_right_primary_analog_stick_click"
#define plInputSlot_XR_Hand_Right_Primary_Analog_Stick_Touch "xr_hand_right_primary_analog_stick_touch"

#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_NegX "xr_hand_right_secondary_analog_stick_negx"
#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_PosX "xr_hand_right_secondary_analog_stick_posx"
#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_NegY "xr_hand_right_secondary_analog_stick_negy"
#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_PosY "xr_hand_right_secondary_analog_stick_posy"
#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_Click "xr_hand_right_secondary_analog_stick_click"
#define plInputSlot_XR_Hand_Right_Secondary_Analog_Stick_Touch "xr_hand_right_secondary_analog_stick_touch"


class PL_GAMEENGINE_DLL plXRInputDevice : public plInputDevice
{
  PL_ADD_DYNAMIC_REFLECTION(plXRInputDevice, plInputDevice);

public:
  /// \name Devices
  ///@{

  /// \brief Fills out a list of valid (connected) device IDs.
  virtual void GetDeviceList(plHybridArray<plXRDeviceID, 64>& out_devices) const = 0;
  /// \brief Returns the deviceID for a specific type of device.
  /// If the device is not connected, -1 is returned instead.
  virtual plXRDeviceID GetDeviceIDByType(plXRDeviceType::Enum type) const = 0;
  /// \brief Returns the current device state for a valid device ID.
  virtual const plXRDeviceState& GetDeviceState(plXRDeviceID deviceID) const = 0;
  /// \brief Returns the device name for a valid device ID.
  ///
  /// This returns a human readable name to identify the device.
  /// For plXRDeviceType::HMD the name is always 'HMD'.
  /// This can be used for e.g. controllers to create custom game input logic
  /// or mappings if a certain type of controller is used.
  /// Values could be for example:
  /// 'Simple Controller', 'Mixed Reality Motion Controller', 'Hand Interaction' etc.
  virtual plString GetDeviceName(plXRDeviceID deviceID) const = 0;
  /// \brief Returns the device features for a valid device ID.
  virtual plBitflags<plXRDeviceFeatures> GetDeviceFeatures(plXRDeviceID deviceID) const = 0;

  /// \brief Returns the input event. Allows tracking device addition and removal.
  const plXRDeviceEvent& GetInputEvent() { return m_InputEvents; }

  ///@}

protected:
  plXRDeviceEvent m_InputEvents;
};
