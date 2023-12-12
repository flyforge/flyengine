#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plEventTrack;

class PLASMA_GUIFOUNDATION_DLL plEventTrackControlPointData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEventTrackControlPointData, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::Seconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 fps);
  const char* GetEventName() const { return m_sEvent.GetData(); }
  void SetEventName(const char* sz) { m_sEvent.Assign(sz); }

  plInt64 m_iTick; // 4800 ticks per second
  plHashedString m_sEvent;
};

class PLASMA_GUIFOUNDATION_DLL plEventTrackData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEventTrackData, plReflectedClass);

public:
  plInt64 TickFromTime(plTime time) const;
  void ConvertToRuntimeData(plEventTrack& out_Result) const;

  plUInt16 m_uiFramesPerSecond = 60;
  plDynamicArray<plEventTrackControlPointData> m_ControlPoints;
};

class PLASMA_GUIFOUNDATION_DLL plEventSet
{
public:
  bool IsModified() const { return m_bModified; }

  const plSet<plString>& GetAvailableEvents() const { return m_AvailableEvents; }

  void AddAvailableEvent(const char* szEvent);

  plResult WriteToDDL(const char* szFile);
  plResult ReadFromDDL(const char* szFile);

private:
  bool m_bModified = false;
  plSet<plString> m_AvailableEvents;
};
