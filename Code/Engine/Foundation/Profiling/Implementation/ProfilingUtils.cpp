#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>

plResult plProfilingUtils::SaveProfilingCapture(plStringView sCapturePath)
{
  plFileWriter fileWriter;
  if (fileWriter.Open(sCapturePath) == PLASMA_SUCCESS)
  {
    plProfilingSystem::ProfilingData profilingData;
    plProfilingSystem::Capture(profilingData);
    // Set sort index to -1 so that the editor is always on top when opening the trace.
    profilingData.m_uiProcessSortIndex = -1;
    if (profilingData.Write(fileWriter).Failed())
    {
      plLog::Error("Failed to write profiling capture: {0}.", sCapturePath);
      return PLASMA_FAILURE;
    }

    plLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  }
  else
  {
    plLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

plResult plProfilingUtils::MergeProfilingCaptures(plStringView sCapturePath1, plStringView sCapturePath2, plStringView sMergedCapturePath)
{
  plString sFirstProfilingJson;
  {
    plFileReader reader;
    if (reader.Open(sCapturePath1).Failed())
    {
      plLog::Error("Failed to read first profiling capture to be merged: {}.", sCapturePath1);
      return PLASMA_FAILURE;
    }
    sFirstProfilingJson.ReadAll(reader);
  }
  plString sSecondProfilingJson;
  {
    plFileReader reader;
    if (reader.Open(sCapturePath2).Failed())
    {
      plLog::Error("Failed to read second profiling capture to be merged: {}.", sCapturePath2);
      return PLASMA_FAILURE;
    }
    sSecondProfilingJson.ReadAll(reader);
  }

  plStringBuilder sMergedProfilingJson;
  {
    // Just glue the array together
    sMergedProfilingJson.Reserve(sFirstProfilingJson.GetElementCount() + 1 + sSecondProfilingJson.GetElementCount());
    const char* szEndArray = sFirstProfilingJson.FindLastSubString("]");
    sMergedProfilingJson.Append(plStringView(sFirstProfilingJson.GetData(), static_cast<plUInt32>(szEndArray - sFirstProfilingJson.GetData())));
    sMergedProfilingJson.Append(",");
    const char* szStartArray = sSecondProfilingJson.FindSubString("[") + 1;
    sMergedProfilingJson.Append(plStringView(szStartArray, static_cast<plUInt32>(sSecondProfilingJson.GetElementCount() - (szStartArray - sSecondProfilingJson.GetData()))));
  }

  plFileWriter fileWriter;
  if (fileWriter.Open(sMergedCapturePath).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
  {
    plLog::Error("Failed to write merged profiling capture: {}.", sMergedCapturePath);
    return PLASMA_FAILURE;
  }
  plLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  return PLASMA_SUCCESS;
}
