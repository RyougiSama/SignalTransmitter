#include "timeviewmodulated.h"
#include <QValueAxis>
#include <QWheelEvent>
#include <QtMath>

TimeViewModulated::TimeViewModulated(QWidget *parent)
    : QChartView(parent)
    , disp_series_(new QLineSeries(this))
{
    // 字体设置
    QFont font;
    font.setBold(true);

    // 创建图表并添加数据序列
    auto chart = new QChart();
    chart->setTitleFont(font);
    chart->setTitle("Modulated Time View");
    chart->legend()->close();
    chart->addSeries(disp_series_);

    // 创建坐标轴
    auto axisX = new QValueAxis();
    axisX->setTitleText("Time (s)");
    double tmax{ 1.0 };  // 默认显示1秒数据
    axisX->setRange(0, tmax);
    axisX->setTickCount(5);

    auto axisY = new QValueAxis();
    axisY->setTitleText("Amplitude");
    axisY->setRange(-1.2, 1.2);  // 调制后信号通常在-1到1之间
    axisY->setTickCount(9);

    // 添加坐标轴到图表
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    disp_series_->attachAxis(axisX);
    disp_series_->attachAxis(axisY);

    // 设置图表到视图
    setChart(chart);
    setRenderHint(QPainter::Antialiasing);

    // 启用鼠标追踪，以便能够实时更新鼠标位置
    setMouseTracking(true);
}

TimeViewModulated::~TimeViewModulated()
{}

void TimeViewModulated::UpdateView()
{
    // 确保有数据可以显示
    if (!txt_model_) return;

    const auto &modulated_data = txt_model_->get_txt_modulated_data();
    const auto sample_rate = txt_model_->kSampleRate;

    // 清空显示序列
    disp_series_->clear();

    // 计算实际可显示的数据数量
    const auto total_samples = modulated_data.size();
    if (total_samples == 0) return;

    // 初始化视图显示范围
    if (display_samples_ > total_samples) {
        display_samples_ = total_samples;
    }

    // 确保起始索引合法
    start_sample_index_ = qBound(0, start_sample_index_, total_samples - 1);

    // 确保显示数量不超过数据范围
    if (start_sample_index_ + display_samples_ > total_samples) {
        display_samples_ = total_samples - start_sample_index_;
    }

    // 提前分配空间 - 每个采样点对应图表上的一个点
    QList<QPointF> points;
    points.reserve(display_samples_);

    // 生成点序列
    for (int i = 0; i < display_samples_; ++i) {
        int sample_index = start_sample_index_ + i;
        if (sample_index < total_samples) {
            // 计算该采样点在时间轴上的位置
            double time = static_cast<double>(sample_index) / sample_rate;
            // 获取该采样点的振幅值
            double amplitude = modulated_data[sample_index];
            // 添加点到序列
            points.append(QPointF(time, amplitude));
        }
    }

    // 替换数据点
    disp_series_->replace(points);

    // 更新X轴范围
    QChart* chart = this->chart();
    auto axes = chart->axes(Qt::Horizontal);
    if (!axes.isEmpty()) {
        QValueAxis* axisX = qobject_cast<QValueAxis*>(axes.first());
        if (axisX) {
            double t_start = static_cast<double>(start_sample_index_) / sample_rate;
            double t_end = static_cast<double>(start_sample_index_ + display_samples_) / sample_rate;
            axisX->setRange(t_start, t_end);
        }
    }

    // 根据调制类型和当前数据调整Y轴范围
    CalculateYAxisRange();
}

void TimeViewModulated::wheelEvent(QWheelEvent *event)
{
    // 如果没有数据，则不处理滚轮事件
    if (!txt_model_ || txt_model_->get_txt_modulated_data().isEmpty())
        return;

    const auto total_samples = txt_model_->get_txt_modulated_data().size();

    // 获取滚轮的垂直滚动量
    QPoint delta = event->angleDelta();

    // 根据按键修饰符确定是滚动还是缩放
    if (event->modifiers() & Qt::ControlModifier) {
        // 按住Ctrl键滚动为缩放
        int zoom_amount = delta.y() / 120; // 每滚动120单位为一个级别

        if (zoom_amount != 0) {
            // 保存当前视图的中心位置
            int center_idx = start_sample_index_ + display_samples_ / 2;

            // 调整显示数量（缩放）- 对于调制波形，每步缩放更多点
            display_samples_ = qBound(50, display_samples_ - zoom_amount * 50, kMaxDisplaySamples);

            // 基于中心位置调整起始索引
            start_sample_index_ = center_idx - display_samples_ / 2;

            // 边界检查
            if (start_sample_index_ < 0) 
                start_sample_index_ = 0;

            if (start_sample_index_ + display_samples_ > total_samples)
                start_sample_index_ = qMax(0, total_samples - display_samples_);

            // 更新视图
            UpdateView();
        }
    } else {
        // 普通滚动 - 在波形上导航
        // 调制后的采样点更多，所以滚动速度要更快
        int scroll_amount = -delta.y() / 5; // 控制滚动速度
        
        if (scroll_amount != 0) {
            // 更新起始索引
            start_sample_index_ += scroll_amount;
            // 边界检查
            if (start_sample_index_ < 0)
                start_sample_index_ = 0;
            if (start_sample_index_ + display_samples_ > total_samples)
                start_sample_index_ = qMax(0, total_samples - display_samples_);
            // 更新视图
            UpdateView();
        }
    }
    // 阻止事件继续传递
    event->accept();
}

void TimeViewModulated::CalculateYAxisRange()
{
    // 根据调制类型设置适合的Y轴范围
    QChart* chart = this->chart();
    auto axes = chart->axes(Qt::Vertical);
    if (!axes.isEmpty()) {
        QValueAxis* axisY = qobject_cast<QValueAxis*>(axes.first());
        if (axisY) {
            if (modulation_type_.compare("ASK", Qt::CaseInsensitive) == 0) {
                // ASK调制通常是0到1的振幅范围
                axisY->setRange(-0.2, 1.2);
            } else if (modulation_type_.compare("PSK", Qt::CaseInsensitive) == 0) {
                // PSK调制通常是-1到1的振幅范围
                axisY->setRange(-1.2, 1.2);
            } else {
                // 默认范围，适合大多数调制方式
                axisY->setRange(-1.2, 1.2);
            }
        }
    }
}
