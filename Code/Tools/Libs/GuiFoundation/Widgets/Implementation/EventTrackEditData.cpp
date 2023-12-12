#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Tracks/EventTrack.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventTrackControlPointData, 1, plRTTIDefaultAllocator<plEventTrackControlPointData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Tick", m_iTick),
    PLASMA_ACCESSOR_PROPERTY("Event", GetEventName, SetEventName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventTrackData, 3, plRTTIDefaultAllocator<plEventTrackData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plEventTrackControlPointData::SetTickFromTime(plTime time, plInt64 fps)
{
  const plUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

plInt64 plEventTrackData::TickFromTime(plTime time) const
{
  const plUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void plEventTrackData::ConvertToRuntimeData(plEventTrack& out_Result) const
{
  out_Result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    out_Result.AddControlPoint(cp.GetTickAsTime(), cp.m_sEvent);
  }
}

void plEventSet::AddAvailableEvent(const char* szEvent)
{
  if (plStringUtils::IsNullOrEmpty(szEvent))
    return;

  if (m_AvailableEvents.Contains(szEvent))
    return;

  m_bModified = true;
  m_AvailableEvents.Insert(szEvent);
}

plResult plEventSet::WriteToDDL(const char* szFile)
{
  plDeferredFileWriter file;
  file.SetOutput(szFile);

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (const auto& s : m_AvailableEvents)
  {
    ddl.BeginObject("Event", s.GetData());
    ddl.EndObject();
  }

  if (file.Close().Succeeded())
  {
    m_bModified = false;
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plEventSet::ReadFromDDL(const char* szFile)
{
  m_AvailableEvents.Clear();

  plFileReader file;
  if (file.Open(szFile).Failed())
    return PLASMA_FAILURE;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return PLASMA_FAILURE;

  auto* pRoot = ddl.GetRootElement();

  plStringBuilder tmp;

  for (auto* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Event"))
    {
      AddAvailableEvent(pChild->GetName().GetData(tmp));
    }
  }

  m_bModified = false;
  return PLASMA_SUCCESS;
}
