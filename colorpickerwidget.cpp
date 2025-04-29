#include "colorpickerwidget.h"
#include "./ui_colorpickerwidget.h"

ColorPickerWidget::ColorPickerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ColorPickerWidget)
{
    ui->setupUi(this);

    connect(ui->colorpicker, &ColorPicker::colorChanged, this, &ColorPickerWidget::onColorChanged);
    connect(ui->colorpicker, &ColorPicker::colorChanged, ui->preview, &ColorPreview::setColor);

    connect(ui->cbAlphaEnabled, &QCheckBox::toggled, ui->colorpicker, &ColorPicker::setAlphaEnabled);

    onColorChanged(Qt::red);
    ui->preview->setColor(Qt::red);
}

ColorPickerWidget::~ColorPickerWidget()
{
    delete ui;
}

void ColorPickerWidget::onColorChanged(const QColor &color)
{
    ui->rgbLineEdit->setText(ui->colorpicker->colorRgbString());
    ui->hsvLineEdit->setText(ui->colorpicker->colorHsvString());
    ui->qcolorLineEdit->setText(ui->colorpicker->colorQColorString());
}
