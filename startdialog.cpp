#include "startdialog.h"

#include <ElaPushButton.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <ElaIcon.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainter>
#include <QGraphicsOpacityEffect>

StartDialog::StartDialog(QWidget *parent)
    : ElaWidget(parent)
{
    setFixedSize(900, 600);
    setWindowTitle("NotePad · 简约启程");

    setupContent();

    // 窗口淡入动画
    auto *fadeIn = new QPropertyAnimation(this, "windowOpacity");
    fadeIn->setDuration(800);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start();

    // 可选：设置窗口背景为纯黑色，与 Ela 主题分离（如果想跟随主题可省略）
    setStyleSheet("background-color: #141414;");
}

void StartDialog::onEmbarkClicked()
{
    emit embark();
    close();
}

void StartDialog::setupContent()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(60, 60, 60, 60);
    mainLayout->setSpacing(25);

    // 顶部弹性空间，将内容推到中央偏上位置
    mainLayout->addStretch();

    // Logo：使用 ElaIcon 绘制一个简单的文档图标（也可用 QLabel + QPixmap）
    ElaIcon* logoIcon = new ElaIcon(ElaIconType::FileLines, 64, this);
    logoIcon->setFixedSize(80, 80);
    mainLayout->addWidget(logoIcon, 0, Qt::AlignCenter);

    // 标题
    titleText = new ElaText("NOTEPAD", this);
    titleText->setTextPixelSize(42);
    titleText->setTextStyle(ElaTextType::Title);
    titleText->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleText);

    // 副标题
    subtitleText = new ElaText("写下你的第一行", this);
    subtitleText->setTextPixelSize(14);
    subtitleText->setTextStyle(ElaTextType::Body);
    subtitleText->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subtitleText);

    mainLayout->addSpacing(40);

    // 按钮
    embarkButton = new ElaPushButton("开始", this);
    embarkButton->setMinimumSize(200, 48);
    embarkButton->setCursor(Qt::PointingHandCursor);
    embarkButton->setLightStyle(true); // 使用浅色风格（在深色背景上更明显）
    mainLayout->addWidget(embarkButton, 0, Qt::AlignCenter);

    // 按钮呼吸动画
    auto *opacityEffect = new QGraphicsOpacityEffect(embarkButton);
    embarkButton->setGraphicsEffect(opacityEffect);
    auto *glowAnim = new QPropertyAnimation(opacityEffect, "opacity");
    glowAnim->setDuration(2000);
    glowAnim->setStartValue(0.8);
    glowAnim->setKeyValueAt(0.5, 1.0);
    glowAnim->setEndValue(0.8);
    glowAnim->setLoopCount(-1);
    glowAnim->setEasingCurve(QEasingCurve::InOutSine);
    glowAnim->start();

    // 底部弹性空间，保持垂直居中
    mainLayout->addStretch();

    connect(embarkButton, &ElaPushButton::clicked, this, &StartDialog::onEmbarkClicked);
}