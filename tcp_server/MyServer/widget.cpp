#include "widget.h"
#include "ui_widget.h"
#include "iostream"

using namespace std;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->pushButton->setEnabled(false);
    ui->pbcontinue->setEnabled(false);

    ServerListenSocket = new QTcpServer(this);	//服务器端监听套接字 ServerListenSocket
    if (!ServerListenSocket->listen(QHostAddress::Any, 8888))	//监听本机8888端口上的任意IP地址的连入
    {
        QMessageBox::critical(this, tr("Fortune Server"), tr("Unable to start the server:%l.").arg(ServerListenSocket->errorString()));
        ServerListenSocket->close();
        return;
    }
    connect(ServerListenSocket, SIGNAL(newConnection()), this, SLOT(clientConnection()));	//将监听套接字有新客户端的接入信号newConnection() 的与处理的槽函数clientConnection() 连接
    clientRequestSize = 0;	//客户端发送过来的第一段数据块的大小（字节数）
}

Widget::~Widget()
{
    delete ui;
}

void Widget::clientConnection()
{
    ui->info->append("An new client is connected!");

    ClientConnectedSocket = ServerListenSocket->nextPendingConnection();	//返回已连接套接字对象

    connect(ClientConnectedSocket, SIGNAL(readyRead()), this, SLOT(readClientRequest()));	//将已连接套接字对象的准备好可读信号readyRead与 readClientRequest()槽函数连接
    connect(ClientConnectedSocket, SIGNAL(disconnected()), ClientConnectedSocket,SLOT(deleteLater()));	//已连接套接字的断开信号与自身的稍后删除信号相连接
}

//读取客户端发送过来的 请求 (发送任意字节流，只是将其按收发双方约定的规则解释为 第一次交互的请求）
void Widget::readClientRequest()
{
    QDataStream in(ClientConnectedSocket);
    in.setVersion(QDataStream::Qt_5_9);

    //如果客户端发送过来的第一段数据块的大小为0，说明确实是第一次交互
    if (clientRequestSize == 0)
    {
        //客户端发送过来的第一段数据块的大小如果小于 64bit ，则说明：还未收到客户端发送过来的前64bit的数据，这64bit的数据存储了客户端第一次请求包的大小（字节数）
        if (ClientConnectedSocket->bytesAvailable() < sizeof(quint16))
        {
            return;	//返回，继续等待 接收数据，数据还在套接字缓存当中
        }
        else//如果 客户端发送过来的第一段数据块的大小>= 64bit 了
        {
            in >> clientRequestSize;//将数据的前64bit提取出来，存储到quint64 类型的clientRequestSize
        }
    }
    if (ClientConnectedSocket->bytesAvailable() < clientRequestSize)//当前套接字缓冲区中存储的数据如果小于clientRequestSize个字节
    {
        return;//返回，继续等待 接收数据，数据还在套接字缓存当中
    }

    quint8 requestType;

    in >> requestType;//从套接字缓冲区中读取 8bit的数据解释为quint8类型，存储到requestType中
    clientRequestSize = 0;
    if (requestType == 'R')  //如果requestType是 'R'  字符R的ASCII值
    {
        timer = new QTimer(this);//启动定时器

        timer->start(20);//设置30ms的定时周期

        connect(timer, SIGNAL(timeout()), this, SLOT(SendData()));//将30ms时间到与发送数据的 SendData() 连接

//        cap.open("E:/videos/worked.mp4");	 //用Opencv 的VideoCapture对象打开视频文件
//        cap_fake.open("E:/videos/original.mp4");

        cap.open("E:/videos/worked.mp4");	 //用Opencv 的VideoCapture对象打开视频文件
        cap_fake.open("E:/videos/original.mp4");

        cout<<cap.get(CAP_PROP_FRAME_COUNT)<<endl;

        ui->pushButton->setEnabled(true);
        if (!cap.isOpened()) {
            QMessageBox::information(this, tr("warrning"), tr("can't open video!"));
        }
    }
}

void Widget::SendData()
{
    Mat frame;
    Mat frame_fake;

    cap >> frame;	//从视频中提取一帧图像
    cap_fake >> frame_fake;

    if (frame.empty())//图像为空：视频播放结束或是空视频
    {
        //图像流传输结束：
        QByteArray finishFlag;
        QDataStream out(&finishFlag, QIODevice::WriteOnly);	//二进制只写输出流
        out.setVersion(QDataStream::Qt_5_9);	//输出流的版本
        out << (quint64)0xFFFFFFFFFFFFFFFF;
        ClientConnectedSocket->write(finishFlag);	//将整块数据写入套接字
        timer->stop();
        ClientConnectedSocket->close();
//        ServerListenSocket->close();

        QMessageBox::information(this, tr("warning"), tr("the video is end!"));
        return;
    }

    cvtColor(frame, frame, COLOR_BGR2RGB);//颜色转换

    QByteArray byte;	//The QByteArray class provides an array of bytes.
    QBuffer buf(&byte);		//缓存区域

    QImage image((unsigned const char*)frame.data, frame.cols, frame.rows, QImage::Format_RGB888);//Mat图像矩阵转为QImage

    //QString imageSize = "image size is:" + QString::number(frame.cols*frame.rows * 3) + " Bytes";
    //ui.info->addItem(imageSize);//图像的大小（字节数）

    image.save(&buf, "JPEG");	//将图像以jpeg的压缩方式压缩了以后保存在 buf当中


    //QString jpegImageSize =  "jpeg image size is " + QString::number(buf.size()) + " Bytes";
    //ui.info->addItem(jpegImageSize);	//压缩后的jpg图像的大小(字节数）

    QByteArray ss = qCompress(byte, 1);//将压缩后的jpg图像 再用qCompress 压缩 ，第二个参数1-9，9是最大压缩率

    //QString ssSize="ss's size is "+ QString::number(ss.size()) + " Bytes";
    //ui.info->addItem(ssSize);//用qCompress 压缩后的数据大小（字节数）

    //将压缩后的字节串数据编码成Base64方式，字节数会比压缩前稍微变多一些
    QByteArray vv = ss.toBase64();  // QByteArray QByteArray::toBase64() const : Returns a copy of the byte array, encoded as Base64.

    //QString vvSize = "vv's size is "  + QString::number(vv.size()) + " Bytes";
    //ui.info->addItem(vvSize);	//编码后的数据的大小

    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);	//二进制只写输出流
    out.setVersion(QDataStream::Qt_5_9);	//输出流的版本
    /*当操作复杂数据类型时，我们就要确保读取和写入时的QDataStream版本是一样的，简单类型，比如char，short，int，char* 等不需要指定版本也行*/

    /*上面这些编解码的过程肯定是会影响 时效性的,可以考虑只使用jpeg 压缩后就进行发送 。*/

    out << (quint64)0;	//写入套接字的经压缩-编码后的图像数据的大小
    out << vv;			//写入套接字的经压缩-编码后的图像数据

    out.device()->seek(0);
    out << (quint64)(ba.size() - sizeof(quint64));//写入套接字的经压缩-编码后的图像数据的大小

    ClientConnectedSocket->write(ba);	//将整块数据写入套接字

    cvtColor(frame_fake, frame_fake, COLOR_BGR2RGB);
    QImage image_fake((unsigned const char*)frame_fake.data, frame_fake.cols, frame_fake.rows, QImage::Format_RGB888);//Mat图像矩阵转为QImage

    ui->label->setPixmap(QPixmap::fromImage(image_fake));	//再界面上显示该图像    852*480
    ui->label->resize(image_fake.size());
    update();	//更新界面
}


void Widget::on_pushButton_clicked()
{
    timer->stop();
    ui->pbcontinue->setEnabled(true);
    ui->pushButton->setEnabled(false);
}

void Widget::on_pbcontinue_clicked()
{
    timer->start(20);
    ui->pushButton->setEnabled(true);
    ui->pbcontinue->setEnabled(false);
}
