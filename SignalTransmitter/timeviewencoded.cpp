#include "timeviewencoded.h"
#include <QValueAxis>
#include <QWheelEvent>

TimeViewEncoded::TimeViewEncoded(QWidget *parent)
    : QChartView(parent)
    , disp_series_(new QLineSeries(this))
{
    // 字体设置
    QFont font;
    font.setBold(true);
    // 创建图表并添加数据序列
    auto chart = new QChart();
    chart->setTitleFont(font);
    chart->setTitle("Encoded Time View");
    chart->legend()->close();
    chart->addSeries(disp_series_);
    // 创建坐标轴
    auto axisX = new QValueAxis();
    axisX->setTitleText("Time (s)");
    double tmax{ 5.0 };
    axisX->setRange(0, tmax);
    axisX->setTickCount(5);

    auto axisY = new QValueAxis();
    axisY->setTitleText("Amplitude");
    axisY->setRange(-0.2, 1.2);
    axisY->setTickCount(8);
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

TimeViewEncoded::~TimeViewEncoded()
{}

void TimeViewEncoded::UpdateView()
{
    // 确保有数据可以显示
    if (!txt_model_) return;

    const auto &encoded = txt_model_->get_txt_encoded_data();
    const auto sample_rate = txt_model_->kSampleRate;
    const auto samples_per_bit = txt_model_->kSamplesPerBit;

    // 计算比特宽度，即一个比特在时间轴上的宽度（秒）
    const double bit_width = static_cast<double>(samples_per_bit) / sample_rate;

    // 清空显示序列
    disp_series_->clear();

    // 计算实际可显示的数据数量
    const auto total_bits = encoded.size();
    if (total_bits == 0) return;

    // 初始化视图显示范围
    if (display_count_ > total_bits) {
        display_count_ = total_bits;
    }

    // 确保起始索引合法
    start_index_ = qBound(0, start_index_, total_bits - 1);

    // 确保显示数量不超过数据范围
    if (start_index_ + display_count_ > total_bits) {
        display_count_ = total_bits - start_index_;
    }

    // 提前分配空间 - 每个比特需要2个点（起点和终点）来形成矩形波形
    QList<QPointF> points;
    points.reserve(display_count_ * 2);

    // 生成点序列 - 为每个比特添加两个点，形成矩形信号
    for (auto i = 0; i < display_count_; ++i) {
        int bit_index = start_index_ + i;
        // 获取当前比特值
        uint8_t bit_value = encoded[bit_index];

        // 计算该比特在时间轴上的起始位置
        double time_start = bit_index * bit_width;
        // 计算该比特在时间轴上的结束位置（下一个比特的起始位置）
        double time_end = (bit_index + 1) * bit_width;

        // 添加该比特的起点 - 在时间轴上的起始位置显示该比特值
        points.append(QPointF(time_start, bit_value));
        // 添加该比特的终点 - 在时间轴上的结束位置显示该比特值
        // 这样两点之间会形成水平线段，表示该比特在这段时间内的值保持不变
        points.append(QPointF(time_end - 0.0001, bit_value)); // 略微提前结束点，避免和下一个比特的起点重合
    }

    // 替换数据点
    disp_series_->replace(points);

    // 更新X轴范围
    QChart* chart = this->chart();
    auto axes = chart->axes(Qt::Horizontal);
    if (!axes.isEmpty()) {
        QValueAxis* axisX = qobject_cast<QValueAxis*>(axes.first());
        if (axisX) {
            double t_start = start_index_ * bit_width;
            double t_end = (start_index_ + display_count_) * bit_width;
            axisX->setRange(t_start, t_end);
        }
    }
}

void TimeViewEncoded::wheelEvent(QWheelEvent *event)
{
    // 如果没有数据，则不处理滚轮事件
    if (!txt_model_ || txt_model_->get_txt_encoded_data().isEmpty())
        return;

    const auto total_bits = txt_model_->get_txt_encoded_data().size();

    // 获取滚轮的垂直滚动量
    QPoint delta = event->angleDelta();

    // 根据按键修饰符确定是滚动还是缩放
    if (event->modifiers() & Qt::ControlModifier) {
        // 按住Ctrl键滚动为缩放
        int zoom_amount = delta.y() / 120; // 每滚动120单位为一个级别

        if (zoom_amount != 0) {
            // 保存当前视图的中心位置
            int center_idx = start_index_ + display_count_ / 2;
            // 调整显示数量（缩放）
            display_count_ = qBound(10, display_count_ - zoom_amount * 10, kMaxDisplayCount);
            // 基于中心位置调整起始索引
            start_index_ = center_idx - display_count_ / 2;
            // 边界检查
            if (start_index_ < 0) 
                start_index_ = 0;
            if (start_index_ + display_count_ > total_bits)
                start_index_ = qMax(0, total_bits - display_count_);
            // 更新视图
            UpdateView();
        }
    } else {
        // 普通滚动 - 在波形上导航
        int scroll_amount = -delta.y() / 20; // 控制滚动速度
        if (scroll_amount != 0) {
            // 更新起始索引
            start_index_ += scroll_amount;
            // 边界检查
            if (start_index_ < 0)
                start_index_ = 0;
            if (start_index_ + display_count_ > total_bits)
                start_index_ = qMax(0, total_bits - display_count_);
            // 更新视图
            UpdateView();
        }
    }
    // 阻止事件继续传递
    event->accept();
}
