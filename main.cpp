#include "colorpickerwidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ColorPickerWidget w;
    w.show();
    return a.exec();
}
