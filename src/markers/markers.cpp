#include "markers.h"
#include <QPainter>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>
#include "project11/gz4d_geo.h"
#include <QDebug>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include "backgroundraster.h"
#include <grid_map_ros/grid_map_ros.hpp>
#include <QTimer>

Markers::Markers(QWidget* parent, QGraphicsItem *parentItem):
  camp_ros::ROSWidget(parent),
  GeoGraphicsItem(parentItem)
{
  ui_.setupUi(this);
  connect(this, &Markers::newMarkersMadeAvailable, this, &Markers::newMarkersAvailable, Qt::QueuedConnection);
  connect(ui_.displayCheckBox, &QCheckBox::stateChanged, this, &Markers::visibilityChanged);
  ui_.displayCheckBox->setChecked(true);
}

QRectF Markers::boundingRect() const
{
  QRectF ret;
  BackgroundRaster* bg = findParentBackgroundRaster();
  if(!current_markers_.empty() && is_visible_)
  {
    for(auto ns: current_markers_)
      for(auto m: ns.second)
      {
        auto p = markerPath(*(m.second), bg);
        ret |= p.boundingRect();
      }
  }
  return ret;
}

void Markers::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  BackgroundRaster* bg = findParentBackgroundRaster();
  if(!current_markers_.empty() && is_visible_)
    for(auto ns: current_markers_)
      for(auto m: ns.second)
      {
        painter->save();
        QPen p;
        p.setColor(QColor(m.second->marker.color.r*255, m.second->marker.color.g*255, m.second->marker.color.b*255, m.second->marker.color.a*255));
        if(m.second->marker.type == visualization_msgs::msg::Marker::LINE_STRIP)
        {
          p.setWidthF(m.second->marker.scale.x/pixel_size_);
          // p.setWidth(0);
          // p.setCosmetic(true);
        }
        if(m.second->marker.type == visualization_msgs::msg::Marker::TEXT_VIEW_FACING)
          p.setCosmetic(true);
        painter->setPen(p);
        QBrush b = painter->brush();
        b.setColor(QColor(m.second->marker.color.r*255, m.second->marker.color.g*255, m.second->marker.color.b*255, m.second->marker.color.a*128));
        if(m.second->marker.type == visualization_msgs::msg::Marker::SPHERE || m.second->marker.type == visualization_msgs::msg::Marker::TEXT_VIEW_FACING)
          b.setStyle(Qt::BrushStyle::SolidPattern);
        painter->setBrush(b);
        painter->drawPath(markerPath(*m.second, bg));
        painter->restore();
      }
}

QPainterPath Markers::markerPath(const MarkerData& marker, BackgroundRaster* bg) const
{
  QPainterPath path;

  if(bg)
  {
    switch(marker.marker.type)
    {
      case visualization_msgs::msg::Marker::SPHERE:
      {
        QRectF bbox(marker.local_position, marker.local_position);
        path.addEllipse(bbox.marginsAdded(QMarginsF(marker.marker.scale.x/2.0 /pixel_size_, marker.marker.scale.y/2.0/pixel_size_, marker.marker.scale.x/2.0/pixel_size_, marker.marker.scale.y/2.0/pixel_size_)));
        break;
      }
      case visualization_msgs::msg::Marker::LINE_STRIP:
      {
        auto cosr = cos(-marker.rotation);
        auto sinr = sin(-marker.rotation);
        for(auto p1 = marker.marker.points.begin(); p1 != marker.marker.points.end(); ++p1)
        {
          auto x = p1->x*cosr + p1->y*sinr;
          auto y = p1->x*sinr - p1->y*cosr;
          x /= pixel_size_;
          y /= pixel_size_;
          if(p1 == marker.marker.points.begin())
            path.moveTo(marker.local_position.x()+x,marker.local_position.y()+y);
          else
            path.lineTo(marker.local_position.x()+x, marker.local_position.y()+y);
        }
        break;
      }
      case visualization_msgs::msg::Marker::TEXT_VIEW_FACING:
      {
        {
          QFont font;
          int font_size = std::max(5, int(marker.marker.scale.z*bg->scaledPixelSize()*10));
          font.setPixelSize(font_size);
          QFontMetrics metrics(font);
          auto bounds = metrics.boundingRect(marker.marker.text.c_str());
          path.addText(QPointF(marker.local_position.x()-bounds.width()*pixel_size_/2.0, marker.local_position.y()+bounds.height()*pixel_size_/2.0), font, marker.marker.text.c_str());

        }
        break;
      }
      default:
        RCLCPP_WARN_STREAM(node_->get_logger(), "marker type not handles: " << marker.marker.type);
    }
  }
  return path;
}

void Markers::setTopic(std::string topic, std::string type)
{
  if(node_)
  {
    if(type == "visualization_msgs/MarkerArray")
      marker_array_subscription_ = node_->create_subscription<visualization_msgs::msg::MarkerArray>(topic, 1, std::bind(&Markers::markerArrayCallback, this, std::placeholders::_1));
    if(type == "visualization_msgs/Marker")
      marker_subscription_ = node_->create_subscription<visualization_msgs::msg::Marker>(topic, 1, std::bind(&Markers::markerCallback, this, std::placeholders::_1));

    ui_.topicLabel->setText(topic.c_str());
  }

}

void Markers::setPixelSize(double s)
{
  pixel_size_ = s;
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
    auto marker_data = std::make_shared<MarkerData>();
    marker_data->marker = m;
    try
    {
      if(m.action == visualization_msgs::msg::Marker::ADD)
      {
        auto now = node_->get_clock()->now();
        if(rclcpp::Duration(m.lifetime).nanoseconds() != 0 && rclcpp::Time(m.header.stamp)+rclcpp::Duration(m.lifetime) < now)
        {
          rclcpp::Clock clock;
          RCLCPP_DEBUG_STREAM_THROTTLE(node_->get_logger(), clock, 5000, "Expired marker: "<< m.ns << " id: " << m.id);
          continue;
        }
        if(m.header.frame_id.empty())
        {
          rclcpp::Clock clock;
          RCLCPP_DEBUG_STREAM_THROTTLE(node_->get_logger(), clock, 1000, "Missing frame_id in marker: "<< m.ns << " id: " << m.id);
          continue;;
        }
        marker_data->position = getGeoCoordinate(m.pose, m.header);
        marker_data->rotation = tf2::getYaw(m.pose.orientation);
      }
      std::lock_guard<std::mutex> lock(new_markers_mutex_);
      new_markers_.push_back(marker_data);
    }
    catch (tf2::TransformException &ex)
    {
      rclcpp::Clock clock;
      RCLCPP_WARN_STREAM_THROTTLE(node_->get_logger(), clock, 1000, "Unable to find transform to earth for marker: " << m.ns << " id: " << m.id << " what: " << ex.what());
    }
  }
  emit newMarkersMadeAvailable();
}

QGeoCoordinate Markers::getGeoCoordinate(const geometry_msgs::msg::Pose &pose, const std_msgs::msg::Header &header)
{
  // if(header.frame_id.empty())
  // {
  //   ROS_WARN_STREAM_THROTTLE(1.0, "Markers: Missing frame_id: " << header);
  //   return {};
  // }
  // try
  // {
    geometry_msgs::msg::PoseStamped ps;
    ps.header = header;
    ps.pose = pose;
    auto ecef = transform_buffer_->transform(ps, "earth", tf2::durationFromSec(1.5));

    gz4d::GeoPointECEF ecef_point;
    ecef_point[0] = ecef.pose.position.x;
    ecef_point[1] = ecef.pose.position.y;
    ecef_point[2] = ecef.pose.position.z;
    gz4d::GeoPointLatLongDegrees ll = ecef_point;
    return QGeoCoordinate(ll.latitude(), ll.longitude(), ll.altitude());
  // }
  // catch (tf2::TransformException &ex)
  // {
  //   ROS_WARN_STREAM_THROTTLE(2.0, "Unable to find transform to earth for marker: " << ex.what() << " lookup time: " << header.stamp << " now: " << ros::Time::now() << " source frame: " << header.frame_id);
  // }
  // return {};
}


void Markers::newMarkersAvailable()
{
  prepareGeometryChange();

  std::vector<std::shared_ptr<MarkerData> > new_markers;
  {
    std::lock_guard<std::mutex> lock(new_markers_mutex_);
    new_markers = new_markers_;
    new_markers_.clear();
  }

  auto bg = findParentBackgroundRaster();
  for(auto marker: new_markers)
  {
    switch(marker->marker.action)
    {
      case visualization_msgs::msg::Marker::ADD:
        if(bg)
          marker->local_position = geoToPixel(marker->position, bg);
        //ROS_INFO_STREAM(marker->marker.ns << ": " << marker->marker.id << " local pos: " << marker->local_position.x() << ", " << marker->local_position.y());
        current_markers_[marker->marker.ns][marker->marker.id] = marker;
        if(rclcpp::Duration(marker->marker.lifetime).seconds() != 0)
          QTimer::singleShot((rclcpp::Duration(marker->marker.lifetime).seconds()+1.0)*1000, this, &Markers::newMarkersAvailable);
        break;
      case visualization_msgs::msg::Marker::DELETE:
        current_markers_[marker->marker.ns][marker->marker.id].reset();
        break;
      case visualization_msgs::msg::Marker::DELETEALL:
        current_markers_[marker->marker.ns].clear();
        break;
      default:
        RCLCPP_WARN_STREAM(node_->get_logger(), "Unknown marker action: " << marker->marker.action);
    }
  }

  auto now = node_->get_clock()->now();

  for(auto& ns: current_markers_)
  {
    std::vector<int32_t> expired;
    for(auto m: ns.second)
      if(rclcpp::Time(m.second->marker.header.stamp).seconds() != 0.0 && rclcpp::Duration(m.second->marker.lifetime).seconds() != 0 && rclcpp::Time(m.second->marker.header.stamp) + rclcpp::Duration(m.second->marker.lifetime) < now)
        expired.push_back(m.first);
    for(auto e: expired)
    {
      RCLCPP_DEBUG_STREAM(node_->get_logger(), "Purging " << ns.first << ": " << e);
      ns.second.erase(e);

    }
  }

  GeoGraphicsItem::update();
}

void Markers::visibilityChanged()
{
  prepareGeometryChange();
  is_visible_ = ui_.displayCheckBox->isChecked();
  GeoGraphicsItem::update();
}


void Markers::updateBackground(BackgroundRaster * bg)
{
  prepareGeometryChange();
  setParentItem(bg);
  if(bg)
  {
    setPixelSize(bg->pixelSize());
  }
  GeoGraphicsItem::update();
}
