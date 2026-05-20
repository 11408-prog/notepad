#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QColor>
#include <QFont>
#include <QWidget>

class ElaComboBox;
class ElaLineEdit;
class QLabel;
class QPushButton;
class QCheckBox;

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

    void loadFromSettings();
    void setThemeDark(bool dark);

signals:
    void themeChanged(bool dark);
    void editorSettingsChanged(bool showLineNumbers, bool highlightCurrentLine, const QColor &highlightColor);
    void editorFontChanged(const QFont &font);

private:
    QWidget* createSection(const QString &title);
    ElaLineEdit* createPathEdit(const QString &placeholder);
    void addPathRow(QWidget *section, const QString &labelText, ElaLineEdit *edit, const QString &settingsKey);
    void saveEditorSettings();
    void updateColorPreview();

    ElaComboBox *m_themeComboBox = nullptr;
    QCheckBox *m_lineNumberCheckBox = nullptr;
    QCheckBox *m_highlightLineCheckBox = nullptr;
    QPushButton *m_highlightColorButton = nullptr;
    QPushButton *m_fontButton = nullptr;
    ElaLineEdit *m_compilerPathEdit = nullptr;
    ElaLineEdit *m_devCppPathEdit = nullptr;

    QColor m_highlightColor;
    QFont m_editorFont;
};

#endif // SETTINGSWIDGET_H
