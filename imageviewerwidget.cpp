#include "imageviewerwidget.h"
#include "imageenhancer.h"
#include "imagedisplaywidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QToolButton>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QListWidget>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QIcon>
#include <QDir>
#include <QKeyEvent>
#include <QImageReader>
#include <QSignalBlocker>

// ====================== 支持格式 ======================

const QStringList ImageViewerWidget::s_supportedFormats = {
    "*.png","*.jpg","*.jpeg","*.bmp","*.gif",
    "*.webp","*.tiff","*.tif","*.ico"
};

static QPixmap loadPixmapWithReader(const QString &path, const QSize &targetSize = QSize())
{
    QImageReader reader(path);
    reader.setAutoTransform(true);

    if (targetSize.isValid()) {
        const QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            reader.setScaledSize(originalSize.scaled(targetSize, Qt::KeepAspectRatio));
        }
    }

    const QImage image = reader.read();
    return image.isNull() ? QPixmap() : QPixmap::fromImage(image);
}

// ====================== ImageViewerWidget ======================

ImageViewerWidget::ImageViewerWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

QToolButton* ImageViewerWidget::makeToolBtn(const QString &text, const QString &tip)
{
    auto *btn = new QToolButton(this);
    btn->setText(text);
    btn->setToolTip(tip);
    btn->setFixedHeight(30);
    btn->setMinimumWidth(30);
    btn->setStyleSheet(R"(
        QToolButton {
            border: none; border-radius: 4px; padding: 0 8px;
            font-size: 13px; color: #cccccc; background: transparent;
        }
        QToolButton:hover   { background: rgba(255,255,255,0.1); color: #ffffff; }
        QToolButton:pressed { background: rgba(255,255,255,0.06); }
    )");
    return btn;
}

void ImageViewerWidget::setupUI()
{
    setStyleSheet("background-color: #252526;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 顶部工具栏 ==========
    QWidget *toolbar = new QWidget(this);
    toolbar->setFixedHeight(42);
    toolbar->setStyleSheet("background-color: #2d2d2d; border-bottom: 1px solid #3a3a3a;");

    QHBoxLayout *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(8, 0, 8, 0);
    tbLayout->setSpacing(2);

    auto makeSep = [this]() {
        auto *sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setFixedWidth(1);
        sep->setStyleSheet("background: #3a3a3a;");
        return sep;
    };

    auto *openFileBtn   = makeToolBtn("📂 打开",   "打开图片文件");
    auto *openFolderBtn = makeToolBtn("🗂 文件夹", "打开整个文件夹");
    auto *prevBtn       = makeToolBtn("◀",         "上一张  ←");
    auto *nextBtn       = makeToolBtn("▶",         "下一张  →");
    auto *zoomOutBtn    = makeToolBtn("－",         "缩小");
    auto *zoomInBtn     = makeToolBtn("＋",         "放大");
    auto *fitBtn        = makeToolBtn("⊡ 适应",    "适应窗口大小");
    auto *oneToOneBtn   = makeToolBtn("1:1",        "原始尺寸");
    auto *rotLBtn       = makeToolBtn("↺",          "向左旋转 90°");
    auto *rotRBtn       = makeToolBtn("↻",          "向右旋转 90°");
    auto *enhanceBtn    = makeToolBtn("✨ 增强",    "画质增强面板");
    auto *thumbToggleBtn= makeToolBtn("▥",          "显示/隐藏缩略图");
    auto *exportBtn     = makeToolBtn("↑ 导出",    "导出当前图片");

    m_zoomSpin = new QSpinBox(this);
    m_zoomSpin->setRange(5, 2000);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    m_zoomSpin->setFixedSize(70, 28);
    m_zoomSpin->setAlignment(Qt::AlignCenter);
    m_zoomSpin->setStyleSheet(R"(
        QSpinBox {
            background: #3a3a3a; border: 1px solid #4a4a4a;
            border-radius: 4px; color: #cccccc; font-size: 12px;
        }
        QSpinBox::up-button, QSpinBox::down-button { width: 0; }
    )");

    connect(openFileBtn,    &QToolButton::clicked, this, &ImageViewerWidget::onOpenFile);
    connect(openFolderBtn,  &QToolButton::clicked, this, &ImageViewerWidget::onOpenFolder);
    connect(prevBtn,        &QToolButton::clicked, this, [this]{ navigateTo(m_currentIndex - 1); });
    connect(nextBtn,        &QToolButton::clicked, this, [this]{ navigateTo(m_currentIndex + 1); });
    connect(zoomOutBtn,     &QToolButton::clicked, this, &ImageViewerWidget::onZoomOut);
    connect(zoomInBtn,      &QToolButton::clicked, this, &ImageViewerWidget::onZoomIn);
    connect(fitBtn,         &QToolButton::clicked, this, &ImageViewerWidget::onFitWindow);
    connect(oneToOneBtn,    &QToolButton::clicked, this, &ImageViewerWidget::onResetZoom);
    connect(rotLBtn,        &QToolButton::clicked, this, &ImageViewerWidget::onRotateLeft);
    connect(rotRBtn,        &QToolButton::clicked, this, &ImageViewerWidget::onRotateRight);
    connect(enhanceBtn,     &QToolButton::clicked, this, &ImageViewerWidget::onToggleEnhancePanel);
    connect(thumbToggleBtn, &QToolButton::clicked, this, &ImageViewerWidget::onToggleThumbs);
    connect(exportBtn,      &QToolButton::clicked, this, &ImageViewerWidget::onExport);
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ImageViewerWidget::onZoomSpinChanged);

    tbLayout->addWidget(openFileBtn);
    tbLayout->addWidget(openFolderBtn);
    tbLayout->addWidget(makeSep());
    tbLayout->addWidget(prevBtn);
    tbLayout->addWidget(nextBtn);
    tbLayout->addWidget(makeSep());
    tbLayout->addWidget(zoomOutBtn);
    tbLayout->addWidget(m_zoomSpin);
    tbLayout->addWidget(zoomInBtn);
    tbLayout->addWidget(fitBtn);
    tbLayout->addWidget(oneToOneBtn);
    tbLayout->addWidget(makeSep());
    tbLayout->addWidget(rotLBtn);
    tbLayout->addWidget(rotRBtn);
    tbLayout->addStretch();
    tbLayout->addWidget(enhanceBtn);
    tbLayout->addWidget(makeSep());
    tbLayout->addWidget(thumbToggleBtn);
    tbLayout->addWidget(makeSep());
    tbLayout->addWidget(exportBtn);

    // ========== 中间：图片 + 右侧面板 ==========
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setStyleSheet("QSplitter::handle { background: #3a3a3a; }");

    m_display = new ImageDisplayWidget(this);
    connect(m_display, &ImageDisplayWidget::zoomChanged,
            this, &ImageViewerWidget::onZoomDisplayChanged);

    // 右侧容器：增强面板（上）+ 缩略图（下）
    QWidget *rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(160);
    rightPanel->setStyleSheet("background: #2a2a2a;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // ---- 画质增强面板 ----
    m_enhancePanel = new QWidget(rightPanel);
    m_enhancePanel->setVisible(false);
    m_enhancePanel->setStyleSheet("background: #2a2a2a; border-bottom: 1px solid #3a3a3a;");
    setupEnhancePanel(m_enhancePanel, rightLayout);

    // ---- 缩略图面板 ----
    m_thumbPanel = new QWidget(rightPanel);
    m_thumbPanel->setStyleSheet("background: #2a2a2a;");
    QVBoxLayout *thumbLayout = new QVBoxLayout(m_thumbPanel);
    thumbLayout->setContentsMargins(4, 4, 4, 4);
    thumbLayout->setSpacing(0);

    QLabel *thumbTitle = new QLabel("图片列表", m_thumbPanel);
    thumbTitle->setAlignment(Qt::AlignCenter);
    thumbTitle->setStyleSheet("color: #888; font-size: 11px; padding: 4px 0 6px 0;");
    thumbLayout->addWidget(thumbTitle);

    m_thumbList = new QListWidget(m_thumbPanel);
    m_thumbList->setViewMode(QListWidget::IconMode);
    m_thumbList->setIconSize(QSize(130, 100));
    m_thumbList->setSpacing(4);
    m_thumbList->setResizeMode(QListWidget::Adjust);
    m_thumbList->setStyleSheet(R"(
        QListWidget { background: transparent; border: none; }
        QListWidget::item { border-radius: 4px; padding: 2px; }
        QListWidget::item:hover    { background: rgba(255,255,255,0.08); }
        QListWidget::item:selected { background: #0078d4; }
    )");
    connect(m_thumbList, &QListWidget::itemClicked,
            this, &ImageViewerWidget::onThumbnailClicked);
    thumbLayout->addWidget(m_thumbList);

    rightLayout->addWidget(m_enhancePanel);
    rightLayout->addWidget(m_thumbPanel, 1);

    m_splitter->addWidget(m_display);
    m_splitter->addWidget(rightPanel);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    // ========== 底部信息栏 ==========
    m_infoLabel = new QLabel("尚未打开图片", this);
    m_infoLabel->setFixedHeight(24);
    m_infoLabel->setStyleSheet(R"(
        QLabel {
            background: #2d2d2d; color: #777; font-size: 11px;
            padding: 0 10px; border-top: 1px solid #3a3a3a;
        }
    )");

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(m_splitter, 1);
    mainLayout->addWidget(m_infoLabel);
}

// ====================== 画质增强面板 ======================

void ImageViewerWidget::setupEnhancePanel(QWidget *panel, QVBoxLayout *parentLayout)
{
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    // 标题
    QLabel *title = new QLabel("画质增强", panel);
    title->setStyleSheet("color: #cccccc; font-size: 12px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // 分割线
    QFrame *line1 = new QFrame(panel);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("background: #3a3a3a;");
    layout->addWidget(line1);

    // 一键自动增强按钮
    auto *autoBtn = new QToolButton(panel);
    autoBtn->setText("一键自动增强");
    autoBtn->setToolTip("直方图均衡化 + 轻度锐化");
    autoBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    autoBtn->setFixedHeight(30);
    autoBtn->setStyleSheet(R"(
        QToolButton {
            background: #0078d4; color: white; border: none;
            border-radius: 4px; font-size: 12px;
        }
        QToolButton:hover   { background: #1084d8; }
        QToolButton:pressed { background: #006cbe; }
    )");
    connect(autoBtn, &QToolButton::clicked, this, &ImageViewerWidget::onAutoEnhance);
    layout->addWidget(autoBtn);

    // 降噪滑动条
    QLabel *blurTitle = new QLabel("降噪强度", panel);
    blurTitle->setStyleSheet("color: #aaa; font-size: 11px;");
    layout->addWidget(blurTitle);

    QHBoxLayout *blurRow = new QHBoxLayout();
    m_blurSlider = new QSlider(Qt::Horizontal, panel);
    m_blurSlider->setRange(0, 10);
    m_blurSlider->setValue(0);
    m_blurSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 4px; background: #444; border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 14px; height: 14px; margin: -5px 0;
            background: #0078d4; border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #0078d4; border-radius: 2px;
        }
    )");
    m_blurLabel = new QLabel("0", panel);
    m_blurLabel->setFixedWidth(20);
    m_blurLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_blurLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    connect(m_blurSlider, &QSlider::valueChanged, this, &ImageViewerWidget::onBlurSliderChanged);
    blurRow->addWidget(m_blurSlider);
    blurRow->addWidget(m_blurLabel);
    layout->addLayout(blurRow);

    // 重置按钮
    auto *resetBtn = new QToolButton(panel);
    resetBtn->setText("↩ 重置");
    resetBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    resetBtn->setFixedHeight(26);
    resetBtn->setStyleSheet(R"(
        QToolButton {
            background: #3a3a3a; color: #ccc; border: none;
            border-radius: 4px; font-size: 11px;
        }
        QToolButton:hover   { background: #444; }
        QToolButton:pressed { background: #333; }
    )");
    connect(resetBtn, &QToolButton::clicked, this, &ImageViewerWidget::onResetEnhance);
    layout->addWidget(resetBtn);

    parentLayout->addWidget(panel);
}

// ====================== 画质增强逻辑 ======================

void ImageViewerWidget::onToggleEnhancePanel()
{
    m_enhanceVisible = !m_enhanceVisible;
    m_enhancePanel->setVisible(m_enhanceVisible);
}

void ImageViewerWidget::onAutoEnhance()
{
    if (m_originalPixmap.isNull()) return;
    // 基于原始图像做增强，避免叠加
    QImage enhanced = ImageEnhancer::autoEnhance(m_originalPixmap.toImage());
    m_display->setPixmap(QPixmap::fromImage(enhanced));
    // 重置滑条（自动增强和手动降噪互斥，以自动增强为准）
    QSignalBlocker sliderBlocker(m_blurSlider);
    m_blurSlider->setValue(0);
    m_blurLabel->setText("0");
    m_blurRadius = 0;
    m_equalized  = true;
}

void ImageViewerWidget::onBlurSliderChanged(int value)
{
    m_blurLabel->setText(QString::number(value));
    m_blurRadius = value;
    applyEnhancement();
}

void ImageViewerWidget::applyEnhancement()
{
    if (m_originalPixmap.isNull()) return;
    QImage img = m_originalPixmap.toImage();
    // 先均衡化（如果已勾选），再降噪
    if (m_equalized)
        img = ImageEnhancer::histogramEqualize(img);
    if (m_blurRadius > 0)
        img = ImageEnhancer::gaussianBlur(img, m_blurRadius);
    m_display->setPixmap(QPixmap::fromImage(img));
}

void ImageViewerWidget::onResetEnhance()
{
    if (m_originalPixmap.isNull()) return;
    QSignalBlocker sliderBlocker(m_blurSlider);
    m_blurSlider->setValue(0);
    m_blurLabel->setText("0");
    m_blurRadius = 0;
    m_equalized  = false;
    m_display->setPixmap(m_originalPixmap);
}

// ====================== 打开文件/文件夹 ======================

void ImageViewerWidget::onOpenFile()
{
    QString filter = "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff *.tif *.ico);;所有文件 (*.*)";
    QString path = QFileDialog::getOpenFileName(this, "打开图片", QDir::homePath(), filter);
    if (!path.isEmpty()) openImage(path);
}

void ImageViewerWidget::onOpenFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "打开文件夹", QDir::homePath());
    if (dir.isEmpty()) return;
    loadThumbnails(dir);
    if (!m_imageFiles.isEmpty()) navigateTo(0);
}

void ImageViewerWidget::openImage(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) return;
    QString dir = fi.absolutePath();
    if (dir != m_currentDir) loadThumbnails(dir);

    int idx = m_imageFiles.indexOf(fi.absoluteFilePath());
    if (idx >= 0) navigateTo(idx);
    else {
        QPixmap pix = loadPixmapWithReader(filePath);
        if (!pix.isNull()) {
            m_originalPixmap = pix;
            m_equalized = false;
            m_display->setPixmap(pix);
            updateInfoBar(filePath);
        }
    }
}

void ImageViewerWidget::loadThumbnails(const QString &dirPath)
{
    m_currentDir = dirPath;
    m_imageFiles.clear();
    m_thumbList->clear();

    QDir dir(dirPath);
    QStringList entries = dir.entryList(s_supportedFormats, QDir::Files, QDir::Name);
    for (const QString &name : entries)
        m_imageFiles.append(dir.absoluteFilePath(name));

    for (const QString &path : std::as_const(m_imageFiles)) {
        QPixmap thumb = loadPixmapWithReader(path, QSize(130, 100));
        QIcon icon;
        if (!thumb.isNull())
            icon = QIcon(thumb);
        auto *item = new QListWidgetItem(icon, QString());
        item->setToolTip(QFileInfo(path).fileName());
        m_thumbList->addItem(item);
    }
}

void ImageViewerWidget::navigateTo(int index)
{
    if (index < 0 || index >= m_imageFiles.size()) return;
    m_currentIndex = index;

    QPixmap pix = loadPixmapWithReader(m_imageFiles[index]);
    if (pix.isNull()) return;

    // 切换图片时保存原始图像并重置增强状态
    m_originalPixmap = pix;
    m_equalized = false;
    QSignalBlocker sliderBlocker(m_blurSlider);
    m_blurSlider->setValue(0);
    m_blurLabel->setText("0");
    m_blurRadius = 0;

    m_display->setPixmap(pix);
    updateInfoBar(m_imageFiles[index]);

    QSignalBlocker thumbBlocker(m_thumbList);
    m_thumbList->setCurrentRow(index);
    m_thumbList->scrollToItem(m_thumbList->currentItem());
}

void ImageViewerWidget::onThumbnailClicked(QListWidgetItem *item)
{
    navigateTo(m_thumbList->row(item));
}

// ====================== 缩放 ======================

void ImageViewerWidget::onZoomIn()
{
    if (m_display->hasImage()) m_display->setZoomFactor(m_display->zoomFactor() * 1.2);
}
void ImageViewerWidget::onZoomOut()
{
    if (m_display->hasImage()) m_display->setZoomFactor(m_display->zoomFactor() / 1.2);
}
void ImageViewerWidget::onFitWindow()  { if (m_display->hasImage()) m_display->fitToWindow(); }
void ImageViewerWidget::onResetZoom() { if (m_display->hasImage()) m_display->resetZoom(); }

void ImageViewerWidget::onZoomSpinChanged(int value)
{
    if (m_display->hasImage()) m_display->setZoomFactor(value / 100.0);
}
void ImageViewerWidget::onZoomDisplayChanged(int percent)
{
    QSignalBlocker zoomBlocker(m_zoomSpin);
    m_zoomSpin->setValue(percent);
}

// ====================== 旋转 / 缩略图 ======================

void ImageViewerWidget::onRotateLeft()  { m_display->rotateLeft(); }
void ImageViewerWidget::onRotateRight() { m_display->rotateRight(); }

void ImageViewerWidget::onToggleThumbs()
{
    m_thumbsVisible = !m_thumbsVisible;
    m_thumbPanel->setVisible(m_thumbsVisible);
}

// ====================== 导出 ======================

void ImageViewerWidget::onExport()
{
    if (!m_display->hasImage()) return;
    QString path = QFileDialog::getSaveFileName(this, "导出图片", "image.png",
                                                "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    if (path.isEmpty()) return;
    if (!m_display->currentPixmap().save(path))
        QMessageBox::warning(this, "导出失败", "无法保存到：" + path);
    else
        m_infoLabel->setText("已导出：" + path);
}

// ====================== 信息栏 ======================

void ImageViewerWidget::updateInfoBar(const QString &filePath)
{
    QFileInfo fi(filePath);
    QImageReader reader(filePath);
    QSize sz = reader.size();
    double kb = fi.size() / 1024.0;
    QString sizeStr = kb > 1024
                      ? QString::number(kb / 1024.0, 'f', 1) + " MB"
                      : QString::number(kb, 'f', 1) + " KB";
    m_infoLabel->setText(
        QString("  %1   %2 × %3   %4   %5   %6 / %7")
            .arg(fi.fileName())
            .arg(sz.width()).arg(sz.height())
            .arg(fi.suffix().toUpper())
            .arg(sizeStr)
            .arg(m_currentIndex + 1)
            .arg(m_imageFiles.size())
    );
}

// ====================== 键盘翻页 ======================

void ImageViewerWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Up:
        navigateTo(m_currentIndex - 1); break;
    case Qt::Key_Right:
    case Qt::Key_Down:
        navigateTo(m_currentIndex + 1); break;
    default:
        QWidget::keyPressEvent(event);
    }
}
