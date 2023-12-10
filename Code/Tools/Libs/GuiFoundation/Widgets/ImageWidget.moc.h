#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ImageWidget.h>
#include <QGraphicsScene>

class QGraphicsPixmapItem;

class PLASMA_GUIFOUNDATION_DLL plQtImageScene : public QGraphicsScene
{
public:
  plQtImageScene(QObject* pParent = nullptr);

  void SetImage(QPixmap pixmap);

private:
  QPixmap m_Pixmap;
  QGraphicsPixmapItem* m_pImageItem;
};

class PLASMA_GUIFOUNDATION_DLL plQtImageWidget : public QWidget, public Ui_ImageWidget
{
  Q_OBJECT

public:
  plQtImageWidget(QWidget* pParent, bool bShowButtons = true);
  ~plQtImageWidget();

  void SetImage(QPixmap pixmap);

  void SetImageSize(float fScale = 1.0f);
  void ScaleImage(float fFactor);

private Q_SLOTS:

  void on_ButtonZoomIn_clicked();
  void on_ButtonZoomOut_clicked();
  void on_ButtonResetZoom_clicked();

private:
  void ImageApplyScale();

  plQtImageScene* m_pScene;
  float m_fCurrentScale;
};

