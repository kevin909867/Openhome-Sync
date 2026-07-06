#include "mainwindow.h"

#include <QApplication>
#include <fstream>
#include <string>

static void appendStartupLog(const char *message)
{
    std::ofstream file("C:/temp/openlightsync-startup.log", std::ios::app);
    if (!file.is_open()) {
        return;
    }

    file << message << "\n";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    appendStartupLog("main: after QApplication");
    MainWindow w;
    appendStartupLog("main: after MainWindow ctor");
    w.show();
    appendStartupLog("main: after show");
    const int exitCode = QApplication::exec();
    appendStartupLog("main: exec returned");
    return exitCode;
}
