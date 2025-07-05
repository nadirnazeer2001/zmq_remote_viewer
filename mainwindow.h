#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <zmq.hpp>
#include <QTimer>
#include <atomic>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();  // Must be declared

private slots:
    void on_connectButton_clicked();  // Auto-connected slot
    void updateFrame();
    void updateFPS();  // FPS counter update
    void connectToCamera();  // Connection handler
    void disconnectFromCamera();  // Disconnection handler

private:
    Ui::MainWindow *ui;
    std::unique_ptr<zmq::context_t> zmqContext;
    std::unique_ptr<zmq::socket_t> zmqSocket;
    QTimer *frameTimer;
    QTimer *fpsTimer;
    std::atomic<int> frameCounter;
    bool isConnected;

    QImage processReceivedData(const zmq::message_t &message);
};

#endif // MAINWINDOW_H
