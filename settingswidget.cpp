#include "settingswidget.h"
#include "appsettings.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <ElaComboBox.h>
#include <ElaLineEdit.h>
#include <ElaText.h>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    auto *titleLabel = new ElaText("设置", this);
    titleLabel->setTextPixelSize(26);
    titleLabel->setTextStyle(ElaTextType::Title);
    mainLayout->addWidget(titleLabel);

    QWidget *appearanceSection = createSection("外观");
    auto *themeLayout = new QHBoxLayout();
    themeLayout->setContentsMargins(0, 0, 0, 0);
    auto *themeLabel = new QLabel("主题", appearanceSection);
    m_themeComboBox = new ElaComboBox(appearanceSection);
    m_themeComboBox->addItem("深色");
    m_themeComboBox->addItem("浅色");
    themeLayout->addWidget(themeLabel);
    themeLayout->addWidget(m_themeComboBox);
    themeLayout->addStretch();
    static_cast<QVBoxLayout*>(appearanceSection->layout())->addLayout(themeLayout);
    mainLayout->addWidget(appearanceSection);

    QWidget *editorSection = createSection("编辑器");
    auto *editorOptionLayout = new QHBoxLayout();
    editorOptionLayout->setContentsMargins(0, 0, 0, 0);
    m_lineNumberCheckBox = new QCheckBox("显示行号", editorSection);
    m_highlightLineCheckBox = new QCheckBox("高亮当前行", editorSection);
    editorOptionLayout->addWidget(m_lineNumberCheckBox);
    editorOptionLayout->addWidget(m_highlightLineCheckBox);
    editorOptionLayout->addStretch();
    static_cast<QVBoxLayout*>(editorSection->layout())->addLayout(editorOptionLayout);

    auto *editorActionLayout = new QHBoxLayout();
    editorActionLayout->setContentsMargins(0, 0, 0, 0);
    m_fontButton = new QPushButton("编辑器字体", editorSection);
    m_highlightColorButton = new QPushButton("当前行颜色", editorSection);
    editorActionLayout->addWidget(m_fontButton);
    editorActionLayout->addWidget(m_highlightColorButton);
    editorActionLayout->addStretch();
    static_cast<QVBoxLayout*>(editorSection->layout())->addLayout(editorActionLayout);
    mainLayout->addWidget(editorSection);

    QWidget *toolsSection = createSection("工具路径");
    m_compilerPathEdit = createPathEdit("默认使用 PATH 中的 g++");
    addPathRow(toolsSection, "C++ 编译器", m_compilerPathEdit, "compilerPath");
    m_devCppPathEdit = createPathEdit("例如 Dev-Cpp/devcpp.exe");
    addPathRow(toolsSection, "Dev-C++", m_devCppPathEdit, "tools/devCppPath");
    mainLayout->addWidget(toolsSection);

    mainLayout->addStretch();

    connect(m_themeComboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged), this, [this](int index) {
        const bool dark = (index == 0);
        appSettings().setValue("theme", dark ? "Dark" : "Light");
        emit themeChanged(dark);
    });
    connect(m_lineNumberCheckBox, &QCheckBox::toggled, this, &SettingsWidget::saveEditorSettings);
    connect(m_highlightLineCheckBox, &QCheckBox::toggled, this, &SettingsWidget::saveEditorSettings);
    connect(m_highlightColorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(m_highlightColor, this, "选择高亮颜色", QColorDialog::ShowAlphaChannel);
        if (!color.isValid())
            return;
        m_highlightColor = color;
        updateColorPreview();
        saveEditorSettings();
    });
    connect(m_fontButton, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QFont font = QFontDialog::getFont(&ok, m_editorFont, this, "选择编辑器字体");
        if (!ok)
            return;
        m_editorFont = font;
        appSettings().setValue("editor/font", font);
        emit editorFontChanged(font);
    });

    loadFromSettings();
}

void SettingsWidget::loadFromSettings()
{
    QSettings& settings = appSettings();
    const QString theme = settings.value("theme", "Light").toString();
    setThemeDark(theme == "Dark");

    const bool showLineNumbers = settings.value("editor/showLineNumbers", true).toBool();
    const bool highlightLine = settings.value("editor/highlightCurrentLine", true).toBool();
    const QColor defaultColor(80, 80, 80, 80);
    m_highlightColor = settings.value("editor/highlightColor", defaultColor).value<QColor>();

    QFont defaultFont("Cascadia Code", 11);
    defaultFont.setStyleHint(QFont::Monospace);
    m_editorFont = settings.value("editor/font", defaultFont).value<QFont>();

    m_lineNumberCheckBox->blockSignals(true);
    m_highlightLineCheckBox->blockSignals(true);
    m_lineNumberCheckBox->setChecked(showLineNumbers);
    m_highlightLineCheckBox->setChecked(highlightLine);
    m_lineNumberCheckBox->blockSignals(false);
    m_highlightLineCheckBox->blockSignals(false);

    m_compilerPathEdit->setText(settings.value("compilerPath").toString());
    m_devCppPathEdit->setText(settings.value("tools/devCppPath").toString());
    updateColorPreview();
}

void SettingsWidget::setThemeDark(bool dark)
{
    if (!m_themeComboBox)
        return;
    m_themeComboBox->blockSignals(true);
    m_themeComboBox->setCurrentIndex(dark ? 0 : 1);
    m_themeComboBox->blockSignals(false);
}

QWidget* SettingsWidget::createSection(const QString &title)
{
    auto *section = new QFrame(this);
    section->setObjectName("settingsSection");
    section->setStyleSheet(R"(
        QFrame#settingsSection {
            border: 1px solid rgba(128, 128, 128, 0.28);
            border-radius: 8px;
            background: rgba(128, 128, 128, 0.04);
        }
    )");

    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(12);

    auto *label = new ElaText(title, section);
    label->setTextPixelSize(16);
    layout->addWidget(label);
    return section;
}

ElaLineEdit* SettingsWidget::createPathEdit(const QString &placeholder)
{
    auto *edit = new ElaLineEdit(this);
    edit->setPlaceholderText(placeholder);
    edit->setMinimumWidth(420);
    return edit;
}

void SettingsWidget::addPathRow(QWidget *section, const QString &labelText, ElaLineEdit *edit, const QString &settingsKey)
{
    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);

    auto *label = new QLabel(labelText, section);
    label->setMinimumWidth(90);
    auto *browseButton = new QPushButton("浏览", section);
    auto *clearButton = new QPushButton("清空", section);

    row->addWidget(label);
    row->addWidget(edit, 1);
    row->addWidget(browseButton);
    row->addWidget(clearButton);
    static_cast<QVBoxLayout*>(section->layout())->addLayout(row);

    connect(edit, &ElaLineEdit::editingFinished, this, [edit, settingsKey]() {
        appSettings().setValue(settingsKey, edit->text().trimmed());
    });
    connect(browseButton, &QPushButton::clicked, this, [this, edit, settingsKey]() {
        QString path = QFileDialog::getOpenFileName(this, "选择程序", edit->text().isEmpty() ? QString() : edit->text());
        if (path.isEmpty())
            return;
        edit->setText(path);
        appSettings().setValue(settingsKey, path);
    });
    connect(clearButton, &QPushButton::clicked, this, [edit, settingsKey]() {
        edit->clear();
        appSettings().remove(settingsKey);
    });
}

void SettingsWidget::saveEditorSettings()
{
    const bool showLineNumbers = m_lineNumberCheckBox->isChecked();
    const bool highlightLine = m_highlightLineCheckBox->isChecked();

    QSettings& settings = appSettings();
    settings.setValue("editor/showLineNumbers", showLineNumbers);
    settings.setValue("editor/highlightCurrentLine", highlightLine);
    settings.setValue("editor/highlightColor", m_highlightColor);

    emit editorSettingsChanged(showLineNumbers, highlightLine, m_highlightColor);
}

void SettingsWidget::updateColorPreview()
{
    if (!m_highlightColorButton)
        return;

    const QString color = m_highlightColor.name(QColor::HexArgb);
    m_highlightColorButton->setStyleSheet(QString("QPushButton { border-left: 18px solid %1; padding: 5px 10px; }").arg(color));
}
