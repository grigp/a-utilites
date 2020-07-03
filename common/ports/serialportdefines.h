#ifndef SERIALPORTDEFINES_H
#define SERIALPORTDEFINES_H

#include <QString>
#include <QSerialPort>

namespace SerialPortDefines {

///<Структура с настройками порта
struct Settings
{
    QString name;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;

    Settings(qint32 br,
             QSerialPort::DataBits db,
             QSerialPort::Parity pty,
             QSerialPort::StopBits sb,
             QSerialPort::FlowControl fc)
        : baudRate(br)
        , dataBits(db)
        , parity(pty)
        , stopBits(sb)
        , flowControl(fc)
    {name = "com1";}
    Settings(){}
};

enum Ports {
      pcEmulation = -1
    , pcUsb = 0
    , pcCom1, pcCom2, pcCom3, pcCom4, pcCom5, pcCom6, pcCom7, pcCom8, pcCom9, pcCom10
    , pcCom11, pcCom12, pcCom13, pcCom14, pcCom15, pcCom16, pcCom17, pcCom18, pcCom19, pcCom20
    , pcCom21, pcCom22, pcCom23, pcCom24, pcCom25, pcCom26, pcCom27, pcCom28, pcCom29, pcCom30
    , pcCom31, pcCom32, pcCom33, pcCom34, pcCom35, pcCom36, pcCom37, pcCom38, pcCom39, pcCom40
    , pcCom41, pcCom42, pcCom43, pcCom44, pcCom45, pcCom46, pcCom47, pcCom48, pcCom49, pcCom50
    , pcCom51, pcCom52, pcCom53, pcCom54, pcCom55, pcCom56, pcCom57, pcCom58, pcCom59, pcCom60
    , pcCom61, pcCom62, pcCom63, pcCom64, pcCom65, pcCom66, pcCom67, pcCom68, pcCom69, pcCom70
    , pcCom71, pcCom72, pcCom73, pcCom74, pcCom75, pcCom76, pcCom77, pcCom78, pcCom79, pcCom80
    , pcCom81, pcCom82, pcCom83, pcCom84, pcCom85, pcCom86, pcCom87, pcCom88, pcCom89, pcCom90
    , pcCom91, pcCom92, pcCom93, pcCom94, pcCom95, pcCom96, pcCom97, pcCom98, pcCom99, pcCom100
    , pcCom101, pcCom102, pcCom103, pcCom104, pcCom105, pcCom106, pcCom107, pcCom108, pcCom109, pcCom110
    , pcCom111, pcCom112, pcCom113, pcCom114, pcCom115, pcCom116, pcCom117, pcCom118, pcCom119, pcCom120
    , pcCom121, pcCom122, pcCom123, pcCom124, pcCom125, pcCom126, pcCom127
};

/*!
 * \brief Возвращает имя порта для показа в списке в формате COM <номер>
 */
QString portName(const Ports port);

/*!
 * \brief Возвращает имя порта для использования в QSerialPort в формате COM<номер>
 */
QString serialPortName(const Ports port);

QList<Ports> comPorts();
QList<Ports> comUsbPorts();
QList<Ports> allPorts();

}

#endif // SERIALPORTDEFINES_H
