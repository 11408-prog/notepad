#ifndef NOTE_H
#define NOTE_H

#include <QString>
#include <QDateTime>

struct Note
{
    int id = -1;
    QString title;
    QString content;
    QDateTime created;
    QDateTime lastModified;
    int notebookId = 0;   // 所属笔记本 ID，0 为默认笔记本

    Note() = default;
    Note(const QString &t, const QString &c)
        : title(t), content(c)
    {
        created = QDateTime::currentDateTime();
        lastModified = created;
        notebookId = 0;
    }
};

#endif // NOTE_H