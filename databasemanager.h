#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include "note.h"

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool initialize();

    // 笔记操作
    int addNote(const Note &note);
    bool updateNote(const Note &note);
    bool deleteNote(int id);
    QVector<Note> getAllNotes(const QString &orderBy = "last_modified DESC");
    QVector<Note> getNotesByNotebook(int notebookId, const QString &orderBy = "last_modified DESC");

    // ✅ 新增：全文搜索
    QVector<Note> searchNotes(const QString &keyword, int notebookId = -1,
                              const QString &orderBy = "last_modified DESC");

    // 笔记本操作
    QVector<QPair<int, QString>> getAllNotebooks();
    int addNotebook(const QString &name);
    bool renameNotebook(int id, const QString &newName);
    bool deleteNotebook(int id);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H