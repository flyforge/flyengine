#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/Strings/HashedString.h>
#include <Utilities/UtilitiesDLL.h>

using plConfigFileResourceHandle = plTypedResourceHandle<class plConfigFileResource>;

/// \brief This resource loads config files containing key/value pairs
///
/// The config files usually use the file extension '.plConfig'.
///
/// The file format looks like this:
///
/// To declare a key/value pair for the first time, write its type, name and value:
///   int i = 1
///   float f = 2.3
///   bool b = false
///   string s = "hello"
///
/// To set a variable to a different value than before, it has to be marked with 'override':
///
///   override i = 4
///
/// The format supports C preprocessor features like #include, #define, #ifdef, etc.
/// This can be used to build hierarchical config files:
///
///   #include "BaseConfig.plConfig"
///   override int SomeValue = 7
///
/// It can also be used to define 'enum types':
///
///   #define SmallValue 3
///   #define BigValue 5
///   int MyValue = BigValue
///
/// Since resources can be reloaded at runtime, config resources are a convenient way to define game parameters
/// that you may want to tweak at any time.
/// Using C preprocessor logic (#define, #if, #else, etc) you can quickly select between different configuration sets.
///
/// Once loaded, accessing the data is very efficient.
class PLASMA_UTILITIES_DLL plConfigFileResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConfigFileResource, plResource);

  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plConfigFileResource);

public:
  plConfigFileResource();
  ~plConfigFileResource();

  /// \brief Returns the 'int' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  plInt32 GetInt(plTempHashedString sName) const;

  /// \brief Returns the 'float' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  float GetFloat(plTempHashedString sName) const;

  /// \brief Returns the 'bool' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  bool GetBool(plTempHashedString sName) const;

  /// \brief Returns the 'string' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  plStringView GetString(plTempHashedString sName) const;

  /// \brief Returns the 'int' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  plInt32 GetInt(plTempHashedString sName, plInt32 iFallback) const;

  /// \brief Returns the 'float' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  float GetFloat(plTempHashedString sName, float fFallback) const;

  /// \brief Returns the 'bool' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  bool GetBool(plTempHashedString sName, bool bFallback) const;

  /// \brief Returns the 'string' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  plStringView GetString(plTempHashedString sName, plStringView sFallback) const;

protected:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class plConfigFileResourceLoader;

  plHashTable<plHashedString, plInt32> m_IntData;
  plHashTable<plHashedString, float> m_FloatData;
  plHashTable<plHashedString, plString> m_StringData;
  plHashTable<plHashedString, bool> m_BoolData;

  plDependencyFile m_RequiredFiles;
};


class PLASMA_UTILITIES_DLL plConfigFileResourceLoader : public plResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    plDefaultMemoryStreamStorage m_Storage;
    plMemoryStreamReader m_Reader;
    plDependencyFile m_RequiredFiles;

    plResult PrePropFileLocator(plStringView sCurAbsoluteFile, plStringView sIncludeFile, plPreprocessor::IncludeType incType, plStringBuilder& out_sAbsoluteFilePath);
  };

  virtual plResourceLoadData OpenDataStream(const plResource* pResource) override;
  virtual void CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const plResource* pResource) const override;
};
