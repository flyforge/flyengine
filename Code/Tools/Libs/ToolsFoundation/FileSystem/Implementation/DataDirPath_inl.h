

inline plDataDirPath::plDataDirPath() = default;

inline plDataDirPath::plDataDirPath(plStringView sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  PLASMA_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline plDataDirPath::plDataDirPath(const plStringBuilder& sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  PLASMA_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = sAbsPath;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline plDataDirPath::plDataDirPath(plString&& sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  PLASMA_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = std::move(sAbsPath);
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline plDataDirPath::operator plStringView() const
{
  return m_sAbsolutePath;
}

inline bool plDataDirPath::operator==(plStringView rhs) const
{
  return m_sAbsolutePath == rhs;
}

inline bool plDataDirPath::operator!=(plStringView rhs) const
{
  return m_sAbsolutePath != rhs;
}

inline bool plDataDirPath::IsValid() const
{
  return m_uiDataDirParent != 0;
}

inline void plDataDirPath::Clear()
{
  m_sAbsolutePath.Clear();
  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
}

inline const plString& plDataDirPath::GetAbsolutePath() const
{
  return m_sAbsolutePath;
}

inline plStringView plDataDirPath::GetDataDirParentRelativePath() const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const plUInt32 uiOffset = m_uiDataDirParent + 1;
  return plStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline plStringView plDataDirPath::GetDataDirRelativePath() const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const plUInt32 uiOffset = plMath::Min(m_sAbsolutePath.GetElementCount(), m_uiDataDirParent + m_uiDataDirLength + 1u);
  return plStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline plStringView plDataDirPath::GetDataDir() const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return plStringView(m_sAbsolutePath.GetData(), m_uiDataDirParent + m_uiDataDirLength);
}

inline plUInt8 plDataDirPath::GetDataDirIndex() const
{
  PLASMA_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return m_uiDataDirIndex;
}

inline plStreamWriter& plDataDirPath::Write(plStreamWriter& inout_stream) const
{
  inout_stream << m_sAbsolutePath;
  inout_stream << m_uiDataDirParent;
  inout_stream << m_uiDataDirLength;
  inout_stream << m_uiDataDirIndex;
  return inout_stream;
}

inline plStreamReader& plDataDirPath::Read(plStreamReader& inout_stream)
{
  inout_stream >> m_sAbsolutePath;
  inout_stream >> m_uiDataDirParent;
  inout_stream >> m_uiDataDirLength;
  inout_stream >> m_uiDataDirIndex;
  return inout_stream;
}

bool plCompareDataDirPath::Less(plStringView lhs, plStringView rhs)
{
  int res = lhs.Compare_NoCase(rhs);
  if (res == 0)
  {
    return lhs.Compare(rhs) < 0;
  }

  return res < 0;
}

bool plCompareDataDirPath::Equal(plStringView lhs, plStringView rhs)
{
  return lhs.IsEqual(rhs);
}

inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plDataDirPath& value)
{
  return value.Write(inout_stream);
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plDataDirPath& out_value)
{
  return out_value.Read(inout_stream);
}
