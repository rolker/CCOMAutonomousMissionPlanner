#include "mainwindow.h"
#include <QApplication>
#include <QString>
#include <QFileInfo>

#include "rclcpp/rclcpp.hpp"

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    //ros::init(argc,argv, "CCOMAutonomousMissionPlanner", ros::init_options::AnonymousName);
    QApplication a(argc, argv);
    MainWindow w;
    w.setStyleSheet("QSplitter::handle{background: #8080A0;}");
    w.show();
    
    
    for(int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if(arg.endsWith(".json", Qt::CaseInsensitive))
            //w.open(arg);
            QMetaObject::invokeMethod(&w, "open", Qt::QueuedConnection, Q_ARG(QString, arg));
        else if(QFileInfo(arg).isDir())
            w.setWorkspace(arg);
        else // try background
            //w.openBackground(arg);
            QMetaObject::invokeMethod(&w, "openBackground", Qt::QueuedConnection, Q_ARG(QString, arg));
    }

    return a.exec();
}
