#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plEventTrack;

class PL_GUIFOUNDATION_DLL plEventTrackControlPointData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plEventTrackControlPointData, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 iFps);
  const char* GetEventName() const { return m_sEvent.GetData(); }
  void SetEventName(const char* szSz) { m_sEvent.Assign(szSz); }

  plInt64 m_iTick; // 4800 ticks per second
  plHashedString m_sEvent;
};

class PL_GUIFOUNDATION_DLL plEventTrackData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plEventTrackData, plReflectedClass);

public:
  plInt64 TickFromTime(plTime time) const;
  void ConvertToRuntimeData(plEventTrack& out_result) const;

  plUInt16 m_uiFramesPerSecond = 60;
  plDynamicArray<plEventTrackControlPointData> m_ControlPoints;
};

class PL_GUIFOUNDATION_DLL plEventSet
{
public:
  bool IsModified() const { return m_bModified; }

  const plSet<plString>& GetAvailableEvents() const { return m_AvailableEvents; }

  void AddAvailableEvent(plStringView sEvent);

  plResult WriteToDDL(const char* szFile);
  plResult ReadFromDDL(const char* szFile);

private:
  bool m_bModified = false;
  plSet<plString> m_AvailableEvents;
};
