#include "driver.h"

#include <QTimerEvent>
#include <QTime>
#include <QDebug>

#include "serialport.h"

Driver::Driver(QObject *parent) : QObject(parent)
{

}

void Driver::start()
{
    if (!m_trdInput)
    {
        m_trdInput = new QThread();
        m_port = new SerialPort();
        m_port->toThread(m_trdInput);

        connect(m_port, &SerialPort::error_, this, &Driver::on_error);
        connect(m_trdInput, &QThread::started, m_port, &SerialPort::processPort);
        connect(m_port, &SerialPort::finishedPort, m_trdInput, &QThread::quit);
        connect(m_trdInput, &QThread::finished, m_port, &SerialPort::deleteLater);
        connect(m_port, &SerialPort::finishedPort, m_trdInput, &QThread::deleteLater);
        connect(this, &Driver::portSettings, m_port, &SerialPort::WriteSettingsPort);
        connect(this, &Driver::connectPort, m_port, &SerialPort::ConnectPort);
        connect(this, &Driver::disconnectPort, m_port, &SerialPort::DisconnectPort);
        connect(m_port, &SerialPort::outPortD, this, &Driver::on_readData);
        connect(this, &Driver::writeData, m_port, &SerialPort::WriteToPort);

        m_tmCommError = startTimer(3000);

        auto sps = getSerialPortSettings();
        emit portSettings(SerialPortDefines::serialPortName(m_portName),
                          sps.baudRate, sps.dataBits, sps.parity, sps.stopBits, sps.flowControl);
        emit connectPort();

        m_trdInput->start();

        startSendDataMode();
    }
}

void Driver::stop()
{
    stopSendDataMode();
    emit disconnectPort();
}

void Driver::setPower(const quint8 channel, const quint8 power)
{
    quint8 b1 = 0x80 + (ccPower << 4) + channel;
    quint8 b2 = power;

    QByteArray cmd;
    cmd.resize(1);
    cmd[0] = b1;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = b2;
    emit writeData(cmd);

    delay(m_delaySendByte);

//    qDebug() << "- set power" << channel << power;
}

void Driver::setMode(const DataDefines::Frequency freq, const DataDefines::Modulation modulation, const bool pause, const bool reserve)
{
    quint8 val = 0;

    quint8 keyFreq = m_frequencyByCode.key(freq);
    quint8 keyMod = m_modulationByCode.key(modulation);
    val = (keyFreq << 2) | keyMod;
    if (pause)
        val = val | 0x40;
    if (reserve)
        val = val | 0x80;

    QByteArray cmd;
    cmd.resize(1);
    quint8 b1 = 0x80 + (ccMode << 4) + cpMode;
    cmd[0] = b1;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = val;
    emit writeData(cmd);

    delay(m_delaySendByte);


//    qDebug() << "- set mode" << freq << modulation << QString::number(b1, 16) << QString::number(val, 16);
}

void Driver::on_readData(const QByteArray data)
{
    for (int i = 0; i < data.count(); i++)
    {
        quint8 B = data[i];
        assignByteFromDevice(B);
    }
}

void Driver::on_error(const QString &err)
{
    Q_UNUSED(err);
}

SerialPortDefines::Settings Driver::getSerialPortSettings()
{
    return SerialPortDefines::Settings(115200,
                                       QSerialPort::Data8,
                                       QSerialPort::NoParity,
                                       QSerialPort::OneStop,
                                       QSerialPort::NoFlowControl);
}

void Driver::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_tmCommError)
    {
        if (m_blockCount == m_blockCountPrev)
        {
            if (!m_isCommunicationError)
            {
                m_isCommunicationError = true;
                emit communicationError(driverName(), SerialPortDefines::portName(m_portName), EC_NoData);
            }
        }
        else
            m_isCommunicationError = false;
        m_blockCountPrev = m_blockCount;
    }
}

void Driver::startSendDataMode()
{
    QByteArray cmd;
    cmd.resize(1);
    quint8 b = 0x80 + (ccStart << 4) + cpStart;
    cmd[0] = b;
    emit writeData(cmd);
    delay(m_delaySendByte);

    cmd[0] = cpStart;
    emit writeData(cmd);
    delay(m_delaySendByte);
}

void Driver::stopSendDataMode()
{
    QByteArray cmd;
    cmd.resize(1);
    quint8 b = 0x80 + (ccStop << 4) + cpStop;
    cmd[0] = b;
    emit writeData(cmd);
    delay(m_delaySendByte);
}

void Driver::assignByteFromDevice(quint8 b)
{
    if (!m_isSynchro)
    {
        if ((b & 0x80) != 0)
        {
            m_isSynchro = true;
            m_byteCounter = 0;
            m_byte1 = b;
        }
    }
    else
    {
        ++m_byteCounter;
        if (m_byteCounter == 1 && ((b & 0x80) == 0))
        {
            m_byte2 = b;
            m_command = (m_byte1 & 0x70) >> 4;
            m_param = m_byte1 & 0x0F;
            m_data = m_byte2 & 0x7F;

            m_isSynchro = false;
            m_byteCounter = 0;

            commandWorking();

            ++m_blockCount;
        }
    }
}

void Driver::commandWorking()
{
    if (m_command == ccPowerDevice && m_param >= cpPower1 && m_param <= cpPower12)
    {
        DataDefines::PowerData pd;
        pd.channel = m_param;
        pd.power = m_data;
        emit sendPower(pd);
    }
    else
    if (m_command == ccModeDevice && m_param == cpMode)
    {
        DataDefines::ModeData md;

        quint8 mdl = m_data & 0x3;
        if (m_modulationByCode.contains(mdl))
            md.modulation = m_modulationByCode.value(mdl);
        else
            md.modulation = DataDefines::modAm;

        quint8 freq = (m_data & 0x3C) >> 2;
        if (m_frequencyByCode.contains(freq))
            md.freq = m_frequencyByCode.value(freq);
        else
            md.freq = DataDefines::freq60Hz;

        md.pause = (m_data & 0x40) != 0;
        md.reserve = (m_data & 0x80) != 0;

        emit sendMode(md);
    }
    else
    if (m_command == ccCodeUniqueLo && m_param == cpCodeUniqueLo)
    {
        m_codeLo = m_data;
    }
    else
    if (m_command == ccCodeUniqueHi && m_param == cpCodeUniqueHi)
    {
        m_codeHi = m_data;

        DataDefines::CodeUniqueData cud;
        cud.code = m_codeHi * 265 + m_codeLo;
        emit sendCodeUnique(cud);
    }
    if (m_command == ccBatteryLevel && m_param == cpBatteryLevel)
    {
        DataDefines::BatteryLevelData bld;
        bld.level = m_data;
        emit sendBatteryLevel(bld);
    }
}

void Driver::delay(const int msec)
{
    QTime time;
    time.start();
    for(;time.elapsed() < msec;)
    {
    }
}
