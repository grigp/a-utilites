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

        m_tmCommError = startTimer(1000);

        auto sps = getSerialPortSettings();
        emit portSettings(SerialPortDefines::serialPortName(m_portName),
                          sps.baudRate, sps.dataBits, sps.parity, sps.stopBits, sps.flowControl);
        emit connectPort();

        m_trdInput->start();
    }
}

void Driver::stop()
{
    emit disconnectPort();

}

void Driver::setPower(const quint8 channel, const quint8 power)
{
    QByteArray cmd;
    cmd.resize(1);
    cmd[0] = MarkerCode;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = channel;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = power;
    emit writeData(cmd);

    delay(m_delaySendByte);

//    QByteArray cmd;
//    cmd.resize(3);
//    cmd[0] = MarkerCode;
//    cmd[1] = channel;
//    cmd[2] = power;
//    emit writeData(cmd);
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
    cmd[0] = MarkerCode;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = ccMode;
    emit writeData(cmd);

    delay(m_delaySendByte);

    cmd[0] = val;
    emit writeData(cmd);

    delay(m_delaySendByte);


//    QByteArray cmd;
//    cmd.resize(3);
//    cmd[0] = MarkerCode;
//    cmd[1] = ccMode;
//    cmd[2] = val;
//    emit writeData(cmd);
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

void Driver::assignByteFromDevice(quint8 b)
{
    if (!m_isSynchro)
    {
        if (b == MarkerCode)
        {
            m_isSynchro = true;
            m_byteCounter = 0;
        }
    }
    else
    {
        ++m_byteCounter;
        if (m_byteCounter == 1)
            m_command = b;
        else
        if (m_byteCounter == 2)
        {
            m_data = b;
            m_isSynchro = false;
            m_byteCounter = 0;

            commandWorking();

            ++m_blockCount;
        }
    }
}

void Driver::commandWorking()
{
    if (m_command >= ccPower1 && m_command <= ccPower12)
    {
        DataDefines::PowerData pd;
        pd.channel = m_command;
        pd.power = m_data;
        emit sendPower(pd);
    }
    else
    if (m_command == ccMode)
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
    if (m_command == ccCodeUniqueLo)
    {
        m_codeLo = m_data;
    }
    else
    if (m_command == ccCodeUniqueHi)
    {
        m_codeHi = m_data;

        DataDefines::CodeUniqueData cud;
        cud.code = m_codeHi * 265 + m_codeLo;
        emit sendCodeUnique(cud);
    }
    if (m_command == ccBatteryLevel)
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
