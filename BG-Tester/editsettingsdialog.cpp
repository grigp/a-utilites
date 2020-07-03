#include "editsettingsdialog.h"
#include "ui_editsettingsdialog.h"

EditSettingsDialog::EditSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditSettingsDialog)
{
    ui->setupUi(this);
    loadListPorts();
}

EditSettingsDialog::~EditSettingsDialog()
{
    delete ui;
}

SerialPortDefines::Ports EditSettingsDialog::port() const
{
    return static_cast<SerialPortDefines::Ports>(ui->cbComPort->currentData().toInt());
}

void EditSettingsDialog::setPort(const SerialPortDefines::Ports port)
{
    auto portName = SerialPortDefines::portName(port);
    ui->cbComPort->setCurrentText(portName);
}

int EditSettingsDialog::maxPowerValue() const
{
    return ui->edMaxPower->value();
}

void EditSettingsDialog::setMaxPowerValue(const int mpv)
{
    ui->edMaxPower->setValue(mpv);
}

int EditSettingsDialog::delaySendByte() const
{
    return  ui->edDelaySendByte->value();
}

void EditSettingsDialog::setDelaySendByte(const int delay)
{
    ui->edDelaySendByte->setValue(delay);
}

void EditSettingsDialog::loadListPorts()
{
    auto ports = SerialPortDefines::comPorts();
    foreach (auto port, ports)
        ui->cbComPort->addItem(SerialPortDefines::portName(port), port);
}
