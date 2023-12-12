#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QGraphicsPixmapItem>
#include <QScrollArea>
#include <QScrollBar>

plQtImageScene::plQtImageScene(QObject* pParent)
  : QGraphicsScene(pParent)
{
  m_pImageItem = nullptr;
  setItemIndexMethod(QGraphicsScene::NoIndex);
}

void plQtImageScene::SetImage(QPixmap pixmap)
{
  if (m_pImageItem)
    delete m_pImageItem;

  m_Pixmap = pixmap;
  m_pImageItem = addPixmap(m_Pixmap);
  setSceneRect(0, 0, m_Pixmap.width(), m_Pixmap.height());
}



plQtImageWidget::plQtImageWidget(QWidget* parent, bool bShowButtons)
  : QWidget(parent)
{
  setupUi(this);
  m_pScene = new plQtImageScene(GraphicsView);
  GraphicsView->setScene(m_pScene);

  m_fCurrentScale = 1.0f;

  if (!bShowButtons)
    ButtonBar->setVisible(false);
}

plQtImageWidget::~plQtImageWidget() {}

void plQtImageWidget::SetImageSize(float fScale)
{
  if (m_fCurrentScale == fScale)
    return;

  m_fCurrentScale = fScale;
  ImageApplyScale();
}

void plQtImageWidget::ScaleImage(float fFactor)
{
  float fPrevScale = m_fCurrentScale;
  m_fCurrentScale = plMath::Clamp(m_fCurrentScale * fFactor, 0.2f, 5.0f);

  fFactor = m_fCurrentScale / fPrevScale;
  ImageApplyScale();
}

void plQtImageWidget::ImageApplyScale()
{
  QTransform scale = QTransform::fromScale(m_fCurrentScale, m_fCurrentScale);
  GraphicsView->setTransform(scale);
}

void plQtImageWidget::SetImage(QPixmap pixmap)
{
  m_pScene->SetImage(pixmap);
  ImageApplyScale();
}

void plQtImageWidget::on_ButtonZoomIn_clicked()
{
  ScaleImage(1.25f);
}

void plQtImageWidget::on_ButtonZoomOut_clicked()
{
  ScaleImage(0.75f);
}

void plQtImageWidget::on_ButtonResetZoom_clicked()
{
  SetImageSize(1.0f);
}
