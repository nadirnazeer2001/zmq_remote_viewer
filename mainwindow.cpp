#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , zmqContext(new zmq::context_t(1))
    , zmqSocket(nullptr)
    , frameTimer(new QTimer(this))
    , fpsTimer(new QTimer(this))
    , frameCounter(0)
    , isConnected(false)
{
    ui->setupUi(this);
    ui->camera->setStyleSheet("background-color: black;");

    // Set default values
    ui->ipaddress->setText("tcp://localhost");
    ui->port->setText("5555");

    // Setup timers
    connect(frameTimer, &QTimer::timeout, this, &MainWindow::updateFrame);
    connect(fpsTimer, &QTimer::timeout, this, &MainWindow::updateFPS);
    fpsTimer->start(1000);

    // Connect button - use correct name from your UI
    connect(ui->fetchstream, &QPushButton::clicked,  // Changed to fetchstream
            this, &MainWindow::on_connectButton_clicked);

    // UI setup
    if (ui->camera) {
        ui->camera->setScaledContents(false);
        ui->camera->setAlignment(Qt::AlignCenter);
    }
}

MainWindow::~MainWindow()
{
    disconnectFromCamera();
    delete ui;
}

void MainWindow::on_connectButton_clicked()
{
    if (isConnected) {
        disconnectFromCamera();
        ui->fetchstream->setText("Connect to Camera");
        ui->status->setText("Disconnected");
    } else {
        connectToCamera();
    }
}

void MainWindow::updateFPS()
{
    int currentFrames = frameCounter.exchange(0);
    ui->status->setText(QString("FPS: %1").arg(currentFrames));
    qDebug() << "Current FPS:" << currentFrames;
}

void MainWindow::connectToCamera()
{
    try {
        QString address = ui->ipaddress->text().trimmed();
        QString port = ui->port->text().trimmed();

        if (!address.startsWith("tcp://")) {
            address = "tcp://" + address;
        }

        QString fullAddress = address + ":" + port;
        qDebug() << "Connecting to:" << fullAddress;
        ui->status->setText("Connecting...");
        QApplication::processEvents(); // Update UI immediately

        // Initialize socket if needed
        if (!zmqSocket) {
            zmqSocket = std::make_unique<zmq::socket_t>(*zmqContext, ZMQ_SUB);
            zmqSocket->set(zmq::sockopt::subscribe, "");
            zmqSocket->set(zmq::sockopt::rcvtimeo, 1000); // Longer timeout
            zmqSocket->set(zmq::sockopt::linger, 0);
        }

        // Connect
        try {
            zmqSocket->connect(fullAddress.toStdString());
        } catch (...) {
            // If already connected, disconnect first
            zmqSocket->disconnect(fullAddress.toStdString());
            zmqSocket->connect(fullAddress.toStdString());
        }

        // Test connection with timeout
        zmq::message_t testMsg;
        if (zmqSocket->recv(testMsg, zmq::recv_flags::dontwait)) {
            isConnected = true;
            frameTimer->start(33);
            ui->status->setText("Connected");
        } else {
            ui->status->setText("Connected (Waiting for data...)");
            isConnected = true; // Still mark as connected
            frameTimer->start(33);
        }
    } catch (const zmq::error_t &e) {
        qCritical() << "Connection error:" << e.what();
        ui->status->setText("Connection failed");
        QMessageBox::critical(this, "Error",
            QString("Could not connect to %1\n%2")
            .arg(ui->ipaddress->text())
            .arg(e.what()));
    }
}

void MainWindow::disconnectFromCamera()
{
    frameTimer->stop();
    if (zmqSocket) {
        try {
            zmqSocket->close();
        } catch (...) {
            qWarning() << "Error closing socket";
        }
        zmqSocket.reset();
    }
    isConnected = false;
    if (ui->camera) {
        ui->camera->clear();
    }
}

void MainWindow::updateFrame()
{
    if (!isConnected || !zmqSocket) return;

    try {
        zmq::message_t message;
        if (zmqSocket->recv(message, zmq::recv_flags::dontwait)) {
            frameCounter++;

            // Debug: Save raw data occasionally
            if (frameCounter % 100 == 0) {
                std::ofstream debugFile("last_frame.zmq", std::ios::binary);
                debugFile.write(static_cast<const char*>(message.data()), message.size());
                debugFile.close();
            }

            QImage image = processReceivedData(message);
            if (!image.isNull() && ui->camera) {
                QPixmap pixmap = QPixmap::fromImage(image)
                    .scaled(ui->camera->size(),
                            Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);

                ui->camera->setPixmap(pixmap);

                // Optional: Save test frame
                if (frameCounter == 1) {
                    image.save("first_frame_received.png");
                }
            }
        }
    } catch (const zmq::error_t &e) {
        qWarning() << "Stream error:" << e.what();
        disconnectFromCamera();
        ui->status->setText("Disconnected (Error)");
        ui->fetchstream->setText("Connect to Camera");
    }
}

QImage MainWindow::processReceivedData(const zmq::message_t &message)
{
    try {
        // Basic validation
        if (message.size() < 100) {
            qWarning() << "Message too small:" << message.size() << "bytes";
            return QImage();
        }

        // Convert to OpenCV Mat
        std::vector<uchar> buffer(
            static_cast<const uchar*>(message.data()),
            static_cast<const uchar*>(message.data()) + message.size()
        );

        cv::Mat frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (frame.empty()) {
            qWarning() << "Failed to decode image frame";
            return QImage();
        }

        // Convert color space if needed
        if (frame.channels() == 3) {
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        } else if (frame.channels() == 1) {
            cv::cvtColor(frame, frame, cv::COLOR_GRAY2RGB);
        }

        // Create and return QImage
        return QImage(
            frame.data,
            frame.cols,
            frame.rows,
            frame.step,
            frame.channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Grayscale8
        ).copy(); // Important: create a deep copy

    } catch (const cv::Exception &e) {
        qCritical() << "OpenCV Error:" << e.what();
        return QImage();
    } catch (...) {
        qCritical() << "Unknown error processing frame";
        return QImage();
    }
}
