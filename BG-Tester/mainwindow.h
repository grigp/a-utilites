#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QMap>
#include <QSet>

#include "datadefines.h"
#include "serialportdefines.h"

class Driver;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_powerChange(int value);
    void on_powerEdit(int value);
    void on_setModeCheckBox(bool checked);

    void on_combineChange(bool checked);

    void on_sliderPressed();  ///! Начало редактирования слайдера
    void on_sliderReleased(); ///! Окончание редактирования слайдера

    void on_btnSettingsClicked();
    void on_btnStartConnectClicked();

    void getPower(DataDefines::PowerData data);
    void getMode(DataDefines::ModeData data);
    void getCodeUnique(DataDefines::CodeUniqueData data);
    void getBatteryLevel(DataDefines::BatteryLevelData data);
    void communicationError(const QString &drvName, const QString &port, const int errorCode);

    void on_btnStartClicked();
    void on_btnPauseClicked();
    void on_btnResetClicked();

    void on_btnSaveLogClicked();
    void on_btnSaveConfig();
    void on_btnRestoreConfig();

    void on_btnSelectAll();
    void on_btnUnSelectAll();

private:
    Ui::MainWindow *ui;

    void initUi();

    void initDefaultMasks();

    void setPower(const QString &objName, const int power);

    void setMode();

    DataDefines::Frequency currentFrequency() const;
    void setCurrentFrequency(const DataDefines::Frequency freq);

    DataDefines::Modulation currentModulation() const;
    void setCurrentModulation(const DataDefines::Modulation modulation);

    void assignMaxPowerValue();

    QList<QSlider*> getCheckedSliders();

    void saveSelected() const;
    void restoreSelected();

    void saveSizeAndPosition();
    void restoreSizeAndPosition();

    struct PowerValueCtrl
    {
        QSlider* slider;
        QSpinBox* editor;
        QLabel* label;
        PowerValueCtrl() {}
        PowerValueCtrl(QSlider* sld, QSpinBox* ed, QLabel* lbl)
            : slider(sld), editor(ed), label(lbl)
        {}
    };

    QMap<QSlider*, QSpinBox*> m_indicatorPowerValue;
    QMap<QSlider*, QCheckBox*> m_indicatorPowerCheck;
    QMap<int, PowerValueCtrl> m_indicatorPowerByNumber;
    QMap<DataDefines::Frequency, QCheckBox*> m_freqControls;
    QMap<DataDefines::Modulation, QCheckBox*> m_modulationControls;
    QMap<int, int> m_initializedPower;
    QMap<int, int> m_changed;   ///! Перечень параметров, которые были изменены за последний цикл
    bool m_initializedMode {false};
//    QSet<int> m_restoredPower;
    bool m_isRestoredConfig {false};
//    bool m_isRestoredMode {false};

    bool m_isProcess {false};
    bool m_isPause {false};
    bool m_reserve {false};
    int m_timerProcess {-1};

    Driver* m_driver {nullptr};
    SerialPortDefines::Ports m_port {SerialPortDefines::pcCom1};
    int m_maxPowerValue {60};
    int m_startPowerValue {-1}; ///! Начальное значение мощности при редактировании
    QSlider* m_curSlider {nullptr};

    QStringList m_log;
};

#endif // MAINWINDOW_H
