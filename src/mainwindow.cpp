#include <QFileInfo>
#include <QDataStream>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

// includes for the serial port
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_kitConnected = false;
    m_receiveThread = NULL;
    m_sendThread = NULL;
    m_rxHistory = "";
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_browseForBitfile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Select bitfile", "", "*.bit");

    ui->bitfileLocation->setText(fileName);
}

void MainWindow::on_connectDisconnect_clicked()
{
    if(!m_kitConnected)
    {
        connectToKit();
    }
    else
    {
        disconnectFromKit();
    }
}

void MainWindow::dataReceived(QByteArray data)
{
    m_rxHistory += QString::fromLocal8Bit(data);

    ui->rxText->setPlainText(m_rxHistory);
}

void MainWindow::connectToKit()
{
    if(m_kitConnected)
        return;

    m_serialPortDescriptor = open (ui->devNode->text().toLocal8Bit().data(), O_RDWR | O_NOCTTY | O_SYNC);

    if (m_serialPortDescriptor < 0)
    {
        errorMessage("error opening port");
        return;
    }

    // set speed to 115,200 bps, 8n1 (no parity)
    if (set_interface_attribs (m_serialPortDescriptor, B115200, 0) != 0)
    {
        errorMessage("could not set interface attribs");
        return;
    }

    set_blocking (m_serialPortDescriptor, 0);                    // set no blocking

    m_receiveThread = new DataReceiveThread(m_serialPortDescriptor, this);

    connect(this, SIGNAL(stopRequest()), m_receiveThread, SLOT(stopRequest()));
    connect(m_receiveThread, SIGNAL(dataReceived(QByteArray)), this, SLOT(dataReceived(QByteArray)), Qt::BlockingQueuedConnection);

    // start the data receive thread
    m_receiveThread->start();


    m_kitConnected = true;
    ui->connectDisconnect->setText("Disconnect");
}

void MainWindow::disconnectFromKit()
{
    if(!m_kitConnected)
        return;

    // terminate the receiver thread
    emit stopRequest();
    qDebug() << "waiting for receiver thread to terminate...\n";
    while(m_receiveThread->isRunning());
    qDebug() << "receiver thread terminated \n";
    delete m_receiveThread;
    m_receiveThread = NULL;


    ::close(m_serialPortDescriptor);
    m_kitConnected = false;
    ui->connectDisconnect->setText("Connect to kit");
}

void MainWindow::errorMessage(QString message)
{
    QMessageBox::critical(this, "Error", message, "OK");
}

void MainWindow::sendBinaryFile(QString fileName)
{
    QFile f(fileName);
    if(!(f.open(QIODevice::ReadOnly)))
    {
        errorMessage("Could not open file");
        return;
    }

    qDebug() << "sending binary file of size " << f.size();

    sendDataByteArray(f.readAll());

    f.close();
}

void MainWindow::sendData(QString data, bool addNewLine)
{
    if(!m_kitConnected)
        return;

    // zero-terminate the buffer and add newline if requested
    if(addNewLine)
        data = data+"\n";
    QByteArray buffer = data.toLocal8Bit();
    buffer.append((char) 0);

    sendDataByteArray(buffer);
}

void MainWindow::sendDataByteArray(QByteArray buffer)
{
    if(!m_kitConnected)
        return;

    if(buffer.size() > 1024)
    {
        if(m_sendThread)
        {
            qDebug() << "send thread is already active, wait until finished";
            return;
        }

        m_sendThread = new DataSendThread(m_serialPortDescriptor, buffer, this);

        connect(m_sendThread, SIGNAL(progress(uint,uint)),this,SLOT(progress(uint,uint)));
        connect(m_sendThread, SIGNAL(sendThreadFinished()), this, SLOT(senderThreadFinished()));

        m_sendThread->start();
    }
    else
    {
        char * buf = buffer.data();
        unsigned int n = buffer.length();
        qDebug() << "sending data of length " << n << ", one-go";

        ::write(m_serialPortDescriptor, buf, n);
    }
}

void MainWindow::configureFPGA(QString bitFileName)
{
    if(!m_kitConnected)
        return;

    // stop the receiver thread
    emit stopRequest();
    qDebug() << "waiting for receiver thread to terminate...\n";
    while(m_receiveThread->isRunning());
    qDebug() << "receiver thread terminated \n";


    // read buffer
    char buf[1024];

    // step 1 req: request version
    sendData("get_ver");

    // step 1 ack: verify response
    int n = ::read(m_serialPortDescriptor, buf, 6);
    if(n != 6 || memcmp(buf, "3.0.2", 6) != 0)
    {
        qDebug() << "unexpected get_ver_response of len " << n << " : " << QByteArray(buf, n);
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }



    // step 2 req: load config 1
    sendData("load_config 1");

    // step 2 ack: verify response
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 3: drive_prog 0
    sendData("drive_prog 0");

    // step 3 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 4: drive_mode 7
    sendData("drive_mode 7");

    // step 4 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 5: spi_mode 1
    sendData("spi_mode 1");

    // step 5 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 6: drive_prog 1
    sendData("drive_prog 1");

    // step 6 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 7: read_init
    sendData("read_init");

    // step 7 verify
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    n = ::read(m_serialPortDescriptor, buf, 1);
    if(n != 1 || buf[0] != 1)
    {
        qDebug() << "unexpected read_init response of len " << n << " : " << QByteArray(buf, n);
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 8: drive_mode 8
    sendData("drive_mode 8");

    // step 8 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 9: fpga_rst 1
    sendData("fpga_rst 1");

    // step 9 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 10: send the bitfile data length
    QFileInfo f(bitFileName);
    sendData("ss_program " + QString::number(f.size()));

    // step 10 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 11: send the bitfile data
    sendBinaryFile(bitFileName);

    // likely to be a large operation, wait until finished
    while(m_sendThread)
        qApp->processEvents();  // keep the GUI updated in the meanwhile

    // step 11 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 12: read_init
    sendData("read_init");

    // step 12 verify
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    n = ::read(m_serialPortDescriptor, buf, 1);
    if(n != 1 || buf[0] != 0)
    {
        qDebug() << "unexpected step 12 read_init response of len " << n << " : " << QByteArray(buf, n).toHex();
        qDebug() << "continuing anyway...";
        //m_receiveThread->start();
        //return;
    }
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }


    // step 13: read_done
    sendData("read_done");

    // step 13 verify
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    n = ::read(m_serialPortDescriptor, buf, 1);
    if(n != 1 || buf[0] != 1)
    {
        qDebug() << "unexpected step 13 read_done response of len " << n << " : " << QByteArray(buf, n);
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 14: spi_mode 0
    sendData("spi_mode 0");

    // step 14 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }


    // step 15 req: load config 0
    sendData("load_config 0");

    // step 15 ack: verify response
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    // step 16: fpga_rst 0
    sendData("fpga_rst 0");

    // step 16 ack
    if(!waitForACK())
    {
        // restart the receiver thread
        m_receiveThread->start();
        return;
    }

    qDebug() << "Bitfile config succeeded!";

    ui->rxText->setPlainText(ui->rxText->toPlainText() + "\nBitfile config succeeded!");

    // restart the receiver thread
    m_receiveThread->start();
}

bool MainWindow::waitForACK()
{
    char buf[1024];
    int n = ::read(m_serialPortDescriptor, buf, 4);
    if(n != 4 || memcmp(buf, "ack\0", 4) != 0)
    {
        qDebug() << "unexpected ack response: " << QByteArray(buf, n);
        return false;
    }

    return true;
}

int MainWindow::set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        errorMessage ("error from tcgetattr");
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    //tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 10;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        errorMessage ("error from tcsetattr");
        return -1;
    }

    return 0;
}


void MainWindow::set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        errorMessage("error from tggetattr");
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    // tty.c_cc[VEOF] = '\n';
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        errorMessage ("error setting term attributes");
}

void MainWindow::on_send_clicked()
{
    if(ui->asText->isChecked())
        sendData(ui->txData->text(), true);
    else if(ui->asUInt->isChecked())
    {
        unsigned int i = ui->txData->text().toUInt();
        qDebug() << "sending uint value = " << i;
        QByteArray buf;
        QDataStream d(&buf, QIODevice::WriteOnly);

        d << i;

        qDebug() << "byte array = " << buf.toHex();
        sendDataByteArray(buf);
    }
}

void MainWindow::on_sendBinFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Select binary file to send", "", "*.*");

    sendBinaryFile(fileName);
}

void MainWindow::on_clearRXHistory_clicked()
{
    m_rxHistory = "";
    ui->rxText->setPlainText(m_rxHistory);
}

void MainWindow::on_doConfigure_clicked()
{
    configureFPGA(ui->bitfileLocation->text());
}

void MainWindow::progress(unsigned int current, unsigned int maximum)
{
    ui->progressBar->setMaximum(maximum);
    ui->progressBar->setValue(current);
}

void MainWindow::senderThreadFinished()
{
    while(m_sendThread->isRunning());
    qDebug() << "sender thread finished, destroying ";
    delete m_sendThread;
    m_sendThread = NULL;
}
