#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Login");
    setModal(true); // 模态对话框

    m_usernameEdit = new QLineEdit;
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_loginButton = new QPushButton("Login");
    m_cancelButton = new QPushButton("Cancel");

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel("Username:"));
    layout->addWidget(m_usernameEdit);
    layout->addWidget(new QLabel("Password:"));
    layout->addWidget(m_passwordEdit);
    layout->addWidget(m_loginButton);
    layout->addWidget(m_cancelButton);
    setLayout(layout);
}

void LoginDialog::onLoginClicked()
{
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();

    // 简单验证：用户名 "admin" 密码 "123"
    if (username == "admin" && password == "123") {
        accept(); // 关闭对话框并返回 Accepted
    } else {
        QMessageBox::warning(this, "Login Failed", "Invalid username or password.");
    }
}

QString LoginDialog::getUsername() const
{
    return m_usernameEdit->text();
}