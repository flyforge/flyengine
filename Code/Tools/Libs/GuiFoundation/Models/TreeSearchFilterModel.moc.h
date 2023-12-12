#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QSortFilterProxyModel>

class QWidget;

class PLASMA_GUIFOUNDATION_DLL plQtTreeSearchFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  plQtTreeSearchFilterModel(QWidget* parent);

  void SetFilterText(const QString& text);

  /// \brief By default only nodes (and their parents) are shown that fit the search criterion.
  /// If this is enabled, all child nodes of nodes that fit the criterion are included as well.
  void SetIncludeChildren(bool bInclude);

protected:
  void RecomputeVisibleItems();
  bool UpdateVisibility(const QModelIndex& idx, bool bParentIsVisible);
  virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

  bool m_bIncludeChildren;
  QAbstractItemModel* m_pSourceModel;
  QString m_sFilterText;
  plMap<QModelIndex, bool> m_Visible;
};

