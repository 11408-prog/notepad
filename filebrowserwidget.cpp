#include "filebrowserwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>

FileBrowserWidget::FileBrowserWidget(QWidget *parent)
    : QWidget(parent)
{
    // 创建模型（只显示文件/目录，不显示过滤）
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    m_model->setRootPath(QDir::rootPath()); // 临时设置，后面会改为用户目录

    // 创建树视图
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIndex(m_model->index(QDir::homePath()));
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(true);
    m_treeView->setHeaderHidden(false);
    m_treeView->hideColumn(1); // 隐藏大小列
    m_treeView->hideColumn(2); // 隐藏类型列
    m_treeView->hideColumn(3); // 隐藏修改日期列（可选，如果想简洁可隐藏）

    // 创建工具栏
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setText(QDir::homePath());
    m_pathEdit->setPlaceholderText("输入路径，按回车跳转...");
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &FileBrowserWidget::onPathReturnPressed);

    m_browseButton = new QPushButton("浏览...", this);
    connect(m_browseButton, &QPushButton::clicked, this, &FileBrowserWidget::onBrowseButtonClicked);

    QHBoxLayout *toolLayout = new QHBoxLayout();
    toolLayout->addWidget(m_pathEdit);
    toolLayout->addWidget(m_browseButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolLayout);
    mainLayout->addWidget(m_treeView);
    setLayout(mainLayout);

    // 连接双击信号
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileBrowserWidget::onTreeViewDoubleClicked);
}

void FileBrowserWidget::setRootPath(const QString &path)
{
    QModelIndex idx = m_model->index(path);
    if (idx.isValid()) {
        m_treeView->setRootIndex(idx);
        m_pathEdit->setText(path);
    }
}

QString FileBrowserWidget::currentRootPath() const
{
    return m_model->filePath(m_treeView->rootIndex());
}

void FileBrowserWidget::onTreeViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QFileInfo fileInfo = m_model->fileInfo(index);
    if (fileInfo.isFile()) {
        emit fileDoubleClicked(fileInfo.absoluteFilePath());
    }
}

void FileBrowserWidget::onBrowseButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹", currentRootPath());
    if (!dir.isEmpty()) {
        setRootPath(dir);
    }
}

void FileBrowserWidget::onPathReturnPressed()
{
    QString path = m_pathEdit->text().trimmed();
    QFileInfo info(path);
    if (info.exists() && info.isDir()) {
        setRootPath(path);
    } else {
        // 可以提示路径无效，或忽略
    }
}