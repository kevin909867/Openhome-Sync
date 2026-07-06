#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    a.setApplicationName("Open-LightSync");
    a.setApplicationDisplayName("Open-LightSync");
    a.setWindowIcon(QIcon("icon.ico"));
    MainWindow w;
    w.showNormal();
    w.raise();
    w.activateWindow();
    return QApplication::exec();
}
