/*********************************************************************
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
*********************************************************************/

/** \author Ioan Sucan */

#ifndef PLANNING_ENVIRONMENT_MODELS_ROBOT_MODELS_
#define PLANNING_ENVIRONMENT_MODELS_ROBOT_MODELS_

#include <planning_models/kinematic_model.h>
#include <ros/ros.h>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>
#include <vector>

namespace planning_environment
{
    
/** \brief A class capable of loading a robot model from the parameter server */
    
class RobotModels
{
public:
	
  struct GroupConfig
  {
    GroupConfig(std::string name) : name_(name){
    }

    ~GroupConfig() {
    }

    const std::vector<std::string>& getJointNames() {
      return joint_names_;
    }
    const std::vector<std::string>& getLinkNames() {
      return link_names_;
    }
    
    std::string name_;
    std::vector<std::string> joint_names_;
    std::vector<std::string> link_names_;
  };
	
  RobotModels(const std::string &description)
  {
    description_ = nh_.resolveName(description);
    loaded_models_ = false;
    loadRobot();
  }

  virtual ~RobotModels(void)
  {
    for(std::map<std::string, GroupConfig*>::iterator it = group_config_map_.begin();
        it != group_config_map_.end();
        it++) {
      delete it->second;
    }
  }
       
  /** \brief Return the name of the description */
  const std::string &getDescription(void) const
  {
    return description_;
  }
	
  /** \brief Return the instance of the constructed kinematic model */
  const boost::shared_ptr<planning_models::KinematicModel> &getKinematicModel(void) const
  {
    return kmodel_;
  }

  /** \brief Return the instance of the parsed robot description */
  const boost::shared_ptr<urdf::Model> &getParsedDescription(void) const
  {
    return urdf_;
  }

  /** \brief Return the map of the planning group joints */
  const std::map< std::string, std::vector<std::string> > &getPlanningGroupJoints(void) const
  {
    return planning_group_joints_;
  }	

  /** \brief Return the map of the planning group links */
  const std::map<std::string, std::vector<std::string> > &getPlanningGroupLinks(void) const {
    return planning_group_links_;
  }

  /** \brief Gets the union of all the joints in all planning groups */
  const std::vector<std::string>& getGroupJointUnion(void) const {
    return group_joint_union_;
  }

  /** \brief Gets the union of all the links in all planning groups */
  const std::vector<std::string>& getGroupLinkUnion(void) const {
    return group_link_union_;
  }
	
  /** \brief Return true if models have been loaded */
  bool loadedModels(void) const
  {
    return loaded_models_;
  }
	
  /** \brief Reload the robot description and recreate the model */
  virtual void reload(void);
	
protected:
	
  void loadRobot(void);

  void readGroupConfigs();
	
  bool readMultiDofConfigs();
	
  ros::NodeHandle                                    nh_;
	  
  std::string                                        description_;
	
  bool                                               loaded_models_;
  boost::shared_ptr<planning_models::KinematicModel> kmodel_;
  
  boost::shared_ptr<urdf::Model>                     urdf_;
	
  std::map< std::string, std::vector<std::string> >  planning_group_links_;
  std::map< std::string, std::vector<std::string> >  planning_group_joints_;
  std::vector<std::string> group_joint_union_;
  std::vector<std::string> group_link_union_;
  std::map< std::string, GroupConfig*> group_config_map_;
  std::vector<planning_models::KinematicModel::MultiDofConfig> multi_dof_configs_;
  
  
};
    
}

#endif

