#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QColor>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMainWindow>
#include <QPoint>
#include <QPushButton>
#include <QStringList>
#include <QUrl>
#include <QTimer>
#include <QVector>

class QButtonGroup;
class QComboBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QNetworkAccessManager;
class QScreen;
class QVBoxLayout;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MovableLamp;
class LogoCanvas;
class ToggleButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateLight();
    void saveClick();
    void loadClick();
    void helpClick();
    void refreshLogos();
    void addDynamicRow();
    void onMonitorChanged(int index);

private:
    struct DynamicRow {
        QWidget *rowWidget = nullptr;
        QLineEdit *lineEdit = nullptr;
    };

    void setupInterface();
    void setupStyles();
    void setupConnections();

    void addDynamicRowWithText(const QString &text);
    void deleteDynamicRow(QWidget *rowWidget, QLineEdit *lineEdit);
    QStringList collectAllInputs() const;

    void populateMonitorList();
    QScreen *selectedScreen() const;
    QString monitorIdentity(QScreen *screen) const;
    int findMonitorByIdentity(const QString &identity) const;

    QImage captureSelectedScreenImage() const;
    QColor sampleFromImage(const QImage &image, const QPoint &globalPos) const;
    QColor sampleAverageColor(const QImage &image, QScreen *screen) const;
    QColor sampleCrazyColor(const QImage &image, QScreen *screen) const;
    QPoint clampToScreen(const QPoint &globalPos, QScreen *screen) const;

    void runScreenMode(const QStringList &entities, const QImage &image, QScreen *screen);
    void runAverageMode(const QStringList &entities, const QImage &image, QScreen *screen);
    void runCrazyMode(const QStringList &entities, const QImage &image, QScreen *screen);

    void sendTurnOn(const QString &entityId, const QColor &color, double transitionSeconds);
    void turnOff();
    void postJson(const QUrl &url, const QJsonObject &payload);

    void loadOnStartup();
    void applyLoadedData(const QJsonObject &obj);
    QJsonObject buildSaveObject() const;

    bool isInAutostart() const;
    void addSelfToAutostart();
    void removeSelfFromAutostart();
    QString autostartDesktopPath() const;

    Ui::MainWindow *ui;

    QWidget *central = nullptr;
    QHBoxLayout *mainLayout = nullptr;
    QWidget *leftContainer = nullptr;
    QWidget *rightContainer = nullptr;
    QVBoxLayout *leftLayout = nullptr;
    QVBoxLayout *rightLayout = nullptr;
    QVBoxLayout *dynamicLayout = nullptr;

    QLineEdit *inputUrl = nullptr;
    QLineEdit *inputToken = nullptr;
    QComboBox *monitorCombo = nullptr;

    QPushButton *modeBtnScreen = nullptr;
    QPushButton *modeBtnAverage = nullptr;
    QPushButton *modeBtnCrazy = nullptr;
    QButtonGroup *modeGroup = nullptr;

    QCheckBox *autostartBox = nullptr;
    QCheckBox *minimizedBox = nullptr;
    QCheckBox *startLampBox = nullptr;

    QPushButton *saveButton = nullptr;
    QPushButton *loadButton = nullptr;
    QPushButton *helpButton = nullptr;

    LogoCanvas *logoCanvas = nullptr;
    ToggleButton *toggleButton = nullptr;

    QVector<DynamicRow> dynamicRows;
    QVector<QScreen *> knownScreens;

    QTimer timer;
    QNetworkAccessManager *networkManager = nullptr;

    QString savePath;
    bool lampStatus = false;
    bool showingCaptureFailure = false;
};

#endif // MAINWINDOW_H
