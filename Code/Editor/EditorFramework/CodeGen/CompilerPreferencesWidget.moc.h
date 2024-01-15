#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class PLASMA_EDITORFRAMEWORK_DLL plQtCompilerPreferencesWidget : public plQtPropertyTypeWidget
{
  Q_OBJECT;

  private Q_SLOTS:

    void on_compiler_preset_changed(int index);

public:
  explicit plQtCompilerPreferencesWidget();
  virtual ~plQtCompilerPreferencesWidget();

  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;

protected:
  QComboBox* m_pCompilerPreset;
};