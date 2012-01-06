/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2011, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/** \author E. Gil Jones */

#ifndef _TRAJECTORY_CONTROLLER_HANDLER_H_
#define _TRAJECTORY_CONTROLLER_HANDLER_H_

#include <ros/ros.h>
#include <boost/function.hpp>

#include <trajectory_msgs/JointTrajectory.h>

#include <trajectory_execution_monitor/trajectory_recorder.h>

namespace trajectory_execution_monitor
{

typedef boost::function<void(bool)> TrajectoryFinishedCallbackFunction;

class TrajectoryControllerHandler {
protected:
  // TrajectoryControllerHandler is augmented to record overshoot of a trajectory
  enum TrajectoryControllerState	{	IDLE, PAUSED, EXECUTING, OVERSHOOTING };

public:

  TrajectoryControllerHandler(const std::string& group_name,
                              const std::string& controller_name) :
    group_name_(group_name),
    controller_name_(controller_name),
    monitor_overshoot_(false),
    controller_state_(IDLE)
  {
    group_controller_combo_name_ = combineGroupAndControllerNames(group_name,controller_name);  
  };

  virtual ~TrajectoryControllerHandler() {

  }

  static std::string combineGroupAndControllerNames(const std::string& group_name,
                                                    const std::string& controller_name) {
    return(group_name+"_"+controller_name);
  }

  // Call this function if the trajectory should monitor overshoot after the trajectory is executed.
  // returns true if the subclass can monitor overshoot
  virtual bool enableOvershoot(
                        double max_overshoot_velocity_epsilon,
                        ros::Duration min_overshoot_time,
                        ros::Duration max_overshoot_time);

  // Disable overshoot monitoring
  virtual void disableOvershoot();

  // Call enableOvershoot function if overshoot should get monitored.  But default this does not happen.
  virtual bool executeTrajectory(const trajectory_msgs::JointTrajectory& trajectory,
                                 boost::shared_ptr<TrajectoryRecorder>& recorder,
                                 const TrajectoryFinishedCallbackFunction& traj_callback) = 0;

  virtual void cancelExecution() = 0;

  const trajectory_msgs::JointTrajectory& getLastGoalTrajectory() const {
    return goal_trajectory_;
  }

  const trajectory_msgs::JointTrajectory& getLastRecordedTrajectory() const {
    return recorded_trajectory_;
  }

  const trajectory_msgs::JointTrajectory& getLastOvershootTrajectory() const {
    return overshoot_trajectory_;
  }

  const std::string& getGroupName() const {
    return group_name_;
  }

  const std::string& getControllerName() const {
    return controller_name_;
  }

  const std::string& getGroupControllerComboName() const {
    return group_controller_combo_name_;
  }

protected:

  bool addNewStateToRecordedTrajectory(const ros::Time& time,
                                       const std::map<std::string, double>& joint_positions,
                                       const std::map<std::string, double>& joint_velocities);
  bool _addNewStateToTrajectory(const ros::Time& time,
                                const std::map<std::string, double>& joint_positions,
                                const std::map<std::string, double>& joint_velocities,
                                trajectory_msgs::JointTrajectory& trajectory);

  void initializeRecordedTrajectory(const trajectory_msgs::JointTrajectory& goal_trajectory);
  void initializeOvershootTrajectory();
  // returns the index from min_overshoot_time_ ago.
  unsigned int findClosestIndex( ros::Duration time_from_start_ );

  std::string group_name_;
  std::string controller_name_;
  std::string group_controller_combo_name_;

  trajectory_msgs::JointTrajectory recorded_trajectory_;
  trajectory_msgs::JointTrajectory overshoot_trajectory_;
  trajectory_msgs::JointTrajectory goal_trajectory_;

  bool monitor_overshoot_;
  double max_overshoot_velocity_epsilon_;
  ros::Duration min_overshoot_time_;
  ros::Duration max_overshoot_time_;

  boost::shared_ptr<trajectory_execution_monitor::TrajectoryRecorder> recorder_;
  trajectory_execution_monitor::TrajectoryFinishedCallbackFunction trajectory_finished_callback_;
  TrajectoryControllerState	controller_state_;
  bool success_;

  //TODO - consider pause and resume execution

};

}

#endif
