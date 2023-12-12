#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  void WriteOutProfilingCapture(const char* szFilePath)
  {
    plStringBuilder outputPath = plTestFramework::GetInstance()->GetAbsOutputPath();
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

    plFileWriter fileWriter;
    if (fileWriter.Open(szFilePath) == PLASMA_SUCCESS)
    {
      plProfilingSystem::ProfilingData profilingData;
      plProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      plLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
} // namespace

PLASMA_CREATE_SIMPLE_TEST_GROUP(Profiling);

PLASMA_CREATE_SIMPLE_TEST(Profiling, Profiling)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Nested scopes")
  {
    plProfilingSystem::Clear();

    {
      PLASMA_PROFILE_SCOPE("Prewarm scope");
      plThreadUtils::Sleep(plTime::Milliseconds(1));
    }

    plTime endTime = plTime::Now() + plTime::Milliseconds(1);

    {
      PLASMA_PROFILE_SCOPE("Outer scope");

      {
        PLASMA_PROFILE_SCOPE("Inner scope");

        while (plTime::Now() < endTime)
        {
        }
      }
    }

    WriteOutProfilingCapture(":output/profilingScopes.json");
  }
}
