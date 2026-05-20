#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <ElaWidget.h>

class ElaText;
class ElaPushButton;

class StartDialog : public ElaWidget
{
    Q_OBJECT

public:
    explicit StartDialog(QWidget *parent = nullptr);

signals:
    void embark();

private slots:
    void onEmbarkClicked();

private:
    void setupContent();

    ElaText* titleText = nullptr;
    ElaText* subtitleText = nullptr;
    ElaPushButton* embarkButton = nullptr;
};

#endif // STARTDIALOG_H