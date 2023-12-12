#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/ProcessGroup.h>
#include <Texture/Image/Image.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_PROCESSES) && (PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)) && defined(BUILDSYSTEM_TEXCONV_PRESENT)

class plTexConvTest : public plTestBaseClass
{
public:
  virtual const char* GetTestName() const override { return "TexConvTool"; }

  virtual plResult GetImage(plImage& ref_img) override
  {
    ref_img.ResetAndMove(std::move(m_pState->m_image));
    return PLASMA_SUCCESS;
  }

private:
  enum SubTest
  {
    RgbaToRgbPNG,
    Combine4,
    LinearUsage,
    ExtractChannel,
    TGA,
  };

  virtual void SetupSubTests() override;

  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  virtual plResult InitializeTest() override
  {
    plStartup::StartupCoreSystems();

    m_pState = PLASMA_DEFAULT_NEW(State);

    const plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());

    if (plFileSystem::AddDataDirectory(sReadDir.GetData(), "TexConvTest", "testdata").Failed())
    {
      return PLASMA_FAILURE;
    }

    plFileSystem::AddDataDirectory(">pltest/", "TexConvDataDir", "imgout", plFileSystem::AllowWrites).IgnoreResult();

    return PLASMA_SUCCESS;
  }

  virtual plResult DeInitializeTest() override
  {
    m_pState.Clear();

    plFileSystem::RemoveDataDirectoryGroup("TexConvTest");
    plFileSystem::RemoveDataDirectoryGroup("TexConvDataDir");

    plStartup::ShutdownCoreSystems();

    return PLASMA_SUCCESS;
  }

  void RunTexConv(plProcessOptions& options, const char* szOutName)
  {
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    const char* szTexConvExecutableName = "TexConv.exe";
#  else
    const char* szTexConvExecutableName = "TexConv";
#  endif
    plStringBuilder sTexConvExe = plOSFile::GetApplicationDirectory();
    sTexConvExe.AppendPath(szTexConvExecutableName);
    sTexConvExe.MakeCleanPath();

    if (!PLASMA_TEST_BOOL_MSG(plOSFile::ExistsFile(sTexConvExe), "%s does not exist", szTexConvExecutableName))
      return;

    options.m_sProcess = sTexConvExe;

    plStringBuilder sOut = plTestFramework::GetInstance()->GetAbsOutputPath();
    sOut.AppendPath("Temp", szOutName);

    options.AddArgument("-out");
    options.AddArgument(sOut);

    if (!PLASMA_TEST_BOOL(m_pState->m_TexConvGroup.Launch(options).Succeeded()))
      return;

    if (!PLASMA_TEST_BOOL_MSG(m_pState->m_TexConvGroup.WaitToFinish(plTime::Minutes(1.0)).Succeeded(), "TexConv did not finish in time."))
      return;

    PLASMA_TEST_INT_MSG(m_pState->m_TexConvGroup.GetProcesses().PeekBack().GetExitCode(), 0, "TexConv failed to process the image");

    m_pState->m_image.LoadFrom(sOut).IgnoreResult();
  }

  struct State
  {
    plProcessGroup m_TexConvGroup;
    plImage m_image;
  };

  plUniquePtr<State> m_pState;
};

void plTexConvTest::SetupSubTests()
{
  AddSubTest("RGBA to RGB - PNG", SubTest::RgbaToRgbPNG);
  AddSubTest("Combine4 - DDS", SubTest::Combine4);
  AddSubTest("Linear Usage", SubTest::LinearUsage);
  AddSubTest("Extract Channel", SubTest::ExtractChannel);
  AddSubTest("TGA loading", SubTest::TGA);
}

plTestAppRun plTexConvTest::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  plStringBuilder sImageData;
  plFileSystem::ResolvePath(":testdata/TexConv", &sImageData, nullptr).IgnoreResult();

  const plStringBuilder sPathPLASMA(sImageData, "/PLASMA.png");
  const plStringBuilder sPathE(sImageData, "/E.png");
  const plStringBuilder sPathZ(sImageData, "/Z.png");
  const plStringBuilder sPathShape(sImageData, "/Shape.png");
  const plStringBuilder sPathTGAv(sImageData, "/PLASMA_flipped_v.tga");
  const plStringBuilder sPathTGAh(sImageData, "/PLASMA_flipped_h.tga");
  const plStringBuilder sPathTGAvhCompressed(sImageData, "/PLASMA_flipped_vh.tga");

  if (iIdentifier == SubTest::RgbaToRgbPNG)
  {
    plProcessOptions opt;
    opt.AddArgument("-rgb");
    opt.AddArgument("in0");

    opt.AddArgument("-in0");
    opt.AddArgument(sPathPLASMA);

    RunTexConv(opt, "RgbaToRgbPNG.png");

    PLASMA_TEST_IMAGE(0, 10);
  }

  if (iIdentifier == SubTest::Combine4)
  {
    plProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathPLASMA);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in1.r");

    opt.AddArgument("-g");
    opt.AddArgument("in0.r");

    opt.AddArgument("-b");
    opt.AddArgument("in2.r");

    opt.AddArgument("-a");
    opt.AddArgument("in3.r");

    opt.AddArgument("-type");
    opt.AddArgument("2D");

    opt.AddArgument("-compression");
    opt.AddArgument("medium");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("linear");

    opt.AddArgument("-usage");
    opt.AddArgument("color");

    RunTexConv(opt, "Combine4.dds");

    // Threshold needs to be higher here since we might fall back to software dxt compression
    // which results in slightly different results than GPU dxt compression.
    PLASMA_TEST_IMAGE(1, 100);
  }

  if (iIdentifier == SubTest::LinearUsage)
  {
    plProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathE);

    opt.AddArgument("-in1");
    opt.AddArgument(sPathZ);

    opt.AddArgument("-in2");
    opt.AddArgument(sPathPLASMA);

    opt.AddArgument("-in3");
    opt.AddArgument(sPathShape);

    opt.AddArgument("-r");
    opt.AddArgument("in3");

    opt.AddArgument("-g");
    opt.AddArgument("in0");

    opt.AddArgument("-b");
    opt.AddArgument("in2");

    opt.AddArgument("-compression");
    opt.AddArgument("high");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("kaiser");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-downscale");
    opt.AddArgument("1");

    RunTexConv(opt, "Linear.dds");

    PLASMA_TEST_IMAGE(2, 10);
  }

  if (iIdentifier == SubTest::ExtractChannel)
  {
    plProcessOptions opt;
    opt.AddArgument("-in0");
    opt.AddArgument(sPathPLASMA);

    opt.AddArgument("-r");
    opt.AddArgument("in0.r");

    opt.AddArgument("-compression");
    opt.AddArgument("none");

    opt.AddArgument("-mipmaps");
    opt.AddArgument("none");

    opt.AddArgument("-usage");
    opt.AddArgument("linear");

    opt.AddArgument("-maxRes");
    opt.AddArgument("64");

    RunTexConv(opt, "ExtractChannel.dds");

    PLASMA_TEST_IMAGE(3, 10);
  }

  if (iIdentifier == SubTest::TGA)
  {
    {
      plProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAv);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "PLASMA_flipped_v.dds");

      PLASMA_TEST_IMAGE(3, 10);
    }

    {
      plProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAh);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "PLASMA_flipped_h.dds");

      PLASMA_TEST_IMAGE(4, 10);
    }

    {
      plProcessOptions opt;
      opt.AddArgument("-in0");
      opt.AddArgument(sPathTGAvhCompressed);

      opt.AddArgument("-rgba");
      opt.AddArgument("in0");

      opt.AddArgument("-usage");
      opt.AddArgument("linear");

      RunTexConv(opt, "PLASMA_flipped_vh.dds");

      PLASMA_TEST_IMAGE(5, 10);
    }
  }

  return plTestAppRun::Quit;
}



static plTexConvTest s_plTexConvTest;

#endif
