#ifndef DRIVER_H
#define DRIVER_H

#include <QObject>
#include <QThread>
#include <QMap>

#include "serialportdefines.h"
#include "datadefines.h"

class SerialPort;

/*!
 * \brief Класс драйвера устройства Driver class
 */
class Driver : public QObject
{
    Q_OBJECT
public:
    explicit Driver(QObject *parent = nullptr);

    static const int DelaySendByteDefault = 50;

    enum ErrorCodes
    {
          EC_NoError = 0
        , EC_NoData = 1
        , EC_User = 100
    };

    QString driverUid() const {return UID;}
    QString driverName() const {return NAME;}

    /*!
     * \brief Запуск передачи данных
     */
    virtual void start();
    /*!
     * \brief Останов передачи данных
     */
    virtual void stop();

    SerialPortDefines::Ports portName() const {return m_portName;}
    void setPortName(const SerialPortDefines::Ports portName) {m_portName = portName;}

    void setPower(const quint8 channel, const quint8 power);
    void setMode(const DataDefines::Frequency freq,
                 const DataDefines::Modulation modulation,
                 const bool pause,
                 const bool reserve);

    int delaySendByte() const {return m_delaySendByte;}
    void setDelaySendByte(const int delay) {m_delaySendByte = delay;}

signals:
    void sendPower(DataDefines::PowerData data);
    void sendMode(DataDefines::ModeData data);
    void sendCodeUnique(DataDefines::CodeUniqueData data);
    void sendBatteryLevel(DataDefines::BatteryLevelData data);
    void communicationError(const QString &drvName, const QString &port, const int errorCode);

    void connectPort();
    void disconnectPort();
    void portSettings(const QString &name,
                      const int baudrate, const int DataBits, const int Parity,
                      const int StopBits, const int FlowControl);
    void writeData(const QByteArray data);
    void error(const int errorCode);


protected slots:
    virtual void on_readData(const QByteArray data);
    virtual void on_error(const QString &err);

protected:
    SerialPort* port() const {return m_port;}
    QThread* trdInput() const {return m_trdInput;}

    /*!
     * \brief Возвращает настройки порта
     */
    virtual SerialPortDefines::Settings getSerialPortSettings();

    void incBlockCount() {++m_blockCount;}
    int blockCount() const {return m_blockCount;}

    void timerEvent(QTimerEvent *event) override;

private:

    const quint8 MarkerCode = 253;
    enum CommandCode
    {
          ccPower = 1
        , ccMode = 2
        , ccReset = 3
        , ccStart = 3
        , ccStop = 3
        , ccBatteryLevel = 4
        , ccCodeUniqueLo = 5
        , ccCodeUniqueHi = 6
        , ccPowerDevice = 7
        , ccModeDevice = 7
    };

    enum CommandParam
    {
          cpPower1 = 1
        , cpPower2 = 2
        , cpPower3 = 3
        , cpPower4 = 4
        , cpPower5 = 5
        , cpPower6 = 6
        , cpPower7 = 7
        , cpPower8 = 8
        , cpPower9 = 9
        , cpPower10 = 10
        , cpPower11 = 11
        , cpPower12 = 12
        , cpMode    = 13
        , cpReset = 14
        , cpStart = 13
        , cpStop = 12
        , cpBatteryLevel = 15
        , cpCodeUniqueLo = 0
        , cpCodeUniqueHi = 1
    };

    QMap<int, DataDefines::Modulation> m_modulationByCode {
        std::pair<int, DataDefines::Modulation> (1, DataDefines::modAm)
      , std::pair<int, DataDefines::Modulation> (2, DataDefines::modFm)
    };
    QMap<int, DataDefines::Frequency> m_frequencyByCode {
        std::pair<int, DataDefines::Frequency> (1, DataDefines::freq100Hz)
      , std::pair<int, DataDefines::Frequency> (2, DataDefines::freq60Hz)
      , std::pair<int, DataDefines::Frequency> (4, DataDefines::freq30Hz)
      , std::pair<int, DataDefines::Frequency> (8, DataDefines::freq15Hz)
    };

    void startSendDataMode();
    void stopSendDataMode();

    /*!
     * \brief Обрабатывает принятый байт из пакета данных байт
     * \param b - текущий байт
     */
    void assignByteFromDevice(quint8 b);

    /*!
     * \brief Обработка команды
     */
    void commandWorking();

    void delay(const int msec);

    int m_blockCount {0};                    ///< Счетчик пакетов
    int m_blockCountPrev {0};
    int m_tmCommError {-1};                  ///< id таймера ошибки связи
    bool m_isCommunicationError {false};     ///< признак ошибки связи

    SerialPortDefines::Ports m_portName;
    SerialPort *m_port {nullptr};
    QThread *m_trdInput {nullptr};

    bool m_isSynchro {false};      ///< Синхронизированы ли мы
    int m_byteCounter {0};         ///< Счетчик байтов
    quint8 m_byte1 {0};            ///< Первый байт команды
    quint8 m_byte2 {0};            ///< Второй байт команды
    quint8 m_command {0};          ///< Команда
    quint8 m_param {0};            ///< Параметр команды
    quint8 m_data {0};             ///< Данные передачи или приема
    quint8 m_codeLo {0};           ///< Младший байт уникального кода
    quint8 m_codeHi {0};           ///< Старший байт уникального кода

    const QString UID = "{9bfc97a3-282e-4f5a-9415-4bc8b5de8cdb}";
    const QString NAME = "Драйвер блока генерации";

    int m_delaySendByte {0};
};

#endif // DRIVER_H
