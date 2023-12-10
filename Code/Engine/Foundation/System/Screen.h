#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Strings/String.h>

/// \brief Describes the properties of a screen
struct PLASMA_FOUNDATION_DLL plScreenInfo
{
  plString m_sDisplayName; ///< Some OS provided name for the screen, typically the manufacturer and model name.

  plInt32 m_iOffsetX;     ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  plInt32 m_iOffsetY;     ///< The virtual position of the screen. Ie. a window created at this location will appear on this screen.
  plInt32 m_iResolutionX; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  plInt32 m_iResolutionY; ///< The virtual resolution. Ie. a window with this dimension will span the entire screen.
  bool m_bIsPrimary;      ///< Whether this is the primary/main screen.
};

/// \brief Provides functionality to detect available monitors
class PLASMA_FOUNDATION_DLL plScreen
{
public:
  /// \brief Enumerates all available screens. When it returns PLASMA_SUCCESS, at least one screen has been found.
  static plResult EnumerateScreens(plHybridArray<plScreenInfo, 2>& out_screens);

  /// \brief Prints the available screen information to the provided log.
  static void PrintScreenInfo(const plHybridArray<plScreenInfo, 2>& screens, plLogInterface* pLog = plLog::GetThreadLocalLogSystem());
};
