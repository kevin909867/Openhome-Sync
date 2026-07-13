#include "mainwindow.h"

#include "widgets/logocanvas.h"
#include "widgets/togglebutton.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

void MainWindow::saveClick()
{
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    QJsonDocument doc(buildSaveObject());
    file.write(doc.toJson(QJsonDocument::Indented));
    QMessageBox::information(this, "Saved Successfully!", "Credentials and Lamps Saved Successfully!");
}

void MainWindow::loadClick()
{
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    applyLoadedData(doc.object());
}

QJsonObject MainWindow::buildSaveObject() const
{
    QJsonObject obj;
    QJsonArray credentials;
    credentials.append(inputUrl->text());
    credentials.append(inputToken->text());
    credentials.append(autostartBox->isChecked());
    credentials.append(minimizedBox->isChecked());
    credentials.append(startLampBox->isChecked());
    obj.insert("credentials", credentials);
    obj.insert("selected_monitor", monitorCombo->currentData().toString());
    obj.insert("lamps", logoCanvas ? logoCanvas->saveLampPositions(collectAllInputs()) : QJsonObject());
    return obj;
}

void MainWindow::applyLoadedData(const QJsonObject &obj)
{
    const QJsonArray credentials = obj.value("credentials").toArray();
    if (credentials.size() >= 5)
    {
        inputUrl->setText(credentials.at(0).toString());
        inputToken->setText(credentials.at(1).toString());
        autostartBox->setChecked(credentials.at(2).toBool());
        minimizedBox->setChecked(credentials.at(3).toBool());
        startLampBox->setChecked(credentials.at(4).toBool());

        if (minimizedBox->isChecked())
            showMinimized();

        if (startLampBox->isChecked())
        {
            toggleButton->setChecked(true);
            toggleButton->click();
        }
    }

    const QString selectedId = obj.value("selected_monitor").toString();
    const int idx = findMonitorByIdentity(selectedId);
    if (idx >= 0)
        monitorCombo->setCurrentIndex(idx);

    const QJsonObject lamps = obj.value("lamps").toObject();
    for (auto &row : dynamicRows)
    {
        row.rowWidget->setParent(nullptr);
        row.rowWidget->deleteLater();
    }
    dynamicRows.clear();

    const auto keys = lamps.keys();
    for (const QString &key : keys)
    {
        addDynamicRowWithText(key);
    }
    if (keys.isEmpty())
        addDynamicRowWithText(QString());

    logoCanvas->setLampPositions(lamps);
    refreshLogos();
}

void MainWindow::loadOnStartup()
{
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    applyLoadedData(doc.object());
}

bool MainWindow::isInAutostart() const
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return settings.contains("Openhome Sync");
#else
    return QFile::exists(autostartDesktopPath());
#endif
}

QString MainWindow::autostartDesktopPath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(dir);
    return dir + "/open-lightsync.desktop";
}

void MainWindow::addSelfToAutostart()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    settings.setValue("Openhome Sync", '"' + exe + '"');
#else
    QFile file(autostartDesktopPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "[Desktop Entry]\\nType=Application\\nName=Openhome Sync\\nExec=\"" << QCoreApplication::applicationFilePath() << "\"\\nTerminal=false\\nX-GNOME-Autostart-enabled=true\\n";
#endif
}

void MainWindow::removeSelfFromAutostart()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    settings.remove("Openhome Sync");
#else
    QFile::remove(autostartDesktopPath());
#endif
}
