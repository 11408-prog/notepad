#include "databasemanager.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

// ✅ 辅助函数：带重试的 SQL 执行
static bool execWithRetry(QSqlQuery &query, int maxRetries = 2) {
    for (int i = 0; i <= maxRetries; ++i) {
        if (query.exec())
            return true;
        // SQLITE_BUSY 错误码为 5
        if (query.lastError().nativeErrorCode() == "5" && i < maxRetries) {
            QThread::msleep(100 * (i + 1));
            continue;
        }
        qDebug() << "SQL Error:" << query.lastError().text();
        return false;
    }
    return false;
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    initialize();
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) m_database.close();
}

bool DatabaseManager::initialize()
{
    if (m_database.isOpen()) return true;

    if (QSqlDatabase::contains("qt_sql_default_connection"))
        m_database = QSqlDatabase::database("qt_sql_default_connection");
    else
        m_database = QSqlDatabase::addDatabase("QSQLITE");

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/notes.db";
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    if (!dir.exists()) dir.mkpath(".");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qDebug() << "Error: Failed to open database:" << m_database.lastError().text();
        return false;
    }

    // ✅ 优化：设置忙等待超时和 WAL 模式
    m_database.setConnectOptions("QSQLITE_BUSY_TIMEOUT=3000");
    QSqlQuery query;
    query.exec("PRAGMA journal_mode=WAL");
    query.exec("PRAGMA synchronous=NORMAL");
    query.exec("PRAGMA cache_size=10000");

    // 创建或升级 notes 表
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='notes'");
    if (!query.next()) {
        query.exec("CREATE TABLE notes ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "title TEXT NOT NULL, "
                   "content TEXT, "
                   "created DATETIME DEFAULT CURRENT_TIMESTAMP, "
                   "last_modified DATETIME DEFAULT CURRENT_TIMESTAMP, "
                   "notebook_id INTEGER DEFAULT 0)");
    } else {
        query.exec("PRAGMA table_info(notes)");
        bool hasCreated = false, hasNotebookId = false;
        while (query.next()) {
            QString col = query.value(1).toString();
            if (col == "created") hasCreated = true;
            else if (col == "notebook_id") hasNotebookId = true;
        }
        if (!hasCreated) query.exec("ALTER TABLE notes ADD COLUMN created DATETIME DEFAULT CURRENT_TIMESTAMP");
        if (!hasNotebookId) query.exec("ALTER TABLE notes ADD COLUMN notebook_id INTEGER DEFAULT 0");
    }

    // 创建 notebooks 表
    query.exec("CREATE TABLE IF NOT EXISTS notebooks ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT NOT NULL UNIQUE)");
    query.exec("INSERT OR IGNORE INTO notebooks (id, name) VALUES (0, '默认笔记本')");

    // ✅ 新增：创建索引提升查询性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_notebook_id ON notes(notebook_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_notes_last_modified ON notes(last_modified DESC)");

    qDebug() << "Database initialized at:" << dbPath;
    return true;
}

int DatabaseManager::addNote(const Note &note)
{
    QSqlDatabase::database().transaction();  // ✅ 事务
    QSqlQuery query;
    query.prepare("INSERT INTO notes (title, content, created, last_modified, notebook_id) "
                  "VALUES (:title, :content, :created, :last_modified, :notebook_id)");
    query.bindValue(":title", note.title);
    query.bindValue(":content", note.content);
    query.bindValue(":created", note.created);
    query.bindValue(":last_modified", note.lastModified);
    query.bindValue(":notebook_id", note.notebookId);

    if (!execWithRetry(query)) {
        QSqlDatabase::database().rollback();
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    QSqlDatabase::database().commit();
    return newId;
}

bool DatabaseManager::updateNote(const Note &note)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.prepare("UPDATE notes SET title=:title, content=:content, last_modified=:last_modified, "
                  "notebook_id=:notebook_id WHERE id=:id");
    query.bindValue(":title", note.title);
    query.bindValue(":content", note.content);
    query.bindValue(":last_modified", note.lastModified);
    query.bindValue(":notebook_id", note.notebookId);
    query.bindValue(":id", note.id);

    bool ok = execWithRetry(query);
    if (ok) QSqlDatabase::database().commit();
    else QSqlDatabase::database().rollback();
    return ok;
}

bool DatabaseManager::deleteNote(int id)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.prepare("DELETE FROM notes WHERE id=:id");
    query.bindValue(":id", id);
    bool ok = execWithRetry(query);
    if (ok) QSqlDatabase::database().commit();
    else QSqlDatabase::database().rollback();
    return ok;
}

QVector<Note> DatabaseManager::getAllNotes(const QString &orderBy)
{
    QVector<Note> notes;
    QSqlQuery query;
    // ✅ 白名单校验
    static const QStringList allowed = {"last_modified DESC", "created DESC", "title ASC"};
    QString safeOrder = allowed.contains(orderBy) ? orderBy : "last_modified DESC";
    query.exec("SELECT id, title, content, created, last_modified, notebook_id FROM notes ORDER BY " + safeOrder);
    while (query.next()) {
        Note note;
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.content = query.value(2).toString();
        note.created = query.value(3).toDateTime();
        note.lastModified = query.value(4).toDateTime();
        note.notebookId = query.value(5).toInt();
        notes.append(note);
    }
    return notes;
}

QVector<Note> DatabaseManager::getNotesByNotebook(int notebookId, const QString &orderBy)
{
    QVector<Note> notes;
    QSqlQuery query;
    static const QStringList allowed = {"last_modified DESC", "created DESC", "title ASC"};
    QString safeOrder = allowed.contains(orderBy) ? orderBy : "last_modified DESC";
    query.prepare(QString("SELECT id, title, content, created, last_modified, notebook_id FROM notes "
                          "WHERE notebook_id=:nid ORDER BY %1").arg(safeOrder));
    query.bindValue(":nid", notebookId);
    if (query.exec()) {
        while (query.next()) {
            Note note;
            note.id = query.value(0).toInt();
            note.title = query.value(1).toString();
            note.content = query.value(2).toString();
            note.created = query.value(3).toDateTime();
            note.lastModified = query.value(4).toDateTime();
            note.notebookId = query.value(5).toInt();
            notes.append(note);
        }
    }
    return notes;
}

// ✅ 新增：全文搜索实现
QVector<Note> DatabaseManager::searchNotes(const QString &keyword, int notebookId, const QString &orderBy)
{
    QVector<Note> notes;
    if (keyword.isEmpty()) {
        return notebookId == -1 ? getAllNotes(orderBy) : getNotesByNotebook(notebookId, orderBy);
    }

    QSqlQuery query;
    QString sql = "SELECT id, title, content, created, last_modified, notebook_id FROM notes WHERE ";
    sql += "(title LIKE :kw OR content LIKE :kw) ";
    if (notebookId != -1) {
        sql += "AND notebook_id = :nid ";
    }
    static const QStringList allowed = {"last_modified DESC", "created DESC", "title ASC"};
    QString safeOrder = allowed.contains(orderBy) ? orderBy : "last_modified DESC";
    sql += "ORDER BY " + safeOrder;

    query.prepare(sql);
    query.bindValue(":kw", "%" + keyword + "%");
    if (notebookId != -1) {
        query.bindValue(":nid", notebookId);
    }

    if (query.exec()) {
        while (query.next()) {
            Note note;
            note.id = query.value(0).toInt();
            note.title = query.value(1).toString();
            note.content = query.value(2).toString();
            note.created = query.value(3).toDateTime();
            note.lastModified = query.value(4).toDateTime();
            note.notebookId = query.value(5).toInt();
            notes.append(note);
        }
    }
    return notes;
}

// ====================== 笔记本操作 ======================
QVector<QPair<int, QString>> DatabaseManager::getAllNotebooks()
{
    QVector<QPair<int, QString>> notebooks;
    QSqlQuery query("SELECT id, name FROM notebooks ORDER BY id");
    while (query.next())
        notebooks.append({query.value(0).toInt(), query.value(1).toString()});
    return notebooks;
}

int DatabaseManager::addNotebook(const QString &name)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.prepare("INSERT INTO notebooks (name) VALUES (:name)");
    query.bindValue(":name", name);
    if (!execWithRetry(query)) {
        QSqlDatabase::database().rollback();
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    QSqlDatabase::database().commit();
    return newId;
}

bool DatabaseManager::renameNotebook(int id, const QString &newName)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.prepare("UPDATE notebooks SET name=:name WHERE id=:id AND id!=0");
    query.bindValue(":name", newName);
    query.bindValue(":id", id);
    bool ok = execWithRetry(query);
    if (ok) QSqlDatabase::database().commit();
    else QSqlDatabase::database().rollback();
    return ok;
}

bool DatabaseManager::deleteNotebook(int id)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;
    query.prepare("UPDATE notes SET notebook_id=0 WHERE notebook_id=:id");
    query.bindValue(":id", id);
    execWithRetry(query); // 忽略错误，继续删除笔记本
    query.prepare("DELETE FROM notebooks WHERE id=:id AND id!=0");
    query.bindValue(":id", id);
    bool ok = execWithRetry(query);
    if (ok) QSqlDatabase::database().commit();
    else QSqlDatabase::database().rollback();
    return ok;
}