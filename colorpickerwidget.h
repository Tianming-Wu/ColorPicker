#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class ColorPickerWidget;
}
QT_END_NAMESPACE

class ColorPickerWidget : public QWidget
{
    Q_OBJECT

public:
    ColorPickerWidget(QWidget *parent = nullptr);
    ~ColorPickerWidget();

private slots:
    void onColorChanged(const QColor &color);

private:
    Ui::ColorPickerWidget *ui;
};
