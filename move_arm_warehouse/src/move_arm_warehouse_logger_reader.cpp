/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
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
 *
 *  \author E. Gil Jones
 *********************************************************************/

#include <ros/console.h>
#include <boost/foreach.hpp>
#include <sstream>

#include <move_arm_warehouse/move_arm_warehouse_logger_reader.h>

using namespace move_arm_warehouse;

static const std::string DATABASE_NAME="arm_navigation";
static const std::string PLANNING_SCENE_TIME_NAME="planning_scene_time";
static const std::string PAUSED_COLLISION_MAP_TIME_NAME="paused_collision_map_time";

typedef mongo_ros::MessageWithMetadata<planning_environment_msgs::PlanningScene>::ConstPtr PlanningSceneWithMetadata;
typedef mongo_ros::MessageWithMetadata<motion_planning_msgs::MotionPlanRequest>::ConstPtr MotionPlanRequestWithMetadata;
typedef mongo_ros::MessageWithMetadata<trajectory_msgs::JointTrajectory>::ConstPtr JointTrajectoryWithMetadata;
typedef mongo_ros::MessageWithMetadata<motion_planning_msgs::ArmNavigationErrorCodes>::ConstPtr ErrorCodesWithMetadata;
typedef mongo_ros::MessageWithMetadata<head_monitor_msgs::HeadMonitorFeedback>::ConstPtr HeadMonitorFeedbackWithMetadata;
  
MoveArmWarehouseLoggerReader::MoveArmWarehouseLoggerReader()
{
  char hostname[256];
  
  gethostname(hostname, 256);

  hostname_ = hostname;

  ROS_INFO_STREAM("Hostname is " << hostname_);

  std::vector<std::string> indexed_fields;
  indexed_fields.push_back(PLANNING_SCENE_TIME_NAME);
  planning_scene_collection_ = new mongo_ros::MessageCollection<planning_environment_msgs::PlanningScene>(DATABASE_NAME, "planning_scene", indexed_fields);

  indexed_fields.clear();
  indexed_fields.push_back("group_name");
  motion_plan_request_collection_ = new mongo_ros::MessageCollection<motion_planning_msgs::MotionPlanRequest>(DATABASE_NAME, "motion_plan_request", indexed_fields);

  indexed_fields.clear();
  trajectory_collection_ = new mongo_ros::MessageCollection<trajectory_msgs::JointTrajectory>(DATABASE_NAME, "trajectory", indexed_fields);

  indexed_fields.clear();
  indexed_fields.push_back("val");
  outcome_collection_ = new mongo_ros::MessageCollection<motion_planning_msgs::ArmNavigationErrorCodes>(DATABASE_NAME, "outcome", indexed_fields);

  indexed_fields.clear();
  indexed_fields.push_back("paused_collision_map_stamp");
  paused_state_collection_ = new mongo_ros::MessageCollection<head_monitor_msgs::HeadMonitorFeedback>(DATABASE_NAME, "paused_state", indexed_fields);
}

MoveArmWarehouseLoggerReader::~MoveArmWarehouseLoggerReader() {
  delete planning_scene_collection_;
  delete motion_plan_request_collection_;
  delete trajectory_collection_;
  delete outcome_collection_;
  delete paused_state_collection_;
}

///
/// LOGGING FUNCTIONS
///

mongo_ros::Metadata MoveArmWarehouseLoggerReader::initializeMetadataWithHostname()
{
  return mongo_ros::Metadata("hostname", hostname_);
}

void MoveArmWarehouseLoggerReader::addPlanningSceneTimeToMetadata(const planning_environment_msgs::PlanningScene& planning_scene,
                                                            mongo_ros::Metadata& metadata)
{
  metadata.append(PLANNING_SCENE_TIME_NAME, planning_scene.robot_state.joint_state.header.stamp.toSec());
}

void MoveArmWarehouseLoggerReader::pushPlanningSceneToWarehouse(const planning_environment_msgs::PlanningScene planning_scene)
{
  mongo_ros::Metadata metadata = initializeMetadataWithHostname();
  addPlanningSceneTimeToMetadata(planning_scene, metadata);
  planning_scene_collection_->insert(planning_scene, metadata);
}

void MoveArmWarehouseLoggerReader::pushMotionPlanRequestToWarehouse(const planning_environment_msgs::PlanningScene& planning_scene,
                                                              const std::string& stage_name,
                                                              const motion_planning_msgs::MotionPlanRequest& motion_plan_request)
{
  mongo_ros::Metadata metadata = initializeMetadataWithHostname();
  addPlanningSceneTimeToMetadata(planning_scene, metadata);

  metadata.append("stage_name", stage_name);
  
  //adding the presence of goal pose constraints to metadata
  metadata.append("has_goal_position_constraints", !motion_plan_request.goal_constraints.position_constraints.empty());

 
  metadata.append("has_path_constraints", 
                  (!motion_plan_request.path_constraints.orientation_constraints.empty() || motion_plan_request.path_constraints.position_constraints.empty()));
  
  motion_plan_request_collection_->insert(motion_plan_request, metadata);
}

void MoveArmWarehouseLoggerReader::pushJointTrajectoryToWarehouse(const planning_environment_msgs::PlanningScene& planning_scene,
                                                            const std::string& trajectory_source,
                                                            const ros::Duration& production_time,
                                                            const trajectory_msgs::JointTrajectory& trajectory)
{
  mongo_ros::Metadata metadata = initializeMetadataWithHostname();
  addPlanningSceneTimeToMetadata(planning_scene, metadata);

  metadata.append("trajectory_source", trajectory_source);
  metadata.append("production_time", production_time.toSec());
 
  trajectory_collection_->insert(trajectory, metadata);
}

void MoveArmWarehouseLoggerReader::pushOutcomeToWarehouse(const planning_environment_msgs::PlanningScene& planning_scene,
                                                    const std::string& pipeline_stage,
                                                    const motion_planning_msgs::ArmNavigationErrorCodes& error_codes)
{
  mongo_ros::Metadata metadata = initializeMetadataWithHostname();
  addPlanningSceneTimeToMetadata(planning_scene, metadata);

  metadata.append("pipeline_stage", pipeline_stage);
  outcome_collection_->insert(error_codes, metadata);
}

void MoveArmWarehouseLoggerReader::pushPausedStateToWarehouse(const planning_environment_msgs::PlanningScene& planning_scene,
                                                              const head_monitor_msgs::HeadMonitorFeedback& feedback)
{
  mongo_ros::Metadata metadata = initializeMetadataWithHostname();
  addPlanningSceneTimeToMetadata(planning_scene, metadata);
  metadata.append(PAUSED_COLLISION_MAP_TIME_NAME, feedback.paused_collision_map.header.stamp.toSec());

  paused_state_collection_->insert(feedback, metadata);

}

///
/// READING FUNCTIONS
///

void MoveArmWarehouseLoggerReader::getAvailablePlanningSceneList(const std::string& hostname, std::vector<ros::Time>& creation_times)
{
  creation_times.clear();

  // std::stringstream fin(planning_scenes[i]->metadata);
  // YAML::Parser parser(fin);
  // YAML::Node doc;
  // while(parser.GetNextDocument(doc)) {    }

  mongo_ros::Query q;
  std::vector<PlanningSceneWithMetadata> planning_scenes = planning_scene_collection_->pullAllResults(q, true, "creation_time", true);

  creation_times.resize(planning_scenes.size());

  for(unsigned int i = 0; i < planning_scenes.size(); i++) {
    creation_times[i] = ros::Time(planning_scenes[i]->lookupDouble(PLANNING_SCENE_TIME_NAME));
  }
}

mongo_ros::Query MoveArmWarehouseLoggerReader::makeQueryForPlanningSceneTime(const ros::Time& time)
{
  mongo_ros::Query retq;
  retq.append(PLANNING_SCENE_TIME_NAME, time.toSec()); 
  return retq;
}

bool MoveArmWarehouseLoggerReader::getPlanningScene(const std::string& hostname, const ros::Time& time, 
                                                    planning_environment_msgs::PlanningScene& planning_scene)
{
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  std::vector<PlanningSceneWithMetadata> planning_scenes = planning_scene_collection_->pullAllResults(q, false);

  if(planning_scenes.size() == 0) {
    ROS_WARN_STREAM("No scenes with that time");
    return false;
  } else if(planning_scenes.size() > 1) {
    ROS_WARN_STREAM("More than one stream with that time " << planning_scenes.size());
  }
  planning_scene = *planning_scenes[0];
  return true;
}

bool MoveArmWarehouseLoggerReader::getAssociatedOutcomes(const std::string& hostname,
                                                   const ros::Time& time,
                                                   std::vector<std::string>& pipeline_names,
                                                   std::vector<motion_planning_msgs::ArmNavigationErrorCodes>& error_codes)
{
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  std::vector<ErrorCodesWithMetadata> meta_error_codes = outcome_collection_->pullAllResults(q, false);
  
  if(meta_error_codes.size() == 0) {
    ROS_WARN_STREAM("No outcomes associated with time " << time);
    return false;
  } 
  error_codes.resize(meta_error_codes.size());
  pipeline_names.resize(meta_error_codes.size());

  for(unsigned int i = 0; i < meta_error_codes.size(); i++) {
    pipeline_names[i] = meta_error_codes[i]->lookupString("pipeline_stage");
    error_codes[i] = *meta_error_codes[i];
  }
  return true;
}

                                                 
bool MoveArmWarehouseLoggerReader::getAssociatedMotionPlanRequestsStageNames(const std::string& hostname, 
                                                                       const ros::Time& time,
                                                                       std::vector<std::string>& stage_names)
{
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  std::vector<MotionPlanRequestWithMetadata> motion_plan_requests = motion_plan_request_collection_->pullAllResults(q, true, "creation_time", true);

  if(motion_plan_requests.size() == 0) {
    ROS_WARN_STREAM("No motion plan requests with that time");
    return false;
  } 
  stage_names.resize(motion_plan_requests.size());
  for(unsigned int i = 0; i < motion_plan_requests.size(); i++) {
    stage_names[i] = motion_plan_requests[i]->lookupString("stage_name");
  }
  return true; 
}

bool MoveArmWarehouseLoggerReader::getAssociatedMotionPlanRequest(const std::string& hostname, 
                                                            const ros::Time& time,
                                                            const std::string& stage_name,
                                                            motion_planning_msgs::MotionPlanRequest& request)
{  
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  q.append("stage_name", stage_name);
  std::vector<MotionPlanRequestWithMetadata> motion_plan_requests = motion_plan_request_collection_->pullAllResults(q, false);

  if(motion_plan_requests.size() == 0) {
    ROS_WARN_STREAM("No motion plan requests with that time and stage name " << stage_name);
    return false;
  } else if(motion_plan_requests.size() > 1) {
    ROS_WARN_STREAM("More than one motion plan requests with that time and stage name " << stage_name);
    return false;
  }
  request = *motion_plan_requests[0];
  return true;
}

bool MoveArmWarehouseLoggerReader::getAssociatedJointTrajectorySources(const std::string& hostname, 
                                                                 const ros::Time& time,
                                                                 std::vector<std::string>& trajectory_sources)
{
  trajectory_sources.clear();
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  std::vector<JointTrajectoryWithMetadata> joint_trajectories = trajectory_collection_->pullAllResults(q, true, "creation_time");

  if(joint_trajectories.size() == 0) {
    ROS_WARN_STREAM("No joint trajectories with that time");
    return false;
  } 
  trajectory_sources.resize(joint_trajectories.size());
  for(unsigned int i = 0; i < joint_trajectories.size(); i++) {
    trajectory_sources[i] = joint_trajectories[i]->lookupString("trajectory_source");
  }
  return true; 
}

bool MoveArmWarehouseLoggerReader::getAssociatedJointTrajectory(const std::string& hostname, 
                                                          const ros::Time& time,
                                                          const std::string& trajectory_source,
                                                          const unsigned int& trajectory_index,
                                                          ros::Duration& duration, 
                                                          trajectory_msgs::JointTrajectory& joint_trajectory)
{
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  q.append("trajectory_source", trajectory_source);
  std::vector<JointTrajectoryWithMetadata> joint_trajectories = trajectory_collection_->pullAllResults(q, false);

  if(joint_trajectories.size() == 0) {
    ROS_WARN_STREAM("No joint trajectories with that time and source name " << trajectory_source);
    return false;
  } else if(joint_trajectories.size() <= trajectory_index) {
    ROS_WARN_STREAM("Not enough trajectories for that index: " << trajectory_index);
    return false;
  }

  duration = ros::Duration(joint_trajectories[trajectory_index]->lookupDouble("production_time"));
  joint_trajectory = *joint_trajectories[trajectory_index];
  return true;
}

bool MoveArmWarehouseLoggerReader::getAssociatedPausedStates(const std::string& hostname, 
                                                             const ros::Time& time,
                                                             std::vector<ros::Time>& paused_times)
{
  paused_times.clear();
  mongo_ros::Query q = makeQueryForPlanningSceneTime(time);  
  std::vector<HeadMonitorFeedbackWithMetadata> paused_states = paused_state_collection_->pullAllResults(q, true, "creation_time");

  if(paused_states.size() == 0) {
    return false;
  } 
  paused_times.resize(paused_states.size());
  for(unsigned int i = 0; i < paused_states.size(); i++) {
    paused_times[i] = ros::Time(paused_states[i]->lookupDouble("paused_collision_map_stamp"));
  }
  return true;   
}

bool MoveArmWarehouseLoggerReader::getAssociatedPausedState(const std::string& hostname, 
                                                      const ros::Time& planning_time, 
                                                      const ros::Time& paused_time,
                                                      head_monitor_msgs::HeadMonitorFeedback& paused_state)
{
  mongo_ros::Query q = makeQueryForPlanningSceneTime(planning_time);  
  q.append(PAUSED_COLLISION_MAP_TIME_NAME, paused_time.toSec());

  std::vector<HeadMonitorFeedbackWithMetadata> paused_states = paused_state_collection_->pullAllResults(q, false);

  if(paused_states.size() == 0) {
    ROS_WARN_STREAM("No paused states with that time");
    return false;
  } else if(paused_states.size() > 1) {
    ROS_WARN_STREAM("Multiple paused states with time");
    return false;
  }
  paused_state = *paused_states[0];
  return true;
}