#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

/// \brief This class holds a simple map that maps strings (keys) to strings (values), which represent certain stats.
///
/// This can be used by a game to store (and continuously update) information about the internal game state. Other tools can then
/// display this information in a convenient manner. For example the stats can be shown on screen. The data is also transmitted through
/// plTelemetry, and the plInspector tool will display the information.
class PLASMA_FOUNDATION_DLL plStats
{
public:
  using MapType = plMap<plString, plVariant>;

  /// \brief Removes the stat with the given name.
  ///
  /// This will also send a 'remove' message through plTelemetry, such that external tools can remove it from their list.
  static void RemoveStat(plStringView sStatName);

  /// \brief Sets the value of the given stat, adds it if it did not exist before.
  ///
  /// szStatName may contain slashes (but not backslashes) to define groups and subgroups, which can be used by tools such as plInspector
  /// to display the stats in a hierarchical way.
  /// This function will also send the name and value of the stat through plTelemetry, such that tools like plInspector will show the
  /// changed value.
  static void SetStat(plStringView sStatName, const plVariant& value);

  /// \brief Returns the value of the given stat. Returns an invalid plVariant, if the stat did not exist before.
  static const plVariant& GetStat(plStringView sStatName) { return s_Stats[sStatName]; }

  /// \brief Returns the entire map of stats, can be used to display them.
  static const MapType& GetAllStats() { return s_Stats; }

  /// \brief The event data that is broadcast whenever a stat is changed.
  struct StatsEventData
  {
    /// \brief Which type of event this is.
    enum EventType
    {
      Add,   ///< A variable has been set for the first time.
      Set,   ///< A variable has been changed.
      Remove ///< A variable that existed has been removed.
    };

    EventType m_EventType;
    plStringView m_sStatName;
    plVariant m_NewStatValue;
  };

  using plEventStats = plEvent<const StatsEventData&, plMutex>;

  /// \brief Adds an event handler that is called every time a stat is changed.
  static void AddEventHandler(plEventStats::Handler handler) { s_StatsEvents.AddEventHandler(handler); }

  /// \brief Removes a previously added event handler.
  static void RemoveEventHandler(plEventStats::Handler handler) { s_StatsEvents.RemoveEventHandler(handler); }

private:
  static plMutex s_Mutex;
  static MapType s_Stats;
  static plEventStats s_StatsEvents;
};
