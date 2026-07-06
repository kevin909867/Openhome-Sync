#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Open-LightSync");
    a.setApplicationDisplayName("Open-LightSync");
    a.setWindowIcon(QIcon("icon.ico"));
    MainWindow w;
    w.show();
    return QApplication::exec();
}
