#include "userdbmanager.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QRandomGenerator>

UserDbManager& UserDbManager::instance()
{
    static UserDbManager manager;
    return manager;
}

UserDbManager::UserDbManager(QObject *parent)
    : QObject(parent)
{
    initialize();
}

bool UserDbManager::initialize()
{
    if (m_database.isOpen())
        return true;

    if (QSqlDatabase::contains("user_connection")) {
        m_database = QSqlDatabase::database("user_connection");
    } else {
        m_database = QSqlDatabase::addDatabase("QSQLITE", "user_connection");
    }

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/users.db";
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (!dir.exists())
        dir.mkpath(".");

    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qDebug() << "User DB open failed:" << m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    QString createTable = "CREATE TABLE IF NOT EXISTS users ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "username TEXT UNIQUE NOT NULL, "
                          "password_hash TEXT NOT NULL, "
                          "salt TEXT NOT NULL)";
    if (!query.exec(createTable)) {
        qDebug() << "User table creation failed:" << query.lastError().text();
        return false;
    }
    return true;
}

QString UserDbManager::generateSalt()
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString salt;
    for (int i = 0; i < 16; ++i) {
        int idx = QRandomGenerator::global()->bounded(chars.length());
        salt.append(chars.at(idx));
    }
    return salt;
}

QString UserDbManager::hashPassword(const QString &password, const QString &salt)
{
    QString combined = password + salt;
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex();
}

bool UserDbManager::registerUser(const QString &username, const QString &password)
{
    if (username.isEmpty() || password.isEmpty())
        return false;

    QSqlQuery query(m_database);
    query.prepare("SELECT id FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return false; // 用户名已存在
    }

    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);

    query.prepare("INSERT INTO users (username, password_hash, salt) VALUES (:username, :hash, :salt)");
    query.bindValue(":username", username);
    query.bindValue(":hash", hash);
    query.bindValue(":salt", salt);

    return query.exec();
}

bool UserDbManager::loginUser(const QString &username, const QString &password)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT password_hash, salt FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (!query.exec() || !query.next())
        return false;

    QString storedHash = query.value(0).toString();
    QString salt = query.value(1).toString();
    QString computedHash = hashPassword(password, salt);
    return computedHash == storedHash;
}