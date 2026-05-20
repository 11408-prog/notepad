#include "imagedisplaywidget.h"

#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTransform>
#include <QWheelEvent>

ImageDisplayWidget::ImageDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 200);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #1a1a1a;");
}

void ImageDisplayWidget::setPixmap(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    m_rotation = 0;
    m_offset = QPoint(0, 0);
    fitToWindow();
}

void ImageDisplayWidget::setZoomFactor(double factor)
{
    m_zoomFactor = qBound(0.05, factor, 20.0);
    updateDisplay();
    emit zoomChanged(qRound(m_zoomFactor * 100));
}

void ImageDisplayWidget::fitToWindow()
{
    if (m_originalPixmap.isNull()) return;
    double fw = static_cast<double>(width()) / m_originalPixmap.width();
    double fh = static_cast<double>(height()) / m_originalPixmap.height();
    m_zoomFactor = qMin(fw, fh) * 0.96;
    m_offset = QPoint(0, 0);
    updateDisplay();
    emit zoomChanged(qRound(m_zoomFactor * 100));
}

void ImageDisplayWidget::resetZoom()
{
    m_zoomFactor = 1.0;
    m_offset = QPoint(0, 0);
    updateDisplay();
    emit zoomChanged(100);
}

void ImageDisplayWidget::rotateLeft()
{
    m_rotation = (m_rotation + 270) % 360;
    m_offset = QPoint(0, 0);
    updateDisplay();
}

void ImageDisplayWidget::rotateRight()
{
    m_rotation = (m_rotation + 90) % 360;
    m_offset = QPoint(0, 0);
    updateDisplay();
}

void ImageDisplayWidget::updateDisplay()
{
    if (m_originalPixmap.isNull()) {
        update();
        return;
    }

    QTransform transform;
    transform.rotate(m_rotation);
    QPixmap rotated = m_originalPixmap.transformed(transform, Qt::SmoothTransformation);
    const int newW = qRound(rotated.width() * m_zoomFactor);
    const int newH = qRound(rotated.height() * m_zoomFactor);
    m_displayPixmap = rotated.scaled(newW, newH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    update();
}

void ImageDisplayWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#1a1a1a"));
    if (m_displayPixmap.isNull()) {
        painter.setPen(QColor("#555"));
        painter.setFont(QFont("Microsoft YaHei", 14));
        painter.drawText(rect(), Qt::AlignCenter, "打开图片或文件夹");
        return;
    }

    const int x = (width() - m_displayPixmap.width()) / 2 + m_offset.x();
    const int y = (height() - m_displayPixmap.height()) / 2 + m_offset.y();
    painter.drawPixmap(x, y, m_displayPixmap);
}

void ImageDisplayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_originalPixmap.isNull() && m_zoomFactor < 1.0)
        fitToWindow();
}

void ImageDisplayWidget::wheelEvent(QWheelEvent *event)
{
    const double delta = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
    setZoomFactor(m_zoomFactor * delta);
    event->accept();
}

void ImageDisplayWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageDisplayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_offset += event->pos() - m_dragStart;
        m_dragStart = event->pos();
        update();
    }
}

void ImageDisplayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}
