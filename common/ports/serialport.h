#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
//#include <QtSerialPort/QserialPort>  // Обьявляем работу с портом
//#include <QtSerialPort/QSerialPortInfo>

#include "serialportdefines.h"

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    void toThread(QThread *thread);

signals:
    void finishedPort();             // Сигнал закрытия класса
    void error_(const QString &err);        // Сигнал ошибок порта
    void outPortS(const QString &data);     // Сигнал вывода полученных данных
    void outPortD(const QByteArray data);  // Сигнал вывода полученных данных

public slots:

    void DisconnectPort(); // Слот отключения порта

    void ConnectPort(void); // Слот подключения порта

    void WriteSettingsPort(QString name, int baudrate, int DataBits, int Parity, int StopBits, int FlowControl);     // Слот занесение настроек порта в класс

    void processPort(); // Тело

    void WriteToPort(QByteArray data); // Слот от правки данных в порт

private slots:
    void handleError(QSerialPort::SerialPortError error);  // Слот обработки ошибок
    void ReadInPort();                                     // Слот чтения из порта по ReadyRead

private:
    QSerialPort m_nativePort;
    SerialPortDefines::Settings m_SettingsPort;

};

#endif // SERIALPORT_H
