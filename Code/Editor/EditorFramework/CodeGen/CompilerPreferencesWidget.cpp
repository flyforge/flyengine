#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CompilerPreferencesWidget.moc.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtCompilerPreferencesWidget::plQtCompilerPreferencesWidget()
  : plQtPropertyTypeWidget(true)
{
  m_pCompilerPreset = new QComboBox();
  int counter = 0;
  for (auto& compiler : plCppProject::GetMachineSpecificCompilers())
  {
    m_pCompilerPreset->addItem(plMakeQString(compiler.m_sNiceName), counter);
    ++counter;
  }
  connect(m_pCompilerPreset, &QComboBox::currentIndexChanged, this, &plQtCompilerPreferencesWidget::on_compiler_preset_changed);

  auto gridLayout = new QGridLayout();
  gridLayout->setColumnStretch(0, 1);
  gridLayout->setColumnStretch(1, 0);
  gridLayout->setColumnMinimumWidth(1, 5);
  gridLayout->setColumnStretch(2, 2);
  gridLayout->setContentsMargins(0, 0, 0, 0);
  gridLayout->setSpacing(0);

  plStringBuilder fmt;
  QLabel* versionText = new QLabel(plMakeQString(
    plFmt("This SDK was compiled with {} version {}. Select a compatible compiler.",
      plCppProject::CompilerToString(plCppProject::GetSdkCompiler()),
      plCppProject::GetSdkCompilerMajorVersion())
      .GetText(fmt)));
  versionText->setWordWrap(true);
  gridLayout->addWidget(versionText, 0, 0, 1, 3);

  gridLayout->addWidget(new QLabel("Compiler Preset"), 1, 0);
  gridLayout->addWidget(m_pCompilerPreset, 1, 2);
  m_pGroupLayout->addLayout(gridLayout);
}

plQtCompilerPreferencesWidget::~plQtCompilerPreferencesWidget() = default;

void plQtCompilerPreferencesWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtScopedUpdatesDisabled _(this);

  plQtPropertyTypeWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    const auto& selection = m_pTypeWidget->GetSelection();

    PL_ASSERT_DEBUG(selection.GetCount() == 1, "Expected exactly one object");
    auto pObj = selection[0].m_pObject;

    plEnum<plCompiler> m_Compiler;
    bool bIsCustomCompiler;
    plString m_sCCompiler, m_sCppCompiler;

    {
      plVariant varCompiler, varIsCustomCompiler, varCCompiler, varCppCompiler;

      m_pObjectAccessor->GetValue(pObj, "Compiler", varCompiler).AssertSuccess();
      m_pObjectAccessor->GetValue(pObj, "CustomCompiler", varIsCustomCompiler).AssertSuccess();
      m_pObjectAccessor->GetValue(pObj, "CCompiler", varCCompiler).AssertSuccess();
      m_pObjectAccessor->GetValue(pObj, "CppCompiler", varCppCompiler).AssertSuccess();

      m_Compiler.SetValue(static_cast<plCompiler::StorageType>(varCompiler.Get<plInt64>()));
      bIsCustomCompiler = varIsCustomCompiler.Get<decltype(bIsCustomCompiler)>();
      m_sCCompiler = varCCompiler.Get<decltype(m_sCCompiler)>();
      m_sCppCompiler = varCppCompiler.Get<decltype(m_sCppCompiler)>();
    }

    plInt32 selectedIndex = -1;
    const auto& machineSpecificCompilers = plCppProject::GetMachineSpecificCompilers();
    // first look for non custom compilers
    for (plUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
    {
      const auto& curCompiler = machineSpecificCompilers[i];
      if ((curCompiler.m_bIsCustom == false) &&
          (curCompiler.m_Compiler == m_Compiler) &&
          (curCompiler.m_sCCompiler == m_sCCompiler) &&
          (curCompiler.m_sCppCompiler == m_sCppCompiler))
      {
        selectedIndex = static_cast<int>(i);
        break;
      }
    }

    if (selectedIndex == -1)
    {
      // If we didn't find a system default compiler, look for custom compilers next
      for (plUInt32 i = 0; i < machineSpecificCompilers.GetCount(); ++i)
      {
        const auto& curCompiler = machineSpecificCompilers[i];
        if (curCompiler.m_bIsCustom == true && curCompiler.m_Compiler == m_Compiler)
        {
          selectedIndex = static_cast<int>(i);
          break;
        }
      }
    }

    if (selectedIndex >= 0)
    {
      m_pCompilerPreset->blockSignals(true);
      m_pCompilerPreset->setCurrentIndex(selectedIndex);
      m_pCompilerPreset->blockSignals(false);
    }
  }
}

void plQtCompilerPreferencesWidget::on_compiler_preset_changed(int index)
{
  auto compilerPresets = plCppProject::GetMachineSpecificCompilers();

  if (index >= 0 && index < compilerPresets.GetCount())
  {
    const auto& preset = compilerPresets[index];

    const auto& selection = m_pTypeWidget->GetSelection();
    PL_ASSERT_DEV(selection.GetCount() == 1, "This Widget does not support multi selection");

    auto obj = selection[0].m_pObject;
    m_pObjectAccessor->StartTransaction("Change Compiler Preset");
    m_pObjectAccessor->SetValue(obj, "Compiler", preset.m_Compiler.GetValue()).AssertSuccess();
    m_pObjectAccessor->SetValue(obj, "CustomCompiler", preset.m_bIsCustom).AssertSuccess();
    m_pObjectAccessor->SetValue(obj, "CCompiler", preset.m_sCCompiler).AssertSuccess();
    m_pObjectAccessor->SetValue(obj, "CppCompiler", preset.m_sCppCompiler).AssertSuccess();
    m_pObjectAccessor->FinishTransaction();
  }
}

void plCompilerPreferences_PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  const plRTTI* pRtti = plGetStaticRTTI<plCompilerPreferences>();

  auto& typeAccessor = e.m_pObject->GetTypeAccessor();

  if (typeAccessor.GetType() != pRtti)
    return;

  plPropertyUiState::Visibility compilerFieldsVisibility = plPropertyUiState::Default;

  bool bCustomCompiler = typeAccessor.GetValue("CustomCompiler").Get<bool>();
  if (!bCustomCompiler)
  {
    compilerFieldsVisibility = plPropertyUiState::Disabled;
  }
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  auto compiler = typeAccessor.GetValue("Compiler").Get<plInt64>();
  if (compiler == plCompiler::Vs2022)
  {
    compilerFieldsVisibility = plPropertyUiState::Invisible;
  }
#endif

  auto& props = *e.m_pPropertyStates;

  props["CCompiler"].m_Visibility = compilerFieldsVisibility;
  props["CppCompiler"].m_Visibility = compilerFieldsVisibility;
#if PL_ENABLED(PL_PLATFORM_LINUX)
  props["RcCompiler"].m_Visibility = plPropertyUiState::Invisible;
#else
  props["RcCompiler"].m_Visibility = (compiler == plCompiler::Vs2022) ? plPropertyUiState::Invisible : plPropertyUiState::Default;
#endif
}
