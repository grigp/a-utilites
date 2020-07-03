#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("A-Med");
    a.setApplicationName("BG-Tester");
    a.setApplicationDisplayName("Тестер генератора");
    MainWindow w;
    w.show();

    return a.exec();
}
