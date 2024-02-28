#include "markers.h"
#include <tf2/utils.h>
#include "marker_namespace.h"
#include "../node_manager.h"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

namespace camp_ros
{

Markers::Markers(MapItem* parent, NodeManager* node_manager, QString topic, QString topic_type):
  Layer(parent, node_manager, topic), topic_(topic.toStdString())
{
  qRegisterMetaType<MarkerData>("MarkerData");

  connect(this, &Markers::newMarkerData, this, &Markers::updateMarker);

  if(topic_type == "visualization_msgs/MarkerArray")
  {
    marker_array_subscription_ = node_manager->node()->create_subscription<visualization_msgs::msg::MarkerArray>(topic_, 10, std::bind(&Markers::markerArrayCallback, this, std::placeholders::_1));
    setStatus("[visualization_msgs/MarkerArray]");
  }
  else if(topic_type == "visualization_msgs/Marker")
  {
    marker_subscription_ = node_manager->node()->create_subscription<visualization_msgs::msg::Marker>(topic_, 10, std::bind(&Markers::markerCallback, this, std::placeholders::_1));
    setStatus("[visualization_msgs/Marker]");

  }
}


void Markers::markerArrayCallback(const visualization_msgs::msg::MarkerArray &data)
{
  addMarkers(data.markers);
}

void Markers::markerCallback(const visualization_msgs::msg::Marker &data)
{
  std::vector<visualization_msgs::msg::Marker> markers;
  markers.push_back(data);
  addMarkers(markers);
}

void Markers::addMarkers(const std::vector<visualization_msgs::msg::Marker> &markers)
{
  for(auto m: markers)
  {
    try
    {
      MarkerData marker_data;
      marker_data.marker = m;
      if(m.action == visualization_msgs::msg::Marker::ADD)
      {
        marker_data.position = transformToWebMercator(m.pose, m.header);
        marker_data.rotation = tf2::getYaw(m.pose.orientation);
      }
      emit newMarkerData(marker_data);
    }
    catch (tf2::TransformException &ex)
    {
      rclcpp::Clock clock;
      RCLCPP_WARN_STREAM_THROTTLE(node_manager_->node()->get_logger(), clock, 2000, "Unable to find transform to earth for marker " << m.ns << ": " << m.id << " at lookup time: " << rclcpp::Time(m.header.stamp).seconds() << " now: " << node_manager_->node()->get_clock()->now().seconds() << " source frame: " << m.header.frame_id << " what: " << ex.what());
    }
  }
}

MarkerNamespace* Markers::markerNamespace(const QString& marker_namespace) const
{
  for(auto item: childItems())
  {
    auto ns = qgraphicsitem_cast<MarkerNamespace*>(item);
    if(ns && ns->objectName() == marker_namespace)
      return ns;
  }
  return nullptr;
}

void Markers::updateMarker(const MarkerData& data)
{
  auto marker_namespace = markerNamespace(data.marker.ns.c_str());
  if(!marker_namespace)
    marker_namespace = new MarkerNamespace(this, node_manager_, data.marker.ns.c_str());
  marker_namespace->updateMarker(data);
}

} // namespace camp_ros
