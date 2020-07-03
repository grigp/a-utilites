#ifndef EDITSETTINGSDIALOG_H
#define EDITSETTINGSDIALOG_H

#include <QDialog>

#include "serialportdefines.h"

namespace Ui {
class EditSettingsDialog;
}

class EditSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditSettingsDialog(QWidget *parent = nullptr);
    ~EditSettingsDialog();

    SerialPortDefines::Ports port() const;
    void setPort(const SerialPortDefines::Ports port);

    int maxPowerValue() const;
    void setMaxPowerValue(const int mpv);

    int delaySendByte() const;
    void setDelaySendByte(const int delay);

private:
    Ui::EditSettingsDialog *ui;

    void loadListPorts();
};

#endif // EDITSETTINGSDIALOG_H
