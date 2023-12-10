#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_ReflectionWidget.h>
#include <ads/DockWidget.h>

class plQtReflectionWidget : public ads::CDockWidget, public Ui_ReflectionWidget
{
public:
  Q_OBJECT

public:
  plQtReflectionWidget(QWidget* pParent = 0);

  static plQtReflectionWidget* s_pWidget;

private Q_SLOTS:

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct PropertyData
  {
    plString m_sType;
    plString m_sPropertyName;
    plInt8 m_iCategory;
  };

  struct TypeData
  {
    TypeData() { m_pTreeItem = nullptr; }

    QTreeWidgetItem* m_pTreeItem;

    plUInt32 m_uiSize;
    plString m_sParentType;
    plString m_sPlugin;

    plHybridArray<PropertyData, 16> m_Properties;
  };

  bool UpdateTree();

  plMap<plString, TypeData> m_Types;
};

