#include "loginregisterdialog.h"
#include "userdbmanager.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLabel>


LoginRegisterDialog::LoginRegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Login / Register");
    setMinimumSize(300, 200);

    m_stackedWidget = new QStackedWidget(this);
    setupLoginPage();
    setupRegisterPage();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_stackedWidget);
    setLayout(mainLayout);

    switchToLogin(); // 默认显示登录页
}

void LoginRegisterDialog::setupLoginPage()
{
    m_loginPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_loginPage);

    layout->addWidget(new QLabel("Username:"));
    m_loginUsername = new QLineEdit;
    layout->addWidget(m_loginUsername);

    layout->addWidget(new QLabel("Password:"));
    m_loginPassword = new QLineEdit;
    m_loginPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_loginPassword);

    m_loginButton = new QPushButton("Login");
    m_toRegisterButton = new QPushButton("Create Account");
    layout->addWidget(m_loginButton);
    layout->addWidget(m_toRegisterButton);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginRegisterDialog::onLogin);
    connect(m_toRegisterButton, &QPushButton::clicked, this, &LoginRegisterDialog::switchToRegister);

    m_stackedWidget->addWidget(m_loginPage);
}

void LoginRegisterDialog::setupRegisterPage()
{
    m_registerPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_registerPage);

    layout->addWidget(new QLabel("Username:"));
    m_regUsername = new QLineEdit;
    layout->addWidget(m_regUsername);

    layout->addWidget(new QLabel("Password:"));
    m_regPassword = new QLineEdit;
    m_regPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_regPassword);

    layout->addWidget(new QLabel("Confirm Password:"));
    m_regConfirm = new QLineEdit;
    m_regConfirm->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_regConfirm);

    m_registerButton = new QPushButton("Register");
    m_toLoginButton = new QPushButton("Back to Login");
    layout->addWidget(m_registerButton);
    layout->addWidget(m_toLoginButton);

    connect(m_registerButton, &QPushButton::clicked, this, &LoginRegisterDialog::onRegister);
    connect(m_toLoginButton, &QPushButton::clicked, this, &LoginRegisterDialog::switchToLogin);

    m_stackedWidget->addWidget(m_registerPage);
}

void LoginRegisterDialog::switchToRegister()
{
    m_stackedWidget->setCurrentWidget(m_registerPage);
}

void LoginRegisterDialog::switchToLogin()
{
    m_stackedWidget->setCurrentWidget(m_loginPage);
    m_loginUsername->clear();
    m_loginPassword->clear();
}

void LoginRegisterDialog::onLogin()
{
    QString username = m_loginUsername->text().trimmed();
    QString password = m_loginPassword->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please fill in both fields.");
        return;
    }
    if (UserDbManager::instance().loginUser(username, password)) {
        m_loggedInUsername = username;
        accept(); // 关闭对话框并返回 Accepted
    } else {
        QMessageBox::warning(this, "Login Failed", "Invalid username or password.");
    }
}

void LoginRegisterDialog::onRegister()
{
    QString username = m_regUsername->text().trimmed();
    QString password = m_regPassword->text();
    QString confirm = m_regConfirm->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Username and password cannot be empty.");
        return;
    }
    if (password != confirm) {
        QMessageBox::warning(this, "Error", "Passwords do not match.");
        return;
    }
    if (password.length() < 3) {
        QMessageBox::warning(this, "Error", "Password must be at least 3 characters.");
        return;
    }

    if (UserDbManager::instance().registerUser(username, password)) {
        QMessageBox::information(this, "Success", "Registration successful. Please log in.");
        switchToLogin();
        m_loginUsername->setText(username);
        m_loginPassword->clear();
    } else {
        QMessageBox::warning(this, "Registration Failed", "Username already exists or other error.");
    }
}

QString LoginRegisterDialog::getLoggedInUsername() const
{
    return m_loggedInUsername;
}