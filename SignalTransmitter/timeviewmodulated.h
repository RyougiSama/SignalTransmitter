#pragma once

#include <QChartView>
#include <QLineSeries>
#include "txtmodel.h"

class TimeViewModulated : public QChartView
{
    Q_OBJECT

public:
    TimeViewModulated(QWidget *parent);
    ~TimeViewModulated();

    void set_txt_model(TxtModel *txt_model_ptr) { txt_model_ = txt_model_ptr; }
    void set_modulation_type(const QString &modulation_type) { modulation_type_ = modulation_type; }
    QString get_modulation_type() const { return modulation_type_; }

    void UpdateView();

protected:
    // 重写鼠标滚轮事件处理
    void wheelEvent(QWheelEvent *event) override;

private:
    TxtModel *txt_model_{ nullptr };
    QLineSeries *disp_series_;
    QString modulation_type_{"ASK"}; // 默认为ASK调制

    // 当前视图的起始采样点索引位置
    int start_sample_index_{ 0 };
    // 当前视图显示的采样点数
    int display_samples_{ 500 };
    // 最大可显示的采样点数
    static constexpr int kMaxDisplaySamples{ 1000 };
    // 计算Y轴显示范围
    void CalculateYAxisRange();
};
