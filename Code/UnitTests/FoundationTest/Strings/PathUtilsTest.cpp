#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Strings, PathUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsPathSeparator")
  {
    for (int i = 0; i < 0xFFFF; ++i)
    {
      if (i == '/')
      {
        PLASMA_TEST_BOOL(plPathUtils::IsPathSeparator(i));
      }
      else if (i == '\\')
      {
        PLASMA_TEST_BOOL(plPathUtils::IsPathSeparator(i));
      }
      else
      {
        PLASMA_TEST_BOOL(!plPathUtils::IsPathSeparator(i));
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindPreviousSeparator")
  {
    const char* szPath = "This/Is\\My//Path.dot\\file.extension";

    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 35) == szPath + 20);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 20) == szPath + 11);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 11) == szPath + 10);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 10) == szPath + 7);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 7) == szPath + 4);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath + 4) == nullptr);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(szPath, szPath) == nullptr);
    PLASMA_TEST_BOOL(plPathUtils::FindPreviousSeparator(nullptr, nullptr) == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasAnyExtension")
  {
    PLASMA_TEST_BOOL(plPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    PLASMA_TEST_BOOL(!plPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    PLASMA_TEST_BOOL(!plPathUtils::HasAnyExtension(""));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasExtension")
  {
    PLASMA_TEST_BOOL(plPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    PLASMA_TEST_BOOL(plPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    PLASMA_TEST_BOOL(!plPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    PLASMA_TEST_BOOL(!plPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    PLASMA_TEST_BOOL(!plPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    PLASMA_TEST_BOOL(!plPathUtils::HasExtension("", "ext"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileExtension")
  {
    PLASMA_TEST_BOOL(plPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    PLASMA_TEST_BOOL(plPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileExtension("") == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileNameAndExtension")
  {
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file.extension") == "file.extension");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\.extension") == ".extension");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file") == "file");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("\\file") == "file");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("/") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\") == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileName")
  {
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("\\file") == "file");
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("/") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    PLASMA_TEST_BOOL(plPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileDirectory")
  {
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file.extension") == "This/Is\\My//Path.dot\\");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\.extension") == "This/Is\\My//Path.dot\\");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file") == "This/Is\\My//Path.dot\\");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("\\file") == "\\");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("") == "");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("/") == "/");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\") == "This/Is\\My//Path.dot\\");
    PLASMA_TEST_BOOL(plPathUtils::GetFileDirectory("This") == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsAbsolutePath")
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    PLASMA_TEST_BOOL(plPathUtils::IsAbsolutePath("C:\\temp.stuff"));
    PLASMA_TEST_BOOL(plPathUtils::IsAbsolutePath("C:/temp.stuff"));
    PLASMA_TEST_BOOL(plPathUtils::IsAbsolutePath("\\\\myserver\\temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("\\myserver\\temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("/temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("\\temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("..\\temp.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath(".\\temp.stuff"));
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
    PLASMA_TEST_BOOL(plPathUtils::IsAbsolutePath("/usr/local/.stuff"));
    PLASMA_TEST_BOOL(plPathUtils::IsAbsolutePath("/file.test"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("./file.stuff"));
    PLASMA_TEST_BOOL(!plPathUtils::IsAbsolutePath("file.stuff"));
#else
#  error "Unknown platform."
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRootedPathParts")
  {
    plStringView root, relPath;
    plPathUtils::GetRootedPathParts(":MyRoot\\folder\\file.txt", root, relPath);
    PLASMA_TEST_BOOL(plPathUtils::GetRootedPathRootName(":MyRoot\\folder\\file.txt") == root);
    PLASMA_TEST_BOOL(root == "MyRoot");
    PLASMA_TEST_BOOL(relPath == "folder\\file.txt");
    plPathUtils::GetRootedPathParts("folder\\file2.txt", root, relPath);
    PLASMA_TEST_BOOL(root.IsEmpty());
    PLASMA_TEST_BOOL(relPath == "folder\\file2.txt");
  }
}
