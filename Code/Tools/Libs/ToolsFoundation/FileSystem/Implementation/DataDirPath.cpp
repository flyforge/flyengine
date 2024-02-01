#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/DataDirPath.h>

bool plDataDirPath::UpdateDataDirInfos(plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex /*= 0*/) const
{
  const plUInt32 uiCount = dataDirRoots.GetCount();
  for (plUInt32 i = 0; i < uiCount; ++i)
  {
    plUInt32 uiCurrentIndex = (uiLastKnownDataDirIndex + i) % uiCount;
    PL_ASSERT_DEBUG(!dataDirRoots[uiCurrentIndex].EndsWith_NoCase("/"), "");
    if (m_sAbsolutePath.StartsWith_NoCase(dataDirRoots[uiCurrentIndex]) && !dataDirRoots[uiCurrentIndex].IsEmpty())
    {
      m_uiDataDirIndex = uiCurrentIndex;
      const char* szParentFolder = plPathUtils::FindPreviousSeparator(m_sAbsolutePath.GetData(), m_sAbsolutePath.GetData() + dataDirRoots[uiCurrentIndex].GetElementCount());
      m_uiDataDirParent = szParentFolder - m_sAbsolutePath.GetData();
      m_uiDataDirLength = dataDirRoots[uiCurrentIndex].GetElementCount() - m_uiDataDirParent;
      return true;
    }
  }

  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
  return false;
}
