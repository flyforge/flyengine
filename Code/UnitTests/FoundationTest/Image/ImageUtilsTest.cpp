#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/ImageUtils.h>


PLASMA_CREATE_SIMPLE_TEST(Image, ImageUtils)
{
  plStringBuilder sReadDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
  plStringBuilder sWriteDir = plTestFramework::GetInstance()->GetAbsOutputPath();

  PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == PLASMA_SUCCESS);

  plResult addDir = plFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageTest");
  PLASMA_TEST_BOOL(addDir == PLASMA_SUCCESS);

  if (addDir.Failed())
    return;

  addDir = plFileSystem::AddDataDirectory(sWriteDir.GetData(), "ImageTest", "output", plFileSystem::AllowWrites);
  PLASMA_TEST_BOOL(addDir == PLASMA_SUCCESS);

  if (addDir.Failed())
    return;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeImageDifferenceABS RGB")
  {
    plImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    plImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGB.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedDiff_RGB.tga", "ImageUtils/Diff_RGB.tga", "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeImageDifferenceABS RGBA")
  {
    plImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga").IgnoreResult();

    plImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGBA.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Scaledown Half RGB")
  {
    plImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    plImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGB.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGB.tga", "ImageUtils/ScaledHalf_RGB.tga", "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Scaledown Half RGBA")
  {
    plImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    plImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGBA.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGBA.tga", "ImageUtils/ScaledHalf_RGBA.tga", "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CropImage RGB")
  {
    plImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    plImageUtils::CropImage(ImageA, plVec2I32(100, 50), plSizeU32(300, 200), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGB.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedCrop_RGB.tga", "ImageUtils/Crop_RGB.tga", "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CropImage RGBA")
  {
    plImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    plImageUtils::CropImage(ImageA, plVec2I32(100, 75), plSizeU32(300, 180), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGBA.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedCrop_RGBA.tga", "ImageUtils/Crop_RGBA.tga", "");
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeMeanSquareError")
  {
    plImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    plImage ImageAc, ImageBc;
    plImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();
    plImageUtils::Scale(ImageB, ImageBc, ImageB.GetWidth() / 2, ImageB.GetHeight() / 2).IgnoreResult();

    plImageUtils::ComputeImageDifferenceABS(ImageAc, ImageBc, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/MeanSquareDiff_RGB.tga").IgnoreResult();

    PLASMA_TEST_FILES("ImageUtils/ExpectedMeanSquareDiff_RGB.tga", "ImageUtils/MeanSquareDiff_RGB.tga", "");

    plUInt32 uiError = plImageUtils::ComputeMeanSquareError(ImageDiff, 4);
    PLASMA_TEST_INT(uiError, 1433);
  }

  plFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
