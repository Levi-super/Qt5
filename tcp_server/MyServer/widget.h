#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include<QtNetwork>
#include<QTcpServer>
#include<QTcpSocket>
#include<QImage>
#include<QImageReader>
#include<QTime>
#include<QDebug>
#include<QMessageBox>
#include<QFileDialog>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/core/core.hpp>

using namespace cv;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private:
    Ui::Widget *ui;
    QTcpServer* ServerListenSocket;
    QTcpSocket* ClientConnectedSocket;

    VideoCapture cap;
    VideoCapture cap_fake;

    QTimer* timer;
    quint16 clientRequestSize;

private slots:
    void clientConnection();
    void readClientRequest();
    void SendData();

    void on_pushButton_clicked();
    void on_pbcontinue_clicked();
};

#endif // WIDGET_H
