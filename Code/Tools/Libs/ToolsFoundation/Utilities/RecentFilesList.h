#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief Maintains a list of recently used files and the container window ID they previously resided in.
class PL_TOOLSFOUNDATION_DLL plRecentFilesList
{
public:
  plRecentFilesList(plUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  /// \brief Struct that defines the file and container window of the recent file list.
  struct RecentFile
  {
    RecentFile()
      : m_iContainerWindow(0)
    {
    }
    RecentFile(plStringView sFile, plInt32 iContainerWindow)
      : m_File(sFile)
      , m_iContainerWindow(iContainerWindow)
    {
    }

    plString m_File;
    plInt32 m_iContainerWindow;
  };
  /// \brief Moves the inserted file to the front with the given container ID.
  void Insert(plStringView sFile, plInt32 iContainerWindow);

  /// \brief Returns all files in the list.
  const plDeque<RecentFile>& GetFileList() const { return m_Files; }

  /// \brief Clears the list
  void Clear() { m_Files.Clear(); }

  /// \brief Saves the recent files list to the given file. Uses a simple text file format (one line per item).
  void Save(plStringView sFile);

  /// \brief Loads the recent files list from the given file. Uses a simple text file format (one line per item).
  void Load(plStringView sFile);

private:
  plUInt32 m_uiMaxElements;
  plDeque<RecentFile> m_Files;
};
