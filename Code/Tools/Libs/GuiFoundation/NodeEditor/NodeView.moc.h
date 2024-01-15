#pragma once

#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class plQtNodeScene;

class PLASMA_GUIFOUNDATION_DLL plQtNodeView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit plQtNodeView(QWidget* parent = nullptr);
  ~plQtNodeView();

  void SetScene(plQtNodeScene* pScene);
  plQtNodeScene* GetScene();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  virtual void resizeEvent(QResizeEvent*) override;

  virtual void drawBackground(QPainter *painter, const QRectF &r) override;

private:
  void UpdateView();

  void DrawGrid(QPainter *painter, const double gridStep);

private:
  plQtNodeScene* m_pScene;
  bool m_bPanning;
  plInt32 m_iPanCounter;

  QPointF m_ViewPos;
  QPointF m_ViewScale;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF m_StartDragView;
#else
  QPoint m_vStartDragView;
#endif

  QPointF m_StartDragScene;
};
