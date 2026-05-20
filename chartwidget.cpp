#include "chartwidget.h"
#include "datamodel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>

#include <QtCharts/QPieSlice>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(8);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({"折线图", "柱状图", "散点图", "饼图"});
    m_typeCombo->setFixedWidth(90);

    QLabel *xLabel = new QLabel("X轴:", this);
    m_xCombo = new QComboBox(this);
    m_xCombo->setFixedWidth(130);

    QLabel *yLabel = new QLabel("Y轴:", this);
    m_yCombo = new QComboBox(this);
    m_yCombo->setFixedWidth(130);

    m_exportBtn = new QPushButton("导出图表", this);
    m_exportBtn->setFixedWidth(90);

    ctrlLayout->addWidget(new QLabel("图表类型:", this));
    ctrlLayout->addWidget(m_typeCombo);
    ctrlLayout->addSpacing(12);
    ctrlLayout->addWidget(xLabel);
    ctrlLayout->addWidget(m_xCombo);
    ctrlLayout->addWidget(yLabel);
    ctrlLayout->addWidget(m_yCombo);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(m_exportBtn);

    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(ctrlLayout);
    mainLayout->addWidget(m_chartView);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::updateChart);
    connect(m_xCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::updateChart);
    connect(m_yCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartWidget::updateChart);
    connect(m_exportBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "导出图表", "chart.png",
                                                    "PNG 图片 (*.png);;JPEG 图片 (*.jpg)");
        if (!path.isEmpty()) exportChart(path);
    });
}

void ChartWidget::setModel(DataModel *model)
{
    m_model = model;
    refreshColumns();
}

void ChartWidget::refreshColumns()
{
    if (!m_model) return;
    QStringList cols = m_model->columnNames();

    m_xCombo->blockSignals(true);
    m_yCombo->blockSignals(true);

    m_xCombo->clear();
    m_yCombo->clear();

    m_xCombo->addItems(cols);
    m_yCombo->addItems(cols);

    if (cols.size() > 1) m_yCombo->setCurrentIndex(1);
    if (cols.size() > 0) m_xCombo->setCurrentIndex(0);

    m_xCombo->blockSignals(false);
    m_yCombo->blockSignals(false);

    updateChart();
}

void ChartWidget::updateChart()
{
    if (!m_model || m_model->isEmpty()) return;

    int type = m_typeCombo->currentIndex();
    switch (type) {
    case 0: buildLineChart();    break;
    case 1: buildBarChart();     break;
    case 2: buildScatterChart(); break;
    case 3: buildPieChart();     break;
    }
    applyChartTheme();
}

// ====================== 折线图 ======================
void ChartWidget::buildLineChart()
{
    m_chart->removeAllSeries();
    for (auto *ax : m_chart->axes()) m_chart->removeAxis(ax);

    QString xCol = m_xCombo->currentText();
    QString yCol = m_yCombo->currentText();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    QVector<double> xVals = m_model->getNumericColumn(xCol);
    QVector<double> yVals = m_model->getNumericColumn(yCol);
    int n = qMin(xVals.size(), yVals.size());
    if (n == 0) return;

    auto *series = new QLineSeries();
    series->setName(yCol);
    for (int i = 0; i < n; ++i)
        series->append(xVals[i], yVals[i]);

    m_chart->addSeries(series);
    m_chart->createDefaultAxes();
    m_chart->setTitle(QString("%1 vs %2").arg(xCol, yCol));
}

// ====================== 柱状图 ======================
void ChartWidget::buildBarChart()
{
    m_chart->removeAllSeries();
    for (auto *ax : m_chart->axes()) m_chart->removeAxis(ax);

    QString xCol = m_xCombo->currentText();
    QString yCol = m_yCombo->currentText();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    QStringList categories = m_model->getStringColumn(xCol);
    QVector<double>  yVals = m_model->getNumericColumn(yCol);
    int n = qMin(categories.size(), yVals.size());
    n = qMin(n, 50);
    if (n == 0) return;

    auto *barSet = new QBarSet(yCol);
    QStringList shownCats;
    for (int i = 0; i < n; ++i) {
        *barSet << yVals[i];
        shownCats << categories[i];
    }

    auto *series = new QBarSeries();
    series->append(barSet);
    m_chart->addSeries(series);

    auto *axisX = new QBarCategoryAxis();
    axisX->append(shownCats);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis();
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    m_chart->setTitle(yCol + " by " + xCol);
    m_chart->legend()->setVisible(false);
}

// ====================== 散点图 ======================
void ChartWidget::buildScatterChart()
{
    m_chart->removeAllSeries();
    for (auto *ax : m_chart->axes()) m_chart->removeAxis(ax);

    QString xCol = m_xCombo->currentText();
    QString yCol = m_yCombo->currentText();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    QVector<double> xVals = m_model->getNumericColumn(xCol);
    QVector<double> yVals = m_model->getNumericColumn(yCol);
    int n = qMin(xVals.size(), yVals.size());
    if (n == 0) return;

    auto *series = new QScatterSeries();
    series->setName(QString("%1 vs %2").arg(xCol, yCol));
    series->setMarkerSize(6.0);
    for (int i = 0; i < n; ++i)
        series->append(xVals[i], yVals[i]);

    m_chart->addSeries(series);
    m_chart->createDefaultAxes();
    m_chart->setTitle(QString("散点图：%1 vs %2  (r = %3)")
                          .arg(xCol, yCol)
                          .arg(m_model->correlation(xCol, yCol), 0, 'f', 3));
}

// ====================== 饼图 ======================
void ChartWidget::buildPieChart()
{
    m_chart->removeAllSeries();
    for (auto *ax : m_chart->axes()) m_chart->removeAxis(ax);

    QString xCol = m_xCombo->currentText();
    QString yCol = m_yCombo->currentText();
    if (xCol.isEmpty() || yCol.isEmpty()) return;

    QStringList labels = m_model->getStringColumn(xCol);
    QVector<double> vals = m_model->getNumericColumn(yCol);
    int n = qMin(labels.size(), vals.size());
    n = qMin(n, 20);
    if (n == 0) return;

    double total = 0;
    for (int i = 0; i < n; ++i)
        if (vals[i] > 0) total += vals[i];

    auto *pieSeries = new QPieSeries();
    pieSeries->setHoleSize(0);

    for (int i = 0; i < n; ++i) {
        if (vals[i] <= 0) continue;
        QPieSlice *slice = pieSeries->append(labels[i], vals[i]);
        slice->setLabelVisible(false);
        double pct = (total > 0) ? (vals[i] / total * 100.0) : 0.0;
        QString tip = QString("%1\n%2（%3%）")
                          .arg(labels[i])
                          .arg(vals[i])
                          .arg(pct, 0, 'f', 1);
        connect(slice, &QPieSlice::hovered, this,
                [slice, tip](bool hovered) {
                    slice->setExploded(hovered);
                    slice->setLabelVisible(hovered);
                    if (hovered) {
                        slice->setLabel(tip);
                        slice->setLabelFont(QFont("Microsoft YaHei", 9));
                    }
                });
    }

    if (!pieSeries->slices().isEmpty()) {
        QPieSlice *largest = pieSeries->slices().first();
        for (auto *sl : pieSeries->slices())
            if (sl->value() > largest->value()) largest = sl;
        largest->setExploded(true);
        largest->setExplodeDistanceFactor(0.05);
    }

    m_chart->addSeries(pieSeries);
    m_chart->setTitle(yCol + " 分布");
    m_chart->legend()->setAlignment(Qt::AlignRight);
}

// ====================== 主题 ======================
void ChartWidget::applyChartTheme()
{
    QPalette pal = palette();
    bool isDark = pal.color(QPalette::Window).lightness() < 128;
    m_chart->setTheme(isDark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
}

// ====================== 导出 ======================
void ChartWidget::exportChart(const QString &filePath)
{
    QPixmap pix = m_chartView->grab();
    if (!pix.save(filePath)) {
        QMessageBox::warning(this, "导出失败", "无法保存图表到：" + filePath);
    }
}