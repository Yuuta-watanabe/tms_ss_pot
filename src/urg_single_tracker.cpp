
#include <ros/ros.h>
#include <pthread.h>
#include <fstream>
#include <std_msgs/String.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/MultiEchoLaserScan.h>
#include <urg_c/urg_sensor.h>
#include <urg_c/urg_utils.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <tms_msg_ss/tracking_points.h>
#include <tms_msg_ss/tracking_grid.h>

pthread_mutex_t mutex_laser = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_target = PTHREAD_MUTEX_INITIALIZER;

#include "opencv/cv.h"
#include "define.h"
#include "target.h"
#include "laser.h"
#include "particle_filter.h"
#include "multiple_particle_filter.h"

// LASER_PARAMETOR----------------------------
std::vector< float > scanData1;
CLaser laser;
bool CallbackCalled1 = false;
int nStep1 = 70;
double StartAngle1 = -90.0;
double DivAngle1 = 0.35;
//--------------------------------------------

void *Processing(void *ptr)
{
  CMultipleParticleFilter m_PF;
  ros::Rate r(40);
  ros::Publisher *pub = (ros::Publisher *)ptr;
  uint32_t shape_circle = visualization_msgs::Marker::CYLINDER;
  visualization_msgs::MarkerArray markerArray;

  laser.Init();
  /**********************************/
  laser.m_bNodeActive[0] = true;
  laser.m_nConnectNum = 1;
  laser.GetLRFParam();

  if (laser.m_bNodeActive[0])
    while (!CallbackCalled1)
      r.sleep();

  laser.m_nStep[0] = nStep1;
  laser.m_StartAngle[0] = StartAngle1;
  laser.m_DivAngle[0] = DivAngle1;
  /**********************************/

  CvMat *m_Rotate = cvCreateMat(2, 2, CV_64F);
  CvMat *m_Translate = cvCreateMat(2, 1, CV_64F);
  CvMat *Temp = cvCreateMat(2, 1, CV_64F);
  int count;
  double theta, range;
  int UPDATE = Config::is()->update;

  if(laser.ReadBackgroundData()) laser.m_bResetBackRangeData == false;

  if (laser.m_bResetBackRangeData == true)
  {
    for (int it = 0; it < laser.m_ring; it++)
    {
      for (int n = 0; n < laser.m_cnMaxConnect; n++)
      {
        if (laser.m_bNodeActive[n])
        {
          pthread_mutex_lock(&mutex_laser);

          if (n == 0)
          {
            laser.m_LRFData[n].clear();
            laser.m_LRFData[n].resize(scanData1.size());
            for (int i = 0; i < scanData1.size(); i++)
              laser.m_LRFData[n][i] = scanData1[i];
          }
          pthread_mutex_unlock(&mutex_laser);
        }
      }
      //laser.GetBackLRFDataGaussian();
      r.sleep();
    }
    laser.m_bResetBackRangeData = false;

    //laser.WriteBackgroundData();

    //std::cout << "Back range data is stored" << std::endl;
  }

  int iteration = 0;
  while (ros::ok())
  {
    tms_msg_ss::tracking_grid grid;
    tms_msg_ss::tracking_points points;
    if (laser.m_bResetBackRangeData == false)
    {
      for (int n = 0; n < laser.m_cnMaxConnect; n++)
      {
        if (laser.m_bNodeActive[n])
        {
          pthread_mutex_lock(&mutex_laser);

          if (n == 0)
          {
            laser.m_LRFData[n].clear();
            laser.m_LRFData[n].resize(scanData1.size());
            for (int i = 0; i < scanData1.size(); i++)
            {
              laser.m_LRFData[n][i] = scanData1[i];
            }
          }

          laser.GetDiffLRFCluster(n);

          cvmSet(m_Rotate, 0, 0, cos(deg2rad(laser.m_LRFParam[n].rz)));
          cvmSet(m_Rotate, 0, 1, -sin(deg2rad(laser.m_LRFParam[n].rz)));
          cvmSet(m_Rotate, 1, 0, sin(deg2rad(laser.m_LRFParam[n].rz)));
          cvmSet(m_Rotate, 1, 1, cos(deg2rad(laser.m_LRFParam[n].rz)));
          cvmSet(m_Translate, 0, 0, laser.m_LRFParam[n].tx);
          cvmSet(m_Translate, 1, 0, laser.m_LRFParam[n].ty);

          // Number of clusters
          std::cout << "Number of clusters " << laser.m_LRFClsData[n].size() << std::endl;

          laser.m_LRFClsPoints[n].clear();
          laser.m_LRFClsPoints[n].resize(laser.m_LRFClsData[n].size());
          int id = 0;
          for (int i = 0; i < laser.m_LRFClsData[n].size(); i++)
          {
            count = laser.m_LRFClsData[n][i].n;
            range = laser.m_LRFClsData[n][i].range;
            theta = laser.m_DivAngle[n] * count + laser.m_StartAngle[n];

            cvmSet(laser.m_LRFClsPos[n][i], 0, 0, range * cos(deg2rad(theta)));
            cvmSet(laser.m_LRFClsPos[n][i], 1, 0, range * sin(deg2rad(theta)));
            cvMatMul(m_Rotate, laser.m_LRFClsPos[n][i], Temp);
            cvAdd(m_Translate, Temp, laser.m_LRFClsPos[n][i]);
            laser.m_LRFClsPoints[n][i].x = cvmGet(laser.m_LRFClsPos[n][i], 0, 0);
            laser.m_LRFClsPoints[n][i].y = cvmGet(laser.m_LRFClsPos[n][i], 1, 0);

            grid.id = id;
            grid.x = laser.m_LRFClsPoints[n][i].x;;
            grid.y = laser.m_LRFClsPoints[n][i].y;;
            points.tracking_grid.push_back(grid);
            id++;
          }
          pub->publish(points);

          pthread_mutex_unlock(&mutex_laser);
        }
      }
    }

    //system("clear");
    r.sleep();
  }

  cvReleaseMat(&Temp);
  cvReleaseMat(&m_Rotate);
  cvReleaseMat(&m_Translate);
}

void LaserSensingCallback1(const sensor_msgs::LaserScan::ConstPtr &scan)
{
  pthread_mutex_lock(&mutex_laser);
  int num = floor((scan->angle_max - scan->angle_min) / scan->angle_increment) + 1;
  if (CallbackCalled1 == false)
  {
    nStep1 = num;
    StartAngle1 = rad2deg(scan->angle_min);
    DivAngle1 = rad2deg(scan->angle_increment);
    std::cout << "nStep1 " << nStep1 << " StartAngle1 " << StartAngle1 << " DivAngle1 " << DivAngle1 << std::endl;
  }
  if (scanData1.size() == 0)
    scanData1.resize(num);
  scanData1 = scan->ranges;
  pthread_mutex_unlock(&mutex_laser);
  CallbackCalled1 = true;
}

int main(int argc, char **argv)
{
  Config::is();

  pthread_t thread_p;
  pthread_t thread_v;
  ros::MultiThreadedSpinner spinner(4);

  ros::init(argc, argv, "urg_tracker");
  ros::NodeHandle n;
  ros::Publisher pub = n.advertise< tms_msg_ss::tracking_points >("tracking_points3", 10);
  ros::Subscriber sub = n.subscribe("/hsrb/base_scan", 50, LaserSensingCallback1);

  if (pthread_create(&thread_p, NULL, Processing, (void *)&pub))
  {
    printf("error creating thread.");
    abort();
  }

  spinner.spin();  // spin() will not return until the node has been shutdown

  ros::waitForShutdown();

  if (pthread_join(thread_p, NULL))
  {
    printf("error joining thread.");
    abort();
  }
  scanData1.clear();

  return 0;
}
