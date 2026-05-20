#ifndef FILEBROWSERWIDGET_H
#define FILEBROWSERWIDGET_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>

class FileBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileBrowserWidget(QWidget *parent = nullptr);

    // 设置根路径（例如用户主目录）
    void setRootPath(const QString &path);
    QString currentRootPath() const;

signals:
    // 双击文件时发出信号，携带文件完整路径
    void fileDoubleClicked(const QString &filePath);

private slots:
    void onTreeViewDoubleClicked(const QModelIndex &index);
    void onBrowseButtonClicked();
    void onPathReturnPressed();

private:
    QTreeView *m_treeView;
    QFileSystemModel *m_model;
    QLineEdit *m_pathEdit;
    QPushButton *m_browseButton;
};

#endif // FILEBROWSERWIDGET_H