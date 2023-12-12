#include <FoundationTest/FoundationTestPCH.h>


#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Image);

PLASMA_CREATE_SIMPLE_TEST(Image, Image)
{
  const plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
  const plStringBuilder sWriteDir = plTestFramework::GetInstance()->GetAbsOutputPath();

  PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(sWriteDir) == PLASMA_SUCCESS);

  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sReadDir, "ImageTest") == PLASMA_SUCCESS);
  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sWriteDir, "ImageTest", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "BMP - Good")
  {
    const char* testImagesGood[] = {
      "BMPTestImages/good/pal1", "BMPTestImages/good/pal1bg", "BMPTestImages/good/pal1wb", "BMPTestImages/good/pal4", "BMPTestImages/good/pal4rle",
      "BMPTestImages/good/pal8", "BMPTestImages/good/pal8-0", "BMPTestImages/good/pal8nonsquare",
      /*"BMPTestImages/good/pal8os2",*/ "BMPTestImages/good/pal8rle",
      /*"BMPTestImages/good/pal8topdown",*/ "BMPTestImages/good/pal8v4", "BMPTestImages/good/pal8v5", "BMPTestImages/good/pal8w124",
      "BMPTestImages/good/pal8w125", "BMPTestImages/good/pal8w126", "BMPTestImages/good/rgb16", "BMPTestImages/good/rgb16-565pal",
      "BMPTestImages/good/rgb24", "BMPTestImages/good/rgb24pal", "BMPTestImages/good/rgb32", /*"BMPTestImages/good/rgb32bf"*/
    };

    for (int i = 0; i < PLASMA_ARRAY_SIZE(testImagesGood); i++)
    {
      plImage image;
      {
        plStringBuilder fileName;
        fileName.Format("{0}.bmp", testImagesGood[i]);

        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(image.LoadFrom(fileName) == PLASMA_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        plStringBuilder fileName;
        fileName.Format(":output/{0}_out.bmp", testImagesGood[i]);

        PLASMA_TEST_BOOL_MSG(image.SaveTo(fileName) == PLASMA_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "BMP - Bad")
  {
    const char* testImagesBad[] = {"BMPTestImages/bad/badbitcount", "BMPTestImages/bad/badbitssize",
      /*"BMPTestImages/bad/baddens1", "BMPTestImages/bad/baddens2", "BMPTestImages/bad/badfilesize", "BMPTestImages/bad/badheadersize",*/
      "BMPTestImages/bad/badpalettesize",
      /*"BMPTestImages/bad/badplanes",*/ "BMPTestImages/bad/badrle", "BMPTestImages/bad/badwidth",
      /*"BMPTestImages/bad/pal2",*/ "BMPTestImages/bad/pal8badindex", "BMPTestImages/bad/reallybig", "BMPTestImages/bad/rletopdown",
      "BMPTestImages/bad/shortfile"};


    for (int i = 0; i < PLASMA_ARRAY_SIZE(testImagesBad); i++)
    {
      plImage image;
      {
        plStringBuilder fileName;
        fileName.Format("{0}.bmp", testImagesBad[i]);

        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "File does not exist: '%s'", fileName.GetData());

        PLASMA_LOG_BLOCK_MUTE();
        PLASMA_TEST_BOOL_MSG(image.LoadFrom(fileName) == PLASMA_FAILURE, "Reading image should have failed: '%s'", fileName.GetData());
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TGA")
  {
    const char* testImagesGood[] = {"TGATestImages/good/RGB", "TGATestImages/good/RGBA", "TGATestImages/good/RGB_RLE", "TGATestImages/good/RGBA_RLE"};

    for (int i = 0; i < PLASMA_ARRAY_SIZE(testImagesGood); i++)
    {
      plImage image;
      {
        plStringBuilder fileName;
        fileName.Format("{0}.tga", testImagesGood[i]);

        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(image.LoadFrom(fileName) == PLASMA_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        plStringBuilder fileName;
        fileName.Format(":output/{0}_out.bmp", testImagesGood[i]);

        plStringBuilder fileNameExpected;
        fileNameExpected.Format("{0}_expected.bmp", testImagesGood[i]);

        PLASMA_TEST_BOOL_MSG(image.SaveTo(fileName) == PLASMA_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        PLASMA_TEST_FILES(fileName, fileNameExpected, "");
      }

      {
        plStringBuilder fileName;
        fileName.Format(":output/{0}_out.tga", testImagesGood[i]);

        plStringBuilder fileNameExpected;
        fileNameExpected.Format("{0}_expected.tga", testImagesGood[i]);

        PLASMA_TEST_BOOL_MSG(image.SaveTo(fileName) == PLASMA_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        PLASMA_TEST_FILES(fileName, fileNameExpected, "");
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Write Image Formats")
  {
    struct ImgTest
    {
      const char* szImage;
      const char* szFormat;
      plUInt32 uiMSE;
    };

    ImgTest imgTests[] = {
      {"RGB", "tga", 0},
      {"RGBA", "tga", 0},
      {"RGB", "png", 0},
      {"RGBA", "png", 0},
      {"RGB", "jpg", 4650},
      {"RGBA", "jpeg", 16670},
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
      {"RGB", "tif", 0},
      {"RGBA", "tif", 0},
#endif
    };

    const char* szTestImagePath = "TGATestImages/good";

    for (int idx = 0; idx < PLASMA_ARRAY_SIZE(imgTests); ++idx)
    {
      plImage image;
      {
        plStringBuilder fileName;
        fileName.Format("{}/{}.tga", szTestImagePath, imgTests[idx].szImage);

        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(image.LoadFrom(fileName) == PLASMA_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        plStringBuilder fileName;
        fileName.Format(":output/WriteImageTest/{}.{}", imgTests[idx].szImage, imgTests[idx].szFormat);

        plFileSystem::DeleteFile(fileName);

        PLASMA_TEST_BOOL_MSG(image.SaveTo(fileName) == PLASMA_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        PLASMA_TEST_BOOL_MSG(plFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        plImage image2;
        PLASMA_TEST_BOOL_MSG(image2.LoadFrom(fileName).Succeeded(), "Reading written image failed: '%s'", fileName.GetData());

        image.Convert(plImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();
        image2.Convert(plImageFormat::R8G8B8A8_UNORM_SRGB).IgnoreResult();

        plImage diff;
        plImageUtils::ComputeImageDifferenceABS(image, image2, diff);

        const plUInt32 uiMSE = plImageUtils::ComputeMeanSquareError(diff, 32);

        PLASMA_TEST_BOOL_MSG(uiMSE <= imgTests[idx].uiMSE, "MSE %u is larger than %u for image '%s'", uiMSE, imgTests[idx].uiMSE, fileName.GetData());
      }
    }
  }

  plFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
