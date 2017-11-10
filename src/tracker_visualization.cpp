
#include "ros/ros.h"
#include "stdio.h"
#include "tms_msg_ss/tracking_points.h"
#include "tms_msg_ss/tracking_grid.h"
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include "iostream"
#include "math.h"

ros::Publisher pub;
ros::Subscriber sub;

// colorset
  uint32_t shape_circle = visualization_msgs::Marker::CYLINDER;
float colorset[14][4] = {{1, 0, 0, 0},
                         {0, 1, 0, 0},
                         {0, 0, 1, 0},
                         {1, 1, 0, 0},
                         {0, 1, 1, 0},
                         {1, 0, 1, 0},
                         {1, 1, 1, 0},
                         {0.5, 0, 0, 0},
                         {0, 0.5, 0, 0},
                         {0, 0, 0.5, 0},
                         {0.5, 0.5, 0, 0},
                         {0, 0.5, 0.5, 0},
                         {0.5, 0, 0.5, 0},
                         {0.5, 0.5, 0.5, 0}};

void visualization_callback(const tms_msg_ss::tracking_points::ConstPtr &msg)
{
  visualization_msgs::MarkerArray markerArray;
  for(int i=0;i < msg->tracking_grid.size();i++)
  {
    visualization_msgs::Marker marker_circle;
    marker_circle.header.frame_id = "map";
    marker_circle.header.stamp = ros::Time::now();

    marker_circle.id = msg->tracking_grid[i].id;
    marker_circle.type = shape_circle;
    marker_circle.action = visualization_msgs::Marker::ADD;

    marker_circle.pose.position.x = msg->tracking_grid[i].x;
    marker_circle.pose.position.y = msg->tracking_grid[i].y;
    marker_circle.pose.position.z = 0.5;
    marker_circle.pose.orientation.x = 0.0;
    marker_circle.pose.orientation.y = 0.0;
    marker_circle.pose.orientation.z = 0.0;
    marker_circle.pose.orientation.w = 1.0;

    marker_circle.scale.x = 0.1;
    marker_circle.scale.y = 0.1;
    marker_circle.scale.z = 1.0;

    int color = msg->tracking_grid[i].id % 14;
    marker_circle.color.r = colorset[color][0];
    marker_circle.color.g = colorset[color][1];
    marker_circle.color.b = colorset[color][2];
    marker_circle.color.a = 1.0;

    marker_circle.lifetime = ros::Duration(0.1);
    markerArray.markers.push_back(marker_circle);
   }
   pub.publish(markerArray);
}

// main function
int main(int argc, char **argv)
{
  ros::init(argc, argv, "portable_visualization");
  ros::NodeHandle n;
  pub = n.advertise< visualization_msgs::MarkerArray >("visualization_marker_array", 1);
  sub = n.subscribe("/tracking_points3", 10, visualization_callback);
  ros::spin();
  return 0;
}
