#ifndef CAMP_MISSION_MANAGER_H
#define CAMP_MISSION_MANAGER_H

#include <QWidget>
#include "rclcpp/rclcpp.hpp"
#include "project11_msgs/msg/heartbeat.hpp"
#include "project11_nav_msgs/msg/geooccupancyvectormap.hpp"

namespace Ui
{
class MissionManager;
}

class QGeoCoordinate;

class MissionManager: public QWidget
{
  Q_OBJECT

public:
  explicit MissionManager(QWidget *parent =0);
  ~MissionManager(); 

public slots:
  void updateRobotNamespace(QString robot_namespace);

  void sendCommand(const QString& command);

  void sendNextItem();
  void restartMission();
  void clearTasks();
  void sendCancelOverride();

  void sendMissionPlan(QString const &plan);
  void appendMission(QString const &plan);
  void prependMission(QString const &plan);
  void updateMission(QString const &plan);
  
  void sendHover(QGeoCoordinate const &targetLocation);
  void sendGoto(QGeoCoordinate const &targetLocation);
  void sendIdle();

  void sendGotoLine(int waypoint_index);
  void sendStartLine(int waypoint_index);

  void sendAvoidanceAreas(const project11_nav_msgs::GeoOccupancyVectorMap& map);


private slots:
  void updateMissionStatus(QString const &status);
  void on_gotoLinePushButton_clicked(bool checked);
  void on_startLinePushButton_clicked(bool checked);
  void on_cancelOverridePushButton_clicked(bool checked);

  void on_nextMissionItemPushButton_clicked(bool checked);
  void on_restartMissionPushButton_clicked(bool checked);
  void on_clearTasksPushButton_clicked(bool checked);
  
  void on_missionStatusTextBrowser_customContextMenuRequested(const QPoint &pos);

private:
  void missionStatusCallback(const project11_msgs::Heartbeat::ConstPtr& message);

  Ui::MissionManager* m_ui;
  ros::Subscriber m_mission_status_subscriber;
  ros::Publisher m_send_command_publisher;
  ros::Publisher send_avoidance_costmap_publisher_;


};


#endif