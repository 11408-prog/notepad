#ifndef USERDBMANAGER_H
#define USERDBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>
#include <QRandomGenerator>

class UserDbManager : public QObject
{
    Q_OBJECT
public:
    static UserDbManager& instance();
    bool initialize();
    bool registerUser(const QString &username, const QString &password);
    bool loginUser(const QString &username, const QString &password);

private:
    explicit UserDbManager(QObject *parent = nullptr);
    QSqlDatabase m_database;
    QString hashPassword(const QString &password, const QString &salt);
    QString generateSalt();
};

#endif // USERDBMANAGER_H