#include "mainwindow.h"

#include "codeeditor.h"
#include "compilerrunner.h"
#include "dataanalysiswidget.h"
#include "filebrowserwidget.h"
#include "imageviewerwidget.h"
#include "settingswidget.h"
#include "terminalwidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>

#include <ElaIcon.h>
#include <ElaLineEdit.h>
#include <ElaMenu.h>
#include <ElaMenuBar.h>
#include <ElaPushButton.h>
#include <ElaStatusBar.h>
#include <ElaTabWidget.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <ElaToolBar.h>
#include <ElaToolButton.h>

#include <Qsci/qscilexercpp.h>

void MainWindow::setupUI()
{
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode mode) {
        if (settingPage_) settingPage_->setThemeDark(mode == ElaThemeType::Dark);
        if (terminalPage_) terminalPage_->applyTheme(mode);
        applyUIStyles();
    });
}

void MainWindow::applyUIStyles()
{
    bool isDark = (eTheme->getThemeMode() == ElaThemeType::Dark);
    QString darkBorder = isDark ? "#3c3c3c" : "#d0d0d0";
    QString darkBg = isDark ? "#2b2b2b" : "#f5f5f5";
    QString darkItemBorder = isDark ? "#404040" : "#e0e0e0";
    QString darkHoverBg = isDark ? "#3a3a3a" : "#e8f0fe";
    QString darkSelectedBg = isDark ? "#0d47a1" : "#0078d4";
    QString darkSelectedFg = isDark ? "#ffffff" : "#ffffff";
    QString listStyle = QString(R"(
        QListWidget {
            border: 1px solid %1;
            border-radius: 4px;
            background-color: %2;
            outline: none;
        }
        QListWidget::item {
            padding: 6px 8px;
            border-bottom: 1px solid %3;
        }
        QListWidget::item:hover {
            background-color: %4;
        }
        QListWidget::item:selected {
            background-color: %5;
            color: %6;
        }
    )").arg(darkBorder, darkBg, darkItemBorder, darkHoverBg, darkSelectedBg, darkSelectedFg);
    if (listWidget_) listWidget_->setStyleSheet(listStyle);
    if (notebookListWidget_) notebookListWidget_->setStyleSheet(listStyle);
    if (tabWidget_) tabWidget_->setStyleSheet(R"(
        QTabWidget::pane { border: none; background: transparent; }
        QTabBar::tab { padding: 8px 16px; margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }
    )");
}

void MainWindow::setupConnections()
{
    connect(listWidget_, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);
    connect(listWidget_, &QListWidget::customContextMenuRequested, this, &MainWindow::onListWidgetCustomContextMenuRequested);
    connect(searchBox_, &ElaLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_compilerRunner, &CompilerRunner::outputReady, terminalPage_, &TerminalWidget::appendPlainText);
    connect(m_compilerRunner, &CompilerRunner::compileFailed, this, &MainWindow::parseCompilerErrors);
    connect(m_compilerRunner, &CompilerRunner::compileSucceeded, this, [this](const QString &program) {
        CodeEditor *editor = currentEditor();
        if (editor) editor->clearErrorMarkers();

#ifdef Q_OS_WIN
        bool success = QProcess::startDetached("cmd.exe", QStringList() << "/c" << "start" << "" << program << "& pause");
#else
        bool success = QProcess::startDetached(program);
#endif
        if (!success) terminalPage_->appendPlainText("[错误] 无法启动程序。");
    });
    listWidget_->installEventFilter(this);
}

void MainWindow::setupNavigation()
{
    fileBrowserPage_ = new QWidget(this);
    QVBoxLayout *fileBrowserLayout = new QVBoxLayout(fileBrowserPage_);
    fileBrowserLayout->setContentsMargins(0,0,0,0);
    m_fileBrowserWidget = new FileBrowserWidget(fileBrowserPage_);
    m_fileBrowserWidget->setRootPath(QDir::homePath());
    fileBrowserLayout->addWidget(m_fileBrowserWidget);
    connect(m_fileBrowserWidget, &FileBrowserWidget::fileDoubleClicked, this, &MainWindow::onFileDoubleClicked);

    notebookPage_ = new QWidget(this);
    QVBoxLayout *notebookLayout = new QVBoxLayout(notebookPage_);
    notebookLayout->setContentsMargins(8,8,8,8);
    QHBoxLayout *headerLayout = new QHBoxLayout();
    ElaText *headerLabel = new ElaText("笔记本", notebookPage_);
    headerLabel->setTextPixelSize(18);
    QFont boldFont = headerLabel->font(); boldFont.setBold(true); headerLabel->setFont(boldFont);
    addNotebookBtn_ = new ElaPushButton("新建", notebookPage_);
    addNotebookBtn_->setMinimumSize(70,30);
    addNotebookBtn_->setStyleSheet("QPushButton { border-radius: 4px; }");
    connect(addNotebookBtn_, &ElaPushButton::clicked, this, &MainWindow::addNewNotebook);
    headerLayout->addWidget(headerLabel); headerLayout->addStretch(); headerLayout->addWidget(addNotebookBtn_);
    notebookListWidget_ = new QListWidget(notebookPage_);
    notebookListWidget_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(notebookListWidget_, &QListWidget::itemClicked, this, &MainWindow::onNotebookSelected);
    connect(notebookListWidget_, &QListWidget::customContextMenuRequested, this, &MainWindow::onNotebookCustomContextMenuRequested);
    notebookLayout->addLayout(headerLayout); notebookLayout->addWidget(notebookListWidget_);

    editorPage_ = new QWidget(this);
    QVBoxLayout *editorVLayout = new QVBoxLayout(editorPage_);
    editorVLayout->setContentsMargins(0,0,0,0); editorVLayout->setSpacing(0);
    notesDock_ = new ElaDockWidget("笔记列表", this);
    notesDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    notesDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget *sidebarContainer = new QWidget(notesDock_);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarContainer);
    sidebarLayout->setContentsMargins(5,5,5,5); sidebarLayout->setSpacing(0);
    listWidget_ = new QListWidget(sidebarContainer);
    listWidget_->setContextMenuPolicy(Qt::CustomContextMenu);
    listWidget_->setAlternatingRowColors(true);
    sidebarLayout->addWidget(listWidget_);
    notesDock_->setWidget(sidebarContainer);
    connect(notesDock_, &ElaDockWidget::visibilityChanged, this, [this](bool visible) { if(showNotesAction) showNotesAction->setChecked(visible); });
    tabWidget_ = new ElaTabWidget(editorPage_);
    tabWidget_->setTabsClosable(true); tabWidget_->setMovable(false);
    connect(tabWidget_, &ElaTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(tabWidget_, &ElaTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    QWidget *editorContent = new QWidget(editorPage_);
    QHBoxLayout *contentLayout = new QHBoxLayout(editorContent);
    contentLayout->setContentsMargins(0,0,0,0); contentLayout->setSpacing(0);
    contentLayout->addWidget(notesDock_); contentLayout->addWidget(tabWidget_);
    contentLayout->setStretchFactor(notesDock_, 0); contentLayout->setStretchFactor(tabWidget_, 1);
    editorVLayout->addWidget(editorContent);

    terminalPage_ = new TerminalWidget(this);
    terminalPage_->applyTheme(eTheme->getThemeMode());

    settingPage_ = new SettingsWidget(this);
    settingPage_->setThemeDark(eTheme->getThemeMode() == ElaThemeType::Dark);
    connect(settingPage_, &SettingsWidget::themeChanged, this, [](bool dark) {
        eTheme->setThemeMode(dark ? ElaThemeType::Dark : ElaThemeType::Light);
    });
    connect(settingPage_, &SettingsWidget::editorSettingsChanged, this,
            [this](bool showLineNumbers, bool highlightCurrentLine, const QColor &highlightColor) {
                m_showLineNumbers = showLineNumbers;
                m_highlightCurrentLine = highlightCurrentLine;
                m_currentHighlightColor = highlightColor;
                applyEditorSettingsToAllEditors();
            });
    connect(settingPage_, &SettingsWidget::editorFontChanged, this, [this](const QFont &font) {
        applyFontToAllEditors(font);
        saveFontSettings(font);
    });

    addPageNode("文件", fileBrowserPage_, ElaIconType::FolderOpen);
    fileBrowserPageKey_ = fileBrowserPage_->property("ElaPageKey").toString();
    addExpanderNode("工作区", _workspaceKey, ElaIconType::Folder);
    addPageNode("笔记本", notebookPage_, _workspaceKey, ElaIconType::Notebook);
    notebookPageKey_ = notebookPage_->property("ElaPageKey").toString();
    addPageNode("编辑器", editorPage_, _workspaceKey, ElaIconType::FileLines);
    editorPageKey_ = editorPage_->property("ElaPageKey").toString();
    expandNavigationNode(_workspaceKey);
    addPageNode("终端", terminalPage_, ElaIconType::Terminal);
    terminalPageKey_ = terminalPage_->property("ElaPageKey").toString();
    dataAnalysisPage_ = new DataAnalysisWidget(this);
    addPageNode("数据分析", dataAnalysisPage_, ElaIconType::ChartLine);
    dataAnalysisPageKey_ = dataAnalysisPage_->property("ElaPageKey").toString();
    imagePage_ = new ImageViewerWidget(this);
    addPageNode("图片", imagePage_, ElaIconType::Image);
    imagePageKey_ = imagePage_->property("ElaPageKey").toString();
    addFooterNode("设置", settingPage_, settingPageKey_, 0, ElaIconType::GearComplex);
    addFooterNode("关于", nullptr, _aboutKey, 0, ElaIconType::Info);
    connect(this, &ElaWindow::navigationNodeClicked, this, [this](ElaNavigationType::NavigationNodeType, QString nodeKey) {
        if (nodeKey == _aboutKey) QMessageBox::about(this, "关于 NotePad", "NotePad 终极稳定版\n基于 Qt 6.11.0");
    });
}

CodeEditor* MainWindow::createEditorWithContent(const QString &content)
{
    CodeEditor *editor = new CodeEditor(this);
    editor->setLexer(new QsciLexerCPP(this));
    editor->setText(content);
    editor->setFont(m_editorFont);
    editor->setMarginType(1, QsciScintilla::NumberMargin);
    applyEditorSettings(editor);
    int index = tabWidget_->addTab(editor, content.isEmpty() ? "未命名" : "笔记");
    tabWidget_->setCurrentIndex(index);
    return editor;
}

void MainWindow::setupMenuBar()
{
    menuBar_ = new ElaMenuBar(this);
    menuBar_->setFixedHeight(30);
    setMenuBar(menuBar_);
    ElaMenu *fileMenu = new ElaMenu("文件(&F)", this);
    menuBar_->addMenu(fileMenu);

    QAction *newAction = fileMenu->addElaIconAction(ElaIconType::FileLines, "新建(&N)", QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::on_actionNew_triggered);

    QAction *newCodeNoteAction = fileMenu->addElaIconAction(ElaIconType::FileCode, "新建代码笔记(&C)", QKeySequence("Ctrl+Shift+N"));
    connect(newCodeNoteAction, &QAction::triggered, this, &MainWindow::on_actionNewCodeNote_triggered);

    QAction *openAction = fileMenu->addElaIconAction(ElaIconType::FolderOpen, "打开(&O)...", QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::on_actionOpen_triggered);

    QAction *saveAction = fileMenu->addElaIconAction(ElaIconType::FloppyDisk, "保存(&S)", QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::on_actionSave_triggered);

    fileMenu->addAction("另存为(&A)...", QKeySequence::SaveAs, this, &MainWindow::on_actionSaveAs_triggered);

    QAction *openInDevCppAction = fileMenu->addAction("编译(&D)");
    connect(openInDevCppAction, &QAction::triggered, this, &MainWindow::on_actionOpenInDevCpp_triggered);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addElaIconAction(ElaIconType::DoorOpen, "退出(&X)", QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::on_actionExit_triggered);

    ElaMenu *editMenu = new ElaMenu("编辑(&E)", this);
    menuBar_->addMenu(editMenu);

    QAction *undoAction = editMenu->addElaIconAction(ElaIconType::ArrowRotateLeft, "撤销(&U)", QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, &MainWindow::on_actionUndo_triggered);

    QAction *redoAction = editMenu->addElaIconAction(ElaIconType::ArrowRotateRight, "重做(&R)", QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, &MainWindow::on_actionRedo_triggered);

    editMenu->addSeparator();
    editMenu->addAction("编译(&B)", QKeySequence("Ctrl+B"), this, &MainWindow::on_compile_triggered);
    editMenu->addSeparator();

    QAction *cutAction = editMenu->addElaIconAction(ElaIconType::Scissors, "剪切(&T)", QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &MainWindow::on_actionCut_triggered);

    QAction *copyAction = editMenu->addElaIconAction(ElaIconType::Copy, "复制(&C)", QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::on_actionCopy_triggered);

    QAction *pasteAction = editMenu->addElaIconAction(ElaIconType::Paste, "粘贴(&P)", QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::on_actionPaste_triggered);

    editMenu->addSeparator();

    QAction *selectAllAction = editMenu->addElaIconAction(ElaIconType::CheckDouble, "全选(&A)", QKeySequence::SelectAll);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::on_actionSelectAll_triggered);

    ElaMenu *formatMenu = new ElaMenu("格式(&O)", this);
    menuBar_->addMenu(formatMenu);
    formatMenu->addAction("字体(&F)...", this, &MainWindow::on_actionFont_triggered);

    QAction *wordWrapAction = formatMenu->addAction("自动换行(&W)");
    wordWrapAction->setCheckable(true);
    wordWrapAction->setChecked(true);
    connect(wordWrapAction, &QAction::triggered, this, &MainWindow::on_actionWordWrap_triggered);

    ElaMenu *helpMenu = new ElaMenu("帮助(&H)", this);
    menuBar_->addMenu(helpMenu);
    helpMenu->addAction("关于(&A)...", this, &MainWindow::on_actionAbout_triggered);
}

void MainWindow::setupToolBar()
{
    toolBar_ = new ElaToolBar("主工具栏", this);
    toolBar_->setAllowedAreas(Qt::TopToolBarArea);
    toolBar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar_->setIconSize(QSize(24, 24));
    addToolBar(toolBar_);
    auto addToolButton = [this](ElaIconType::IconName icon, const QString &tip, auto slot) {
        auto *btn = new ElaToolButton(this);
        btn->setElaIcon(icon);
        btn->setToolTip(tip);
        connect(btn, &ElaToolButton::clicked, this, slot);
        toolBar_->addWidget(btn);
        return btn;
    };
    addToolButton(ElaIconType::FileLines, "新建", &MainWindow::on_actionNew_triggered);
    addToolButton(ElaIconType::FolderOpen, "打开", &MainWindow::on_actionOpen_triggered);
    addToolButton(ElaIconType::FloppyDisk, "保存", &MainWindow::on_actionSave_triggered);
    toolBar_->addSeparator();
    addToolButton(ElaIconType::ArrowRotateLeft, "撤销", &MainWindow::on_actionUndo_triggered);
    addToolButton(ElaIconType::ArrowRotateRight, "重做", &MainWindow::on_actionRedo_triggered);
    toolBar_->addSeparator();
    searchBox_ = new ElaLineEdit(this);
    searchBox_->setFixedWidth(200);
    searchBox_->setPlaceholderText("搜索笔记...");
    toolBar_->addWidget(searchBox_);
    QCheckBox *searchContentCheck = new QCheckBox("搜索内容", this);
    searchContentCheck->setChecked(m_searchInContent);
    connect(searchContentCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_searchInContent = checked;
        filterNoteList(searchBox_->text());
    });
    toolBar_->addWidget(searchContentCheck);
    sortComboBox_ = new QComboBox(this);
    sortComboBox_->addItem("按修改时间", "last_modified DESC");
    sortComboBox_->addItem("按创建时间", "created DESC");
    sortComboBox_->addItem("按标题", "title ASC");
    sortComboBox_->setFixedWidth(120);
    connect(sortComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSortChanged);
    toolBar_->addWidget(sortComboBox_);
    ElaMenu *viewMenu = new ElaMenu("视图(&V)", this);
    menuBar_->addMenu(viewMenu);
    showNotesAction = viewMenu->addAction("显示笔记列表");
    showNotesAction->setCheckable(true);
    showNotesAction->setChecked(true);
    connect(showNotesAction, &QAction::triggered, this, [this](bool checked) {
        if (notesDock_) notesDock_->setVisible(checked);
    });
}

void MainWindow::setupStatusBar()
{
    statusBar_ = new ElaStatusBar(this);
    statusLabel_ = new QLabel(this);
    statusLabel_->setStyleSheet("QLabel { padding: 0 8px; }");
    statusBar_->addWidget(statusLabel_);
    QLabel *langLabel = new QLabel("C++", this);
    langLabel->setStyleSheet("QLabel { color: #888; padding: 0 12px; }");
    statusBar_->addPermanentWidget(langLabel);
    setStatusBar(statusBar_);
}
