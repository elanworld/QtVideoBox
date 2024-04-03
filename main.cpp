#include "QtVideoBox.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtVideoBox w;
    w.show();
    return a.exec();
}
