#pragma once

#include <QChartView>
#include <QLineSeries>
#include "txtmodel.h"

class TimeViewEncoded : public QChartView
{
    Q_OBJECT

public:
    TimeViewEncoded(QWidget *parent);
    ~TimeViewEncoded();

    void set_txt_model(TxtModel *txt_model_ptr) { txt_model_ = txt_model_ptr; }

    void UpdateView();

protected:
    // 重写鼠标滚轮事件处理
    void wheelEvent(QWheelEvent *event) override;

private:
    TxtModel *txt_model_{ nullptr };
    QLineSeries *disp_series_;
    
    // 当前视图的起始索引位置
    int start_index_{ 0 };
    // 当前视图显示的比特数
    int display_count_{ 100 };
    // 最大可显示的比特数
    static constexpr int kMaxDisplayCount{ 100 };
};
