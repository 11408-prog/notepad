#ifndef IMAGEVIEWERWIDGET_H
#define IMAGEVIEWERWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QListWidget>
#include <QLabel>
#include <QDir>

class ImageDisplayWidget;
class QPushButton;
class QLabel;
class QListWidget;
class QSpinBox;
class QSplitter;
class QToolButton;
class QSlider;
class QKeyEvent;
class QVBoxLayout;

// ====================== 主页面 ======================
class ImageViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageViewerWidget(QWidget *parent = nullptr);
    void openImage(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onOpenFile();
    void onOpenFolder();
    void onThumbnailClicked(QListWidgetItem *item);
    void onZoomIn();
    void onZoomOut();
    void onFitWindow();
    void onResetZoom();
    void onRotateLeft();
    void onRotateRight();
    void onExport();
    void onZoomSpinChanged(int value);
    void onZoomDisplayChanged(int percent);
    void onToggleThumbs();

    // 画质增强
    void onAutoEnhance();
    void onBlurSliderChanged(int value);
    void onResetEnhance();
    void onToggleEnhancePanel();

private:
    void setupUI();
    void setupEnhancePanel(QWidget *parent, QVBoxLayout *layout);
    void loadThumbnails(const QString &dirPath);
    void updateInfoBar(const QString &filePath);
    void navigateTo(int index);
    void applyEnhancement();
    QToolButton* makeToolBtn(const QString &text, const QString &tip);

    ImageDisplayWidget *m_display       = nullptr;
    QListWidget        *m_thumbList     = nullptr;
    QLabel             *m_infoLabel     = nullptr;
    QSpinBox           *m_zoomSpin      = nullptr;
    QSplitter          *m_splitter      = nullptr;
    QWidget            *m_thumbPanel    = nullptr;
    QWidget            *m_enhancePanel  = nullptr;
    QSlider            *m_blurSlider    = nullptr;
    QLabel             *m_blurLabel     = nullptr;
    bool                m_thumbsVisible   = true;
    bool                m_enhanceVisible  = false;

    // 原始图像（未增强），用于重置
    QPixmap             m_originalPixmap;
    int                 m_blurRadius    = 0;
    bool                m_equalized     = false;

    QString             m_currentDir;
    QStringList         m_imageFiles;
    int                 m_currentIndex  = -1;

    static const QStringList s_supportedFormats;
};

#endif // IMAGEVIEWERWIDGET_H
