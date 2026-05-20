#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>

class DataModel;
class QComboBox;
class QPushButton;

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);

    void setModel(DataModel *model);
    void refreshColumns();   // 数据加载后刷新列选择器
    void exportChart(const QString &filePath); // 导出为 PNG

public slots:
    void updateChart();

private:
    void buildLineChart();
    void buildBarChart();
    void buildScatterChart();
    void buildPieChart();
    void applyChartTheme();

    DataModel     *m_model = nullptr;
    QChartView    *m_chartView = nullptr;
    QChart        *m_chart = nullptr;

           // 控件
    QComboBox *m_typeCombo  = nullptr;
    QComboBox *m_xCombo     = nullptr;
    QComboBox *m_yCombo     = nullptr;
    QPushButton *m_exportBtn = nullptr;
};

#endif // CHARTWIDGET_H