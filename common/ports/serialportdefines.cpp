#include "serialportdefines.h"

#include <QCoreApplication>

namespace  {

QList<SerialPortDefines::Ports> portsEnum(const SerialPortDefines::Ports start,
                                          const SerialPortDefines::Ports finish)
{
    QList<SerialPortDefines::Ports> retval;
    for (int portId = static_cast<int>(start); portId <= static_cast<int>(finish); ++portId)
        retval.append(static_cast<SerialPortDefines::Ports>(portId));
    return retval;
}

}



QString SerialPortDefines::portName(const SerialPortDefines::Ports port)
{
    if (port == pcEmulation)
        return QCoreApplication::tr("Эмуляция");
    else
    if (port == pcUsb)
        return "USB";
    else
        return QString("COM %1").arg(static_cast<int>(port));
}

QString SerialPortDefines::serialPortName(const SerialPortDefines::Ports port)
{
    if (static_cast<int>(port) > 0)
        return QString("COM%1").arg(static_cast<int>(port));
    return QString("");
}

QList<SerialPortDefines::Ports> SerialPortDefines::comPorts()
{
    return portsEnum(pcCom1, pcCom127);
}

QList<SerialPortDefines::Ports> SerialPortDefines::comUsbPorts()
{
    return portsEnum(pcUsb, pcCom127);
}

QList<SerialPortDefines::Ports> SerialPortDefines::allPorts()
{
    return portsEnum(pcEmulation, pcCom127);
}
