#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class PL_FOUNDATION_DLL plApplicationFileSystemConfig
{
public:
  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/DataDirectories.ddl"_plsv;

  plResult Save(plStringView sPath = s_sConfigFile);
  void Load(plStringView sPath = s_sConfigFile);

  /// \brief Sets up the data directories that were configured or loaded into this object
  void Apply();

  /// \brief Removes all data directories that were set up by any call to plApplicationFileSystemConfig::Apply()
  static void Clear();

  plResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    plString m_sDataDirSpecialPath;
    plString m_sRootName;
    bool m_bWritable;            ///< Whether the directory is going to be mounted for writing
    bool m_bHardCodedDependency; ///< If set to true, this indicates that it may not be removed by the user (in a config dialog)

    DataDirConfig()
    {
      m_bWritable = false;
      m_bHardCodedDependency = false;
    }

    bool operator==(const DataDirConfig& rhs) const
    {
      return m_bWritable == rhs.m_bWritable && m_sDataDirSpecialPath == rhs.m_sDataDirSpecialPath && m_sRootName == rhs.m_sRootName;
    }
  };

  bool operator==(const plApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  plHybridArray<DataDirConfig, 4> m_DataDirs;
};


using plApplicationFileSystemConfig_DataDirConfig = plApplicationFileSystemConfig::DataDirConfig;

PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plApplicationFileSystemConfig);
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plApplicationFileSystemConfig_DataDirConfig);
