#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

static const plImageFormat::Enum defaultFormat = plImageFormat::R32G32B32A32_FLOAT;

class plImageConversionTest : public plTestBaseClass
{

public:
  virtual const char* GetTestName() const override { return "Image Conversion"; }

  virtual plResult GetImage(plImage& ref_img) override
  {
    ref_img.ResetAndMove(std::move(m_Image));
    return PLASMA_SUCCESS;
  }

private:
  virtual void SetupSubTests() override
  {
    for (plUInt32 i = 0; i < plImageFormat::NUM_FORMATS; ++i)
    {
      plImageFormat::Enum format = static_cast<plImageFormat::Enum>(i);

      const char* name = plImageFormat::GetName(format);
      PLASMA_ASSERT_DEV(name != nullptr, "Missing format information for format {}", i);

      bool isEncodable = plImageConversion::IsConvertible(defaultFormat, format);

      if (!isEncodable)
      {
        // If a format doesn't have an encoder, ignore
        continue;
      }

      AddSubTest(name, i);
    }
  }

  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override
  {
    plImageFormat::Enum format = static_cast<plImageFormat::Enum>(iIdentifier);

    bool isDecodable = plImageConversion::IsConvertible(format, defaultFormat);

    if (!isDecodable)
    {
      PLASMA_TEST_BOOL_MSG(false, "Format %s can be encoded from %s but not decoded - add a decoder for this format please", plImageFormat::GetName(format), plImageFormat::GetName(defaultFormat));

      return plTestAppRun::Quit;
    }

    {
      plHybridArray<plImageConversion::ConversionPathNode, 16> decodingPath;
      plUInt32 decodingPathScratchBuffers;
      plImageConversion::BuildPath(format, defaultFormat, false, decodingPath, decodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      plLog::Info("[test]Default decoding Path:");
      for (plUInt32 i = 0; i < decodingPath.GetCount(); ++i)
      {
        plLog::Info("[test]  {} -> {}", plImageFormat::GetName(decodingPath[i].m_sourceFormat), plImageFormat::GetName(decodingPath[i].m_targetFormat));
      }
    }

    {
      plHybridArray<plImageConversion::ConversionPathNode, 16> encodingPath;
      plUInt32 encodingPathScratchBuffers;
      plImageConversion::BuildPath(defaultFormat, format, false, encodingPath, encodingPathScratchBuffers).IgnoreResult();

      // the [test] tag tells the test framework to output the log message in the GUI
      plLog::Info("[test]Default encoding Path:");
      for (plUInt32 i = 0; i < encodingPath.GetCount(); ++i)
      {
        plLog::Info("[test]  {} -> {}", plImageFormat::GetName(encodingPath[i].m_sourceFormat), plImageFormat::GetName(encodingPath[i].m_targetFormat));
      }
    }

    // Test LDR: Load, encode to target format, then do image comparison (which internally decodes to BGR8_UNORM again).
    // This visualizes quantization for low bit formats, block compression artifacts, or whether formats have fewer than 3 channels.
    {
      PLASMA_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      PLASMA_TEST_BOOL(m_Image.Convert(format).Succeeded());

      PLASMA_TEST_IMAGE(iIdentifier * 2, plImageFormat::IsCompressed(format) ? 10 : 0);
    }

    // Test HDR: Load, decode to FLOAT32, stretch to [-range, range] and encode;
    // then decode to FLOAT32 again, bring back into LDR range and do image comparison.
    // If the format doesn't support negative values, the left half of the image will be black.
    // If the format doesn't support values with absolute value > 1, the image will appear clipped to fullbright.
    // Also, fill the first few rows in the top left with Infinity, -Infinity, and NaN, which should
    // show up as White, White, and Black, resp., in the comparison.
    {
      const float range = 8;

      PLASMA_TEST_BOOL(m_Image.LoadFrom("ImageConversions/reference.png").Succeeded());

      PLASMA_TEST_BOOL(m_Image.Convert(plImageFormat::R32G32B32A32_FLOAT).Succeeded());

      float posInf = +plMath::Infinity<float>();
      float negInf = -plMath::Infinity<float>();
      float NaN = plMath::NaN<float>();

      for (plUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        plColor* pPixelPointer = m_Image.GetPixelPointer<plColor>(0, 0, 0, 0, y);

        for (plUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Fill with Inf or Nan resp. scale the image into positive and negative HDR range
          if (x < 30 && y < 10)
          {
            *pPixelPointer = plColor(posInf, posInf, posInf, posInf);
          }
          else if (x < 30 && y < 20)
          {
            *pPixelPointer = plColor(negInf, negInf, negInf, negInf);
          }
          else if (x < 30 && y < 30)
          {
            *pPixelPointer = plColor(NaN, NaN, NaN, NaN);
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;

            if (plMath::Abs(scale) > 0.5)
            {
              *pPixelPointer *= scale;
            }
          }

          pPixelPointer++;
        }
      }

      PLASMA_TEST_BOOL(m_Image.Convert(format).Succeeded());

      PLASMA_TEST_BOOL(m_Image.Convert(plImageFormat::R32G32B32A32_FLOAT).Succeeded());

      for (plUInt32 y = 0; y < m_Image.GetHeight(); ++y)
      {
        plColor* pPixelPointer = m_Image.GetPixelPointer<plColor>(0, 0, 0, 0, y);

        for (plUInt32 x = 0; x < m_Image.GetWidth(); ++x)
        {
          // Scale the image back into LDR range if possible
          if (x < 30 && y < 10)
          {
            // Leave pos inf as is - this should be clipped to 1 in the LDR conversion for img cmp
          }
          else if (x < 30 && y < 20)
          {
            // Flip neg inf to pos inf
            *pPixelPointer *= -1.0f;
          }
          else if (x < 30 && y < 30)
          {
            // Leave nan as is - this should be clipped to 0 in the LDR conversion for img cmp
          }
          else
          {
            float scale = (x / float(m_Image.GetWidth()) - 0.5f) * 2.0f * range;
            if (plMath::Abs(scale) > 0.5)
            {
              *pPixelPointer /= scale;
            }
          }

          pPixelPointer++;
        }
      }

      PLASMA_TEST_IMAGE(iIdentifier * 2 + 1, plImageFormat::IsCompressed(format) ? 10 : 0);
    }

    return plTestAppRun::Quit;
  }

  virtual plResult InitializeTest() override
  {
    plStartup::StartupCoreSystems();

    const plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());

    if (plFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageConversionTest").Failed())
    {
      return PLASMA_FAILURE;
    }

    plFileSystem::AddDataDirectory(">pltest/", "ImageComparisonDataDir", "imgout", plFileSystem::AllowWrites).IgnoreResult();

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
    // On linux we use CPU based BC6 and BC7 compression, which sometimes gives slightly different results from the GPU compression on Windows.
    plTestFramework::GetInstance()->SetImageReferenceOverrideFolderName("Images_Reference_Linux");
#endif

    return PLASMA_SUCCESS;
  }

  virtual plResult DeInitializeTest() override
  {
    plFileSystem::RemoveDataDirectoryGroup("ImageConversionTest");
    plFileSystem::RemoveDataDirectoryGroup("ImageComparisonDataDir");

    plStartup::ShutdownCoreSystems();
    plMemoryTracker::DumpMemoryLeaks();

    return PLASMA_SUCCESS;
  }

  virtual plResult InitializeSubTest(plInt32 iIdentifier) override { return PLASMA_SUCCESS; }

  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override { return PLASMA_SUCCESS; }

  plImage m_Image;
};

static plImageConversionTest s_ImageConversionTest;
