#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "driver.h"
#include "editsettingsdialog.h"

#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
  , m_driver(new Driver(this))
{
    ui->setupUi(this);
    initUi();
    setWindowIcon(QIcon(":/images/Icon128.ico"));

    initDefaultMasks();

    ui->lblCommunicationError->setVisible(false);
    ui->btnStart->setEnabled(false);

    QSettings set(QApplication::instance()->organizationName(),
                  QApplication::instance()->applicationName());
    set.beginGroup("device");
    m_port = static_cast<SerialPortDefines::Ports>(set.value("port", SerialPortDefines::pcCom1).toInt());
    m_driver->setPortName(m_port);
    m_driver->setDelaySendByte(set.value("DelaySendByte", Driver::DelaySendByteDefault).toInt());
    set.endGroup();
    set.beginGroup("settings");
    m_maxPowerValue = set.value("MaxPowerValue", 60).toInt();
    set.endGroup();
    assignMaxPowerValue();
    restoreSelected();

    connect(m_driver, &Driver::sendPower, this, &MainWindow::getPower);
    connect(m_driver, &Driver::sendMode, this, &MainWindow::getMode);
    connect(m_driver, &Driver::sendCodeUnique, this, &MainWindow::getCodeUnique);
    connect(m_driver, &Driver::sendBatteryLevel, this, &MainWindow::getBatteryLevel);
    connect(m_driver, &Driver::communicationError, this, &MainWindow::communicationError);

    restoreSizeAndPosition();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_driver)
    {
        m_driver->stop();
        m_driver->deleteLater();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (m_isProcess && m_timerProcess != -1)
    {
        foreach (auto chan, m_changed.keys())
        {
            m_log.append(QString("---> power. chan: %1, value: %2").arg(chan).arg(m_changed.value(chan)));
            m_driver->setPower(static_cast<quint8>(chan), static_cast<quint8>(m_changed.value(chan)));
        }

        m_changed.clear();
    }
    QMainWindow::timerEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    saveSizeAndPosition();
    QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    saveSizeAndPosition();
    QMainWindow::resizeEvent(event);
}

void MainWindow::on_powerChange(int value)
{
    if (sender())
    {
        auto* slider = static_cast<QSlider*>(sender());
        auto* edit = m_indicatorPowerValue.value(slider);
        auto* cb = m_indicatorPowerCheck.value(slider);
        if (edit && cb && (((m_curSlider) && (slider == m_curSlider)) || (!m_curSlider)))
        {
            int v = value;
            int d = v - m_startPowerValue;

            edit->setValue(v);
            if (!m_isPause)
            {
                setPower(slider->objectName(), v);
                m_startPowerValue = v;

                if (cb->isChecked() && m_curSlider)
                {
                    QList<QSlider*> sliders = getCheckedSliders();
                    foreach (auto sldr, sliders)
                        if (sldr != slider)
                        {
                            sldr->setValue(sldr->value() + d);
                            auto* edit = m_indicatorPowerValue.value(sldr);
                            if (edit)
                                edit->setValue(sldr->value() + d);
                            setPower(sldr->objectName(), sldr->value());
                        }
                }
            }
        }
    }
}

void MainWindow::on_powerEdit(int value)
{
    if (sender())
    {
        auto* editor = static_cast<QSpinBox*>(sender());
        auto slider = m_indicatorPowerValue.key(editor);
        if (slider && (slider == m_curSlider || !m_curSlider))
        {
            //! Ограничиваем значение 60, чтоб не сильно било током
            int v = value;

            slider->setValue(v);
            if (!m_isPause)
                setPower(slider->objectName(), v);
        }
    }
}

void MainWindow::on_setModeCheckBox(bool checked)
{
    Q_UNUSED(checked);
    setMode();
}

void MainWindow::on_combineChange(bool checked)
{
    Q_UNUSED(checked);
    saveSelected();
}

void MainWindow::on_sliderPressed()
{
    auto* slider = static_cast<QSlider*>(sender());
    m_startPowerValue = slider->value();
    m_curSlider = slider;
}

void MainWindow::on_sliderReleased()
{
    m_startPowerValue = -1;
    m_curSlider = nullptr;
}

void MainWindow::on_btnSettingsClicked()
{
    EditSettingsDialog dlg(this);
    dlg.setPort(m_port);
    dlg.setMaxPowerValue(m_maxPowerValue);
    dlg.setDelaySendByte(m_driver->delaySendByte());
    if (dlg.exec() == QDialog::Accepted)
    {
        m_port = dlg.port();
        m_maxPowerValue = dlg.maxPowerValue();
        assignMaxPowerValue();

        QSettings set(QApplication::instance()->organizationName(),
                      QApplication::instance()->applicationName());
        set.beginGroup("device");
        set.setValue("port", m_port);
        set.setValue("DelaySendByte", dlg.delaySendByte());
        set.endGroup();
        set.beginGroup("settings");
        set.setValue("MaxPowerValue", m_maxPowerValue);
        set.endGroup();
        m_driver->setPortName(m_port);
        m_driver->setDelaySendByte(dlg.delaySendByte());
    }
}

void MainWindow::on_btnStartConnectClicked()
{
    m_isProcess = !m_isProcess;
    if (m_isProcess)
    {
        ui->btnStartConnect->setText(tr("Стоп"));
        m_log.clear();
        m_driver->start();
        m_timerProcess = startTimer(1000);
    }
    else
    {
        foreach (auto str, m_log)
            ui->teLog->append(str);
        ui->btnStartConnect->setVisible(false);
        m_driver->stop();
    }
    ui->btnRestoreConfig->setEnabled(m_isProcess);
}

void MainWindow::getPower(DataDefines::PowerData data)
{
    ui->lblCommunicationError->setVisible(false);
    auto rec = m_indicatorPowerByNumber.value(data.channel);
    if (rec.editor && rec.slider && rec.label)
    {
        if (!m_initializedPower.contains(data.channel))
        {
            rec.editor->setValue(data.power);
            rec.slider->setValue(data.power);
        }
        //! В метку выводим всегда
        rec.label->setText(QString::number(data.power));
        m_initializedPower.insert(data.channel, data.power);
//        else
//        {
//            if (!m_isRestoredConfig)
//                m_initializedPower.insert(data.channel, data.power);
//            else
//            {
//                if (m_restoredPower.contains(data.channel))
//                {
//                    auto power = m_initializedPower.value(data.channel);
//                    m_log.append("<--- send 253 " + QString::number(data.channel) + " " + QString::number(power));
//                    m_driver->setPower(data.channel, power);
//                    m_initializedPower.insert(data.channel, power);
//                    m_restoredPower.remove(data.channel);
//                }
//            }
//        }
    }

    if (data.channel == 12 )
    {
        QString sPows = "";
        foreach (auto key, m_initializedPower.keys())
            sPows = sPows + " " + QString::number(key) + ":" + QString::number(m_initializedPower.value(key));
        m_log.append("---> power " + sPows);
//        qDebug() << "---> power" << sPows;
    }
}

void MainWindow::getMode(DataDefines::ModeData data)
{
    m_log.append("---> mode freq:" +
                            QString::number(data.freq) + " " +
                 "modul:" + QString::number(data.modulation) + " " +
                 "pause:" + QString::number(data.pause) + " " +
                 "reserve:" + QString::number(data.reserve));
//    qDebug() << "---> mode" <<
//                "freq:" + QString::number(data.freq) <<
//                "modul:" + QString::number(data.modulation) <<
//                "pause:" + QString::number(data.pause) <<
//                "reserve:" + QString::number(data.reserve);

    ui->lblCommunicationError->setVisible(false);
    if (!m_initializedMode)
    {
        auto cbFreq = m_freqControls.value(data.freq);
        if (cbFreq)
            cbFreq->setChecked(true);

        if (data.modulation == DataDefines::modAm || data.modulation == DataDefines::modFm)
        {
            auto cbModulation = m_modulationControls.value(data.modulation);
            if (cbModulation)
                cbModulation->setChecked(true);
        }
        else
        if (data.modulation == DataDefines::modBoth)
        {
            m_modulationControls.value(DataDefines::modAm)->setChecked(true);
            m_modulationControls.value(DataDefines::modFm)->setChecked(true);
        }
        else
        if (data.modulation == DataDefines::modNone)
        {
            m_modulationControls.value(DataDefines::modAm)->setChecked(false);
            m_modulationControls.value(DataDefines::modFm)->setChecked(false);
        }

        m_isPause = data.pause;
        m_reserve = data.reserve;

        m_initializedMode = true;
    }
//    else
//    {
//        if (m_isRestoredMode)
//            setMode();
//        m_isRestoredMode = false;
//    }
}

void MainWindow::getCodeUnique(DataDefines::CodeUniqueData data)
{
    m_log.append("---> device " + QString::number(data.code));
//    qDebug() << "---> device" << QString::number(data.code);
    ui->lblCommunicationError->setVisible(false);
    ui->lblDevice->setText(QString("Устройство № %1").arg(data.code));
}

void MainWindow::getBatteryLevel(DataDefines::BatteryLevelData data)
{
//    int perc = round(1.82 * data.level - 324);
    int perc = round(data.level * 100 / 80.0);
    int persent = perc;
    if (persent > 100)
        persent = 100;
    if (persent < 0)
        persent = 0;
    m_log.append(QString("---> battery level %1 % (%2 - %3)").arg(persent).arg(data.level).arg(perc));
    ui->lblCommunicationError->setVisible(false);
    ui->lblBatteryLevel->setText(QString("%1 %").arg(persent));
}

void MainWindow::communicationError(const QString &drvName, const QString &port, const int errorCode)
{
    Q_UNUSED(errorCode);
    ui->lblCommunicationError->setText(QString(tr("Ошибка связи с устройством") + ": %1 (" + tr("порт") + ": %2)").
                                       arg(drvName).arg(port));
    ui->lblCommunicationError->setVisible(true);
}

void MainWindow::on_btnStartClicked()
{
    if (m_isProcess)
    {
        ui->btnStart->setEnabled(false);
        ui->btnPause->setEnabled(true);
        ui->btnReset->setEnabled(true);

        m_isPause = false;
        setMode();

        for (int i = 0; i < 11; ++i)
        {
            int val = m_indicatorPowerByNumber.value(i+1).slider->value();

            if (m_isProcess)
            {
                m_log.append("<--- send 253 " + QString::number(i+1) + " " + QString::number(val));
                m_driver->setPower(static_cast<quint8>(i+1), static_cast<quint8>(val));
            }
        }

//        m_initializedPower.clear();
//        m_initializedMode = true;
    }
    else
        QMessageBox::information(this, tr("Предупреждение"), tr("Процесс приема не запущен"));
}

void MainWindow::on_btnPauseClicked()
{
    if (m_isProcess)
    {
        ui->btnStart->setEnabled(true);
        ui->btnPause->setEnabled(false);
        ui->btnReset->setEnabled(false);

        m_isPause = true;
        setMode();
    }
    else
        QMessageBox::information(this, tr("Предупреждение"), tr("Процесс приема не запущен"));
}

void MainWindow::on_btnResetClicked()
{
    if (m_isProcess)
    {
//        ui->btnStart->setEnabled(true);
//        ui->btnPause->setEnabled(false);
//        ui->btnReset->setEnabled(false);

        m_isPause = true;
        setMode();
        m_isPause = false;
        m_initializedPower.clear();
        m_initializedMode = true;
    }
    else
        QMessageBox::information(this, tr("Предупреждение"), tr("Процесс приема не запущен"));
}

void MainWindow::on_btnSaveLogClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл логов"), "",
                                 tr("Файлы логов (*.log)"));
    if (fileName != "")
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << ui->teLog->toPlainText();
            file.close();
        }
    }
}

void MainWindow::on_btnSaveConfig()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл конфигурации"), "",
                                 tr("Файлы конфигураций (*.cfg)"));
    if (fileName != "")
    {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QJsonObject root;
            root["frequency"] = currentFrequency();
            root["modulation"] = currentModulation();

            QJsonArray arrPower;
            for (int i = 1; i <= 12; ++i)
            {
                QJsonObject pv;
                pv["value"] = m_indicatorPowerByNumber.value(i).slider->value();
                arrPower.append(pv);
            }
            root["power"] = arrPower;

            QJsonDocument doc(root);
            QByteArray ba = doc.toJson();
            file.write(ba);

            file.close();
        }
    }
}

void MainWindow::on_btnRestoreConfig()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть файл конфигурации"), "",
                                 tr("Файлы конфигураций (*.cfg)"));
    if (fileName != "")
    {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QByteArray ba = file.readAll();
            QJsonDocument loadDoc(QJsonDocument::fromJson(ba));
            QJsonObject root = loadDoc.object();
            file.close();

            DataDefines::Frequency freq = static_cast<DataDefines::Frequency>(root["frequency"].toInt());
            setCurrentFrequency(freq);
            DataDefines::Modulation modulation = static_cast<DataDefines::Modulation>(root["modulation"].toInt());
            setCurrentModulation(modulation);

            if (m_isProcess)
                setMode();
            else
            {
                m_isRestoredConfig = true;
//                m_isRestoredMode = true;
                m_initializedMode = true;
            }

            QJsonArray arrPower = root["power"].toArray();
            for (int i = 0; i < arrPower.size(); ++i)
            {
                QJsonObject pv = arrPower.at(i).toObject();
                auto val = pv["value"].toInt();
                m_indicatorPowerByNumber.value(i+1).slider->setValue(val);
                m_indicatorPowerByNumber.value(i+1).editor->setValue(val);

                if (m_isProcess)
                {
                    m_log.append("<--- send 253 " + QString::number(i+1) + " " + QString::number(val));
                    m_driver->setPower(static_cast<quint8>(i+1), static_cast<quint8>(val));
                }
                else
                {
                    m_initializedPower.insert(i+1, val);
//                    m_restoredPower.insert(i+1);
                }
            }
        }
    }
}

void MainWindow::on_btnSelectAll()
{
    ui->cbPwr1->setChecked(true);
    ui->cbPwr2->setChecked(true);
    ui->cbPwr3->setChecked(true);
    ui->cbPwr4->setChecked(true);
    ui->cbPwr5->setChecked(true);
    ui->cbPwr6->setChecked(true);
    ui->cbPwr7->setChecked(true);
    ui->cbPwr8->setChecked(true);
    ui->cbPwr9->setChecked(true);
    ui->cbPwr10->setChecked(true);
    ui->cbPwr11->setChecked(true);
    ui->cbPwr12->setChecked(true);
}

void MainWindow::on_btnUnSelectAll()
{
    ui->cbPwr1->setChecked(false);
    ui->cbPwr2->setChecked(false);
    ui->cbPwr3->setChecked(false);
    ui->cbPwr4->setChecked(false);
    ui->cbPwr5->setChecked(false);
    ui->cbPwr6->setChecked(false);
    ui->cbPwr7->setChecked(false);
    ui->cbPwr8->setChecked(false);
    ui->cbPwr9->setChecked(false);
    ui->cbPwr10->setChecked(false);
    ui->cbPwr11->setChecked(false);
    ui->cbPwr12->setChecked(false);
}

void MainWindow::initUi()
{
    QFile style(":/qss/main.qss");
    style.open( QFile::ReadOnly );
    QString stlDetail(style.readAll());
    setStyleSheet(stlDetail);
}

void MainWindow::initDefaultMasks()
{
    m_initializedPower.clear();
//    m_restoredPower.clear();
    m_indicatorPowerValue.insert(ui->sldPwr1, ui->edValuePwr1);
    m_indicatorPowerValue.insert(ui->sldPwr2, ui->edValuePwr2);
    m_indicatorPowerValue.insert(ui->sldPwr3, ui->edValuePwr3);
    m_indicatorPowerValue.insert(ui->sldPwr4, ui->edValuePwr4);
    m_indicatorPowerValue.insert(ui->sldPwr5, ui->edValuePwr5);
    m_indicatorPowerValue.insert(ui->sldPwr6, ui->edValuePwr6);
    m_indicatorPowerValue.insert(ui->sldPwr7, ui->edValuePwr7);
    m_indicatorPowerValue.insert(ui->sldPwr8, ui->edValuePwr8);
    m_indicatorPowerValue.insert(ui->sldPwr9, ui->edValuePwr9);
    m_indicatorPowerValue.insert(ui->sldPwr10, ui->edValuePwr10);
    m_indicatorPowerValue.insert(ui->sldPwr11, ui->edValuePwr11);
    m_indicatorPowerValue.insert(ui->sldPwr12, ui->edValuePwr12);

    m_indicatorPowerCheck.insert(ui->sldPwr1, ui->cbPwr1);
    m_indicatorPowerCheck.insert(ui->sldPwr2, ui->cbPwr2);
    m_indicatorPowerCheck.insert(ui->sldPwr3, ui->cbPwr3);
    m_indicatorPowerCheck.insert(ui->sldPwr4, ui->cbPwr4);
    m_indicatorPowerCheck.insert(ui->sldPwr5, ui->cbPwr5);
    m_indicatorPowerCheck.insert(ui->sldPwr6, ui->cbPwr6);
    m_indicatorPowerCheck.insert(ui->sldPwr7, ui->cbPwr7);
    m_indicatorPowerCheck.insert(ui->sldPwr8, ui->cbPwr8);
    m_indicatorPowerCheck.insert(ui->sldPwr9, ui->cbPwr9);
    m_indicatorPowerCheck.insert(ui->sldPwr10, ui->cbPwr10);
    m_indicatorPowerCheck.insert(ui->sldPwr11, ui->cbPwr11);
    m_indicatorPowerCheck.insert(ui->sldPwr12, ui->cbPwr12);

    m_indicatorPowerByNumber.insert(1, PowerValueCtrl(ui->sldPwr1, ui->edValuePwr1, ui->lblValuePwr1));
    m_indicatorPowerByNumber.insert(2, PowerValueCtrl(ui->sldPwr2, ui->edValuePwr2, ui->lblValuePwr2));
    m_indicatorPowerByNumber.insert(3, PowerValueCtrl(ui->sldPwr3, ui->edValuePwr3, ui->lblValuePwr3));
    m_indicatorPowerByNumber.insert(4, PowerValueCtrl(ui->sldPwr4, ui->edValuePwr4, ui->lblValuePwr4));
    m_indicatorPowerByNumber.insert(5, PowerValueCtrl(ui->sldPwr5, ui->edValuePwr5, ui->lblValuePwr5));
    m_indicatorPowerByNumber.insert(6, PowerValueCtrl(ui->sldPwr6, ui->edValuePwr6, ui->lblValuePwr6));
    m_indicatorPowerByNumber.insert(7, PowerValueCtrl(ui->sldPwr7, ui->edValuePwr7, ui->lblValuePwr7));
    m_indicatorPowerByNumber.insert(8, PowerValueCtrl(ui->sldPwr8, ui->edValuePwr8, ui->lblValuePwr8));
    m_indicatorPowerByNumber.insert(9, PowerValueCtrl(ui->sldPwr9, ui->edValuePwr9, ui->lblValuePwr9));
    m_indicatorPowerByNumber.insert(10, PowerValueCtrl(ui->sldPwr10, ui->edValuePwr10, ui->lblValuePwr10));
    m_indicatorPowerByNumber.insert(11, PowerValueCtrl(ui->sldPwr11, ui->edValuePwr11, ui->lblValuePwr11));
    m_indicatorPowerByNumber.insert(12, PowerValueCtrl(ui->sldPwr12, ui->edValuePwr12, ui->lblValuePwr12));

    m_freqControls.insert(DataDefines::freq15Hz, ui->cbHz16);
    m_freqControls.insert(DataDefines::freq30Hz, ui->cbHz32);
    m_freqControls.insert(DataDefines::freq60Hz, ui->cbHz64);
    m_freqControls.insert(DataDefines::freq100Hz, ui->cbHz128);

    m_modulationControls.insert(DataDefines::modAm, ui->cbAm);
    m_modulationControls.insert(DataDefines::modFm, ui->cbFm);
}

void MainWindow::setPower(const QString &objName, const int power)
{
    auto spl = objName.split("Pwr");
    if (spl.size() > 1)
    {
        int chan = QString(spl.at(1)).toInt();
        if (chan >= 1 && chan <= 12)
        {
            m_log.append("<--- send 253 " + QString::number(chan) + " " + QString::number(power));
//            qDebug() << "<--- send 253" << chan << power;
//            m_driver->setPower(static_cast<quint8>(chan), static_cast<quint8>(power));
            m_changed.insert(static_cast<quint8>(chan), static_cast<quint8>(power));
        }
    }
}

void MainWindow::setMode()
{
    DataDefines::Frequency freq = currentFrequency();
    DataDefines::Modulation modulation = currentModulation();

    m_log.append("<--- send 253 13 freq:" +
                            QString::number(freq) + " " +
                 "modul:" + QString::number(modulation) + " " +
                 "pause:" + QString::number(m_isPause) + " " +
                 "reserve:" + QString::number(m_reserve));
    m_driver->setMode(freq, modulation, m_isPause, m_reserve);
}

DataDefines::Frequency MainWindow::currentFrequency() const
{
    DataDefines::Frequency freq = DataDefines::freq15Hz;
    if (ui->cbHz32->isChecked())
        freq = DataDefines::freq30Hz;
    else
    if (ui->cbHz64->isChecked())
        freq = DataDefines::freq60Hz;
    else
    if (ui->cbHz128->isChecked())
        freq = DataDefines::freq100Hz;
    return freq;
}

void MainWindow::setCurrentFrequency(const DataDefines::Frequency freq)
{
    m_freqControls.value(freq)->setChecked(true);
}

DataDefines::Modulation MainWindow::currentModulation() const
{
    DataDefines::Modulation modulation = DataDefines::modNone;
    if (ui->cbAm->isChecked() && !ui->cbFm->isChecked())
        modulation = DataDefines::modAm;
    else
    if (ui->cbFm->isChecked() && !ui->cbAm->isChecked())
        modulation = DataDefines::modFm;
    else
    if (ui->cbFm->isChecked() && ui->cbAm->isChecked())
        modulation = DataDefines::modBoth;
    return modulation;
}

void MainWindow::setCurrentModulation(const DataDefines::Modulation modulation)
{
    if (modulation == DataDefines::modAm || modulation == DataDefines::modFm)
        m_modulationControls.value(modulation)->setChecked(true);
    else
    if (modulation == DataDefines::modBoth)
    {
        m_modulationControls.value(DataDefines::modAm)->setChecked(true);
        m_modulationControls.value(DataDefines::modFm)->setChecked(true);
    }
    else
    if (modulation == DataDefines::modNone)
    {
        m_modulationControls.value(DataDefines::modAm)->setChecked(false);
        m_modulationControls.value(DataDefines::modFm)->setChecked(false);
    }
}

void MainWindow::assignMaxPowerValue()
{
    foreach (auto* sld, m_indicatorPowerValue.keys())
    {
        sld->setMaximum(m_maxPowerValue);
        m_indicatorPowerValue.value(sld)->setMaximum(m_maxPowerValue);
    }
}

QList<QSlider *> MainWindow::getCheckedSliders()
{
    QList<QSlider*> retval;
    foreach (auto* slider, m_indicatorPowerCheck.keys())
        if (m_indicatorPowerCheck.value(slider)->isChecked())
            retval << slider;

    return retval;
}

void MainWindow::saveSelected() const
{
    quint16 flags = 0;
    if (ui->cbPwr1->isChecked()) flags = flags | 0x1;
    if (ui->cbPwr2->isChecked()) flags = flags | 0x2;
    if (ui->cbPwr3->isChecked()) flags = flags | 0x4;
    if (ui->cbPwr4->isChecked()) flags = flags | 0x8;
    if (ui->cbPwr5->isChecked()) flags = flags | 0x10;
    if (ui->cbPwr6->isChecked()) flags = flags | 0x20;
    if (ui->cbPwr7->isChecked()) flags = flags | 0x40;
    if (ui->cbPwr8->isChecked()) flags = flags | 0x80;
    if (ui->cbPwr9->isChecked()) flags = flags | 0x100;
    if (ui->cbPwr10->isChecked()) flags = flags | 0x200;
    if (ui->cbPwr11->isChecked()) flags = flags | 0x400;
    if (ui->cbPwr12->isChecked()) flags = flags | 0x800;

    QSettings set(QApplication::instance()->organizationName(),
                  QApplication::instance()->applicationName());
    set.beginGroup("settings");
    set.setValue("Selected", flags);
    set.endGroup();
}

void MainWindow::restoreSelected()
{
    QSettings set(QApplication::instance()->organizationName(),
                  QApplication::instance()->applicationName());
    set.beginGroup("settings");
    quint16 flags = static_cast<quint16>(set.value("Selected", 0).toInt());
    set.endGroup();

    ui->cbPwr1->setChecked(flags & 0x1);
    ui->cbPwr2->setChecked(flags & 0x2);
    ui->cbPwr3->setChecked(flags & 0x4);
    ui->cbPwr4->setChecked(flags & 0x8);
    ui->cbPwr5->setChecked(flags & 0x10);
    ui->cbPwr6->setChecked(flags & 0x20);
    ui->cbPwr7->setChecked(flags & 0x40);
    ui->cbPwr8->setChecked(flags & 0x80);
    ui->cbPwr9->setChecked(flags & 0x100);
    ui->cbPwr10->setChecked(flags & 0x200);
    ui->cbPwr11->setChecked(flags & 0x300);
    ui->cbPwr12->setChecked(flags & 0x400);
}

void MainWindow::saveSizeAndPosition()
{
    QSettings set(QApplication::instance()->organizationName(),
                  QApplication::instance()->applicationName());
    set.beginGroup("MainWindow");
    set.setValue("Geometry", saveGeometry());
    set.endGroup();
}

void MainWindow::restoreSizeAndPosition()
{
    QSettings set(QApplication::instance()->organizationName(),
                  QApplication::instance()->applicationName());
    set.beginGroup("MainWindow");
    auto geometry = set.value("Geometry", QVariant()).toByteArray();
    set.endGroup();
    restoreGeometry(geometry);
}
