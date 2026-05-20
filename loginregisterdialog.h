#ifndef LOGINREGISTERDIALOG_H
#define LOGINREGISTERDIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>

class LoginRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginRegisterDialog(QWidget *parent = nullptr);
    QString getLoggedInUsername() const;

private slots:
    void onLogin();
    void onRegister();
    void switchToRegister();
    void switchToLogin();

private:
    QStackedWidget *m_stackedWidget;
    // 登录页面控件
    QWidget *m_loginPage;
    QLineEdit *m_loginUsername;
    QLineEdit *m_loginPassword;
    QPushButton *m_loginButton;
    QPushButton *m_toRegisterButton;
    // 注册页面控件
    QWidget *m_registerPage;
    QLineEdit *m_regUsername;
    QLineEdit *m_regPassword;
    QLineEdit *m_regConfirm;
    QPushButton *m_registerButton;
    QPushButton *m_toLoginButton;

    QString m_loggedInUsername;
    void setupLoginPage();
    void setupRegisterPage();
};

#endif // LOGINREGISTERDIALOG_H