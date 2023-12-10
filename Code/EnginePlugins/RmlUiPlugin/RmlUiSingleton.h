#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class plRmlUiContext;
struct plMsgExtractRenderData;

/// \brief The RmlUI configuration to be used on a specific platform
struct PLASMA_RMLUIPLUGIN_DLL plRmlUiConfiguration
{
  plDynamicArray<plString> m_Fonts;

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/RmlUiConfig.ddl"_plsv;

  plResult Save(plStringView sFile = s_sConfigFile) const;
  plResult Load(plStringView sFile = s_sConfigFile);

  bool operator==(const plRmlUiConfiguration& rhs) const;
  bool operator!=(const plRmlUiConfiguration& rhs) const { return !operator==(rhs); }
};

class PLASMA_RMLUIPLUGIN_DLL plRmlUi
{
  PLASMA_DECLARE_SINGLETON(plRmlUi);

public:
  plRmlUi();
  ~plRmlUi();

  plRmlUiContext* CreateContext(const char* szName, const plVec2U32& initialSize);
  void DeleteContext(plRmlUiContext* pContext);

  bool AnyContextWantsInput();

  void ExtractContext(plRmlUiContext& context, plMsgExtractRenderData& msg);

private:
  struct Data;
  plUniquePtr<Data> m_pData;
};
