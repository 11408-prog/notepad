#ifndef IMAGEDISPLAYWIDGET_H
#define IMAGEDISPLAYWIDGET_H

#include <QPoint>
#include <QPixmap>
#include <QWidget>

class ImageDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageDisplayWidget(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);
    void setZoomFactor(double factor);
    double zoomFactor() const { return m_zoomFactor; }
    void fitToWindow();
    void resetZoom();
    void rotateLeft();
    void rotateRight();
    bool hasImage() const { return !m_originalPixmap.isNull(); }
    QPixmap currentPixmap() const { return m_displayPixmap; }

signals:
    void zoomChanged(int percent);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateDisplay();

    QPixmap m_originalPixmap;
    QPixmap m_displayPixmap;
    double m_zoomFactor = 1.0;
    int m_rotation = 0;

    bool m_dragging = false;
    QPoint m_dragStart;
    QPoint m_offset;
};

#endif // IMAGEDISPLAYWIDGET_H
