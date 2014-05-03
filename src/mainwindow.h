#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "datareceivethread.h"
#include "datasendthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_browseForBitfile_clicked();

    void on_connectDisconnect_clicked();

    void dataReceived(QByteArray data);

    void on_send_clicked();

    void on_sendBinFile_clicked();

    void on_clearRXHistory_clicked();

    void on_doConfigure_clicked();

    void progress(unsigned int current, unsigned int maximum);

    void senderThreadFinished();

signals:
    void stopRequest();

private:
    Ui::MainWindow *ui;
    bool m_kitConnected;
    QString m_portDevNode;
    QString m_rxHistory;

    void connectToKit();
    void disconnectFromKit();
    void errorMessage(QString message);

    void sendBinaryFile(QString fileName);
    void sendData(QString data, bool addNewLine=false);
    void sendDataByteArray(QByteArray data);
    void configureFPGA(QString bitFileName);
    bool waitForACK();

    DataReceiveThread * m_receiveThread;
    DataSendThread * m_sendThread;

    //low-level  serial port stuff
    int m_serialPortDescriptor;
    int set_interface_attribs (int fd, int speed, int parity);
    void set_blocking (int fd, int should_block);
};

#endif // MAINWINDOW_H
