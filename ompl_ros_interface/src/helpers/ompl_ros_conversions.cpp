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
*   * Neither the name of Willow Garage, Inc. nor the names of its
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

/** \author Sachin Chitta, Ioan Sucan */

#include <ompl_ros_interface/helpers/ompl_ros_conversions.h>

namespace ompl_ros_interface
{
ompl::base::StateManifoldPtr jointGroupToOmplStateManifoldPtr(const planning_models::KinematicModel::JointModelGroup *joint_group, 
                                                              ompl_ros_interface::OmplStateToKinematicStateMapping &ompl_kinematic_mapping,
                                                              ompl_ros_interface::KinematicStateToOmplStateMapping &kinematic_ompl_mapping)
{
  ompl::base::StateManifoldPtr ompl_state_manifold;
  ompl_state_manifold.reset(dynamic_cast<ompl::base::StateManifold*>(new ompl::base::CompoundStateManifold()));
  ompl::base::CompoundStateManifold* state_manifold = dynamic_cast<ompl::base::CompoundStateManifold*> (ompl_state_manifold.get());

	ompl::base::RealVectorBounds real_vector_bounds(0); // initially assume dimension of R(K) submanifold is 0.
  ompl::base::RealVectorStateManifold real_vector_joints(0);

  std::vector<std::string> real_vector_names;
  std::vector<const planning_models::KinematicModel::JointModel*> joint_models = joint_group->getJointModels();

	for (unsigned int i = 0 ; i < joint_models.size() ; ++i)
  {
		const planning_models::KinematicModel::RevoluteJointModel* revolute_joint = 
			dynamic_cast<const planning_models::KinematicModel::RevoluteJointModel*>(joint_models[i]);
    if (revolute_joint && revolute_joint->continuous_)
    {
      ompl::base::SO2StateManifold *manifold = new ompl::base::SO2StateManifold();
      manifold->setName(revolute_joint->getName());
      state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);

      kinematic_ompl_mapping.joint_state_mapping.push_back(state_manifold->getSubManifoldCount()-1);
      kinematic_ompl_mapping.joint_mapping_type.push_back(ompl_ros_interface::SO2);

      ompl_kinematic_mapping.ompl_state_mapping.push_back(i);
      ompl_kinematic_mapping.mapping_type.push_back(ompl_ros_interface::SO2);
      ROS_DEBUG("Adding SO2 manifold with name %s",revolute_joint->getName().c_str());
    }
    else
    {
      const planning_models::KinematicModel::PlanarJointModel* planar_joint = 
        dynamic_cast<const planning_models::KinematicModel::PlanarJointModel*>(joint_models[i]);
      if (planar_joint)
      {
        ompl::base::SE2StateManifold *manifold = new ompl::base::SE2StateManifold();
        manifold->setName(planar_joint->getName());
        state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);

        kinematic_ompl_mapping.joint_state_mapping.push_back(state_manifold->getSubManifoldCount()-1);
        kinematic_ompl_mapping.joint_mapping_type.push_back(ompl_ros_interface::SE2);

        ompl_kinematic_mapping.ompl_state_mapping.push_back(i);
        ompl_kinematic_mapping.mapping_type.push_back(ompl_ros_interface::SE2);
      }
      else
      {
        const planning_models::KinematicModel::FloatingJointModel* floating_joint = 
          dynamic_cast<const planning_models::KinematicModel::FloatingJointModel*>(joint_models[i]);
        if (floating_joint)
        {
          ompl::base::SE3StateManifold *manifold = new ompl::base::SE3StateManifold();
          manifold->setName(floating_joint->getName());
          state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);

          kinematic_ompl_mapping.joint_state_mapping.push_back(state_manifold->getSubManifoldCount()-1);
          kinematic_ompl_mapping.joint_mapping_type.push_back(ompl_ros_interface::SE3);

          ompl_kinematic_mapping.ompl_state_mapping.push_back(i);
          ompl_kinematic_mapping.mapping_type.push_back(ompl_ros_interface::SE3);
        }
        else
        {
          // the only other case we consider is R^n; since we know that for now at least the only other type of joint available is non-continuous revolute joints
          // we can use the revoluteJoint cast      
          std::pair<double,double> bounds = revolute_joint->getVariableBounds(revolute_joint->getName());
          real_vector_bounds.low.push_back(bounds.first);
          real_vector_bounds.high.push_back(bounds.second);
          real_vector_names.push_back(revolute_joint->getName());
          kinematic_ompl_mapping.joint_state_mapping.push_back(real_vector_bounds.low.size()-1);
          kinematic_ompl_mapping.joint_mapping_type.push_back(ompl_ros_interface::REAL_VECTOR);
          ompl_kinematic_mapping.real_vector_mapping.push_back(i);
          ROS_DEBUG("Adding real vector joint %s with bounds %f %f",revolute_joint->getName().c_str(),real_vector_bounds.low.back(),real_vector_bounds.high.back());
        }
      }    
    }
  }
  // we added everything now, except the R(N) manifold. we now add R(N) as well, if needed
  if (real_vector_bounds.low.size() > 0)
  {
    ompl::base::RealVectorStateManifold *manifold = new ompl::base::RealVectorStateManifold(real_vector_bounds.low.size());
    manifold->setName("real_vector");
    manifold->setBounds(real_vector_bounds);
    for(unsigned int i=0; i < real_vector_names.size(); i++)
      manifold->setDimensionName(i,real_vector_names[i]);
    state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold),1.0);
    kinematic_ompl_mapping.real_vector_index = state_manifold->getSubManifoldCount()-1;
    ompl_kinematic_mapping.real_vector_index = kinematic_ompl_mapping.real_vector_index;
    ompl_kinematic_mapping.ompl_state_mapping.push_back(-1);
    ompl_kinematic_mapping.mapping_type.push_back(ompl_ros_interface::REAL_VECTOR);
  }
  for(unsigned int i=0; i < state_manifold->getSubManifoldCount(); i++)
  {
    ROS_DEBUG("State manifold: sub-manifold %d has name %s",i,state_manifold->getSubManifold(i)->getName().c_str());
  }
  return ompl_state_manifold;
};

bool addToOmplStateManifold(boost::shared_ptr<planning_models::KinematicModel> kinematic_model, 
                            const std::string &joint_name,
                            ompl::base::StateManifoldPtr &ompl_state_manifold)
{
  ompl::base::CompoundStateManifold* state_manifold = dynamic_cast<ompl::base::CompoundStateManifold*> (ompl_state_manifold.get());

  if(!kinematic_model->hasJointModel(joint_name))
    { 
      ROS_DEBUG("Could not find joint %s",joint_name.c_str());
      return false;
    }
  const planning_models::KinematicModel::JointModel* joint_model = kinematic_model->getJointModel(joint_name);

  const planning_models::KinematicModel::RevoluteJointModel* revolute_joint = 
    dynamic_cast<const planning_models::KinematicModel::RevoluteJointModel*>(joint_model);
  if (revolute_joint && revolute_joint->continuous_)
  {
    ompl::base::SO2StateManifold *manifold = new ompl::base::SO2StateManifold();
    manifold->setName(revolute_joint->getName());
    state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);
    ROS_DEBUG("Adding SO2 manifold with name %s",revolute_joint->getName().c_str());
  }
  else
  {
    const planning_models::KinematicModel::PlanarJointModel* planar_joint = 
      dynamic_cast<const planning_models::KinematicModel::PlanarJointModel*>(joint_model);
    if (planar_joint)
    {
      ompl::base::SE2StateManifold *manifold = new ompl::base::SE2StateManifold();
      manifold->setName(planar_joint->getName());
      state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);
    }
    else
    {
      const planning_models::KinematicModel::FloatingJointModel* floating_joint = 
        dynamic_cast<const planning_models::KinematicModel::FloatingJointModel*>(joint_model);
      if (floating_joint)
      {
        ompl::base::SE3StateManifold *manifold = new ompl::base::SE3StateManifold();
        manifold->setName(floating_joint->getName());
        state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold), 1.0);
      }
      else
      {
        // the only other case we consider is R^n; since we know that for now at least the only other type of joint available is non-continuous revolute joints
        // we can use the revoluteJoint cast      
        int real_vector_index = -1;
        if(state_manifold->hasSubManifold("real_vector"))
          real_vector_index = state_manifold->getSubManifoldIndex("real_vector");
 
        if(real_vector_index < 0)
        {
          real_vector_index = state_manifold->getSubManifoldCount();
          ompl::base::RealVectorStateManifold *manifold = new ompl::base::RealVectorStateManifold(0);
          manifold->setName("real_vector");
          state_manifold->addSubManifold(ompl::base::StateManifoldPtr(manifold),1.0);
        }
        ompl::base::StateManifoldPtr real_vector_manifold = state_manifold->getSubManifold("real_vector");
        double min_value, max_value;
        std::pair<double,double> bounds = revolute_joint->getVariableBounds(joint_name);
        min_value = bounds.first;
        max_value = bounds.second;
        real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->addDimension(joint_name,min_value,max_value);    
      }
    }    
  }
  return true;
};


ompl_ros_interface::MAPPING_TYPE getMappingType(const planning_models::KinematicModel::JointModel *joint_model)
{
	const planning_models::KinematicModel::RevoluteJointModel* revolute_joint = 
    dynamic_cast<const planning_models::KinematicModel::RevoluteJointModel*>(joint_model);
  if(revolute_joint && revolute_joint->continuous_)
    return ompl_ros_interface::SO2;
  else if(revolute_joint)
    return ompl_ros_interface::REAL_VECTOR;

  const planning_models::KinematicModel::PlanarJointModel* planar_joint = 
    dynamic_cast<const planning_models::KinematicModel::PlanarJointModel*>(joint_model);
  if(planar_joint)
    return ompl_ros_interface::SE2;

  const planning_models::KinematicModel::FloatingJointModel* floating_joint = 
    dynamic_cast<const planning_models::KinematicModel::FloatingJointModel*>(joint_model);
  if(floating_joint)
    return ompl_ros_interface::SE3;

  return ompl_ros_interface::UNKNOWN;
};

ompl_ros_interface::MAPPING_TYPE getMappingType(const ompl::base::StateManifold *state_manifold)
{
	const ompl::base::SO2StateManifold* SO2_manifold = 
    dynamic_cast<const ompl::base::SO2StateManifold*>(state_manifold);
  if(SO2_manifold)
    return ompl_ros_interface::SO2;

	const ompl::base::SE2StateManifold* SE2_manifold = 
    dynamic_cast<const ompl::base::SE2StateManifold*>(state_manifold);
  if(SE2_manifold)
    return ompl_ros_interface::SE2;

	const ompl::base::SO3StateManifold* SO3_manifold = 
    dynamic_cast<const ompl::base::SO3StateManifold*>(state_manifold);
  if(SO3_manifold)
    return ompl_ros_interface::SO3;

	const ompl::base::SE3StateManifold* SE3_manifold = 
    dynamic_cast<const ompl::base::SE3StateManifold*>(state_manifold);
  if(SE3_manifold)
    return ompl_ros_interface::SE3;

	const ompl::base::RealVectorStateManifold* real_vector_manifold = 
    dynamic_cast<const ompl::base::RealVectorStateManifold*>(state_manifold);
  if(real_vector_manifold)
    return ompl_ros_interface::REAL_VECTOR;

  return ompl_ros_interface::UNKNOWN;
};

ompl_ros_interface::MAPPING_TYPE getMappingType(const ompl::base::StateManifoldPtr &state_manifold)
{
  return getMappingType((const ompl::base::StateManifold*) state_manifold.get());
};


bool getJointStateGroupToOmplStateMapping(planning_models::KinematicState::JointStateGroup* joint_state_group, 
                                          const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                          ompl_ros_interface::KinematicStateToOmplStateMapping &mapping)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  unsigned int num_states = joint_state_group->getDimension();
  mapping.joint_state_mapping.resize(num_states,-1);
  mapping.joint_mapping_type.resize(num_states,ompl_ros_interface::UNKNOWN);

  bool joint_state_index_found = false;
  for(unsigned int j=0; j < num_manifolds; j++)
  {
    if(dynamic_cast<ompl::base::RealVectorStateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j).get()))
    {
      mapping.real_vector_index = j;
      joint_state_index_found = true;
      break;
    }
  }
  std::vector<std::string> joint_names = joint_state_group->getJointNames();
  const planning_models::KinematicModel::JointModelGroup* joint_model_group = joint_state_group->getJointModelGroup();
  std::vector<const planning_models::KinematicModel::JointModel*> joint_models = joint_model_group->getJointModels();

  for(unsigned int i=0; i < num_states; i++)
  {
    std::string name = joint_names[i];
    bool mapping_found = false;
    for(unsigned int j=0; j < num_manifolds; j++)
    {
      if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j)->getName() == name)
      {
        mapping.joint_state_mapping[i] = j;
        mapping.joint_mapping_type[i] = getMappingType(joint_models[i]);
        mapping_found = true;
        break;
      }    
    }
    if(!mapping_found)
    {
      //search through the real vector if it exists
      if(joint_state_index_found)
      {
        ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
        for(unsigned int j=0; j < real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(); j++)
        {
          if(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(j) == name)
          {
            mapping.joint_state_mapping[i] = j;
            mapping_found = true;
            mapping.joint_mapping_type[i] = ompl_ros_interface::REAL_VECTOR;
            break;
          } 
        }
      }        
    }
    if(!mapping_found)
    {
      ROS_ERROR("Could not find mapping for joint %s",name.c_str());
      return false;
    }
  }
  return true;
};

bool getOmplStateToJointStateGroupMapping(const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                          planning_models::KinematicState::JointStateGroup* joint_state_group,
                                          ompl_ros_interface::OmplStateToKinematicStateMapping &mapping)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  unsigned int num_states = joint_state_group->getDimension();
  mapping.ompl_state_mapping.resize(num_manifolds,-1);
  mapping.mapping_type.resize(num_manifolds,ompl_ros_interface::UNKNOWN);

  bool joint_state_index_found = false;
  for(unsigned int j=0; j < num_manifolds; j++)
  {
    if(dynamic_cast<ompl::base::RealVectorStateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j).get()))
    {
      mapping.real_vector_index = j;
      mapping.mapping_type[j] = ompl_ros_interface::REAL_VECTOR;
      joint_state_index_found = true;
      break;
    }
  }

  std::vector<std::string> joint_names = joint_state_group->getJointNames();
  std::vector<const planning_models::KinematicModel::JointModel*> joint_models = joint_state_group->getJointModelGroup()->getJointModels();
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    bool mapping_found = false;
    for(unsigned int j=0; j < num_states; j++)
    {
      if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName() == joint_names[j])
      {
        mapping.ompl_state_mapping[i] = j;
        mapping.mapping_type[i] = getMappingType(joint_models[j]);
        mapping_found = true;
        break;
      }    
    }
  }
  ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
  mapping.real_vector_mapping.resize(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(),-1);
  for(unsigned int i=0; i < real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(); i++)
  {
    bool ompl_real_vector_mapping_found = false;
    for(unsigned int j=0; j < num_states; j++)
    {
      if(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i) == joint_names[j])
      {
        mapping.real_vector_mapping[i] = j;
        ompl_real_vector_mapping_found = true;
      } 
    }
    if(!ompl_real_vector_mapping_found)
    {
      ROS_ERROR("Could not find mapping for joint_state %s",real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i).c_str());
      return false;
    }
  }  
  return true;
};

bool getRobotStateToOmplStateMapping(const motion_planning_msgs::RobotState &robot_state, 
                                     const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                     ompl_ros_interface::RobotStateToOmplStateMapping &mapping,
                                     const bool &fail_if_match_not_found)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  mapping.multi_dof_mapping.resize(robot_state.multi_dof_joint_state.joint_names.size(),-1);
  mapping.multi_dof_joint_mapping_type.resize(robot_state.multi_dof_joint_state.joint_names.size(),ompl_ros_interface::UNKNOWN);
  for(unsigned int i=0; i < robot_state.multi_dof_joint_state.joint_names.size(); i++)
  {
    std::string name = robot_state.multi_dof_joint_state.joint_names[i];
    bool mapping_found = false;
    for(unsigned int j=0; j < num_manifolds; j++)
    {
      if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j)->getName() == name)
      {
        mapping.multi_dof_mapping[i] = j;
        mapping.multi_dof_joint_mapping_type[i] = getMappingType((ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j)).get());
        mapping_found = true;
        break;
      }    
    }
    if(!mapping_found && fail_if_match_not_found)
    {
      ROS_ERROR("Could not find mapping for multi_dof_joint_state %s",name.c_str());
      return false;
    }
  }
  mapping.real_vector_index = -1;
  //If robot state has no joint state, don't care about this part
  if(robot_state.joint_state.name.empty())
    return true;
  else
    return getJointStateToOmplStateMapping(robot_state.joint_state,ompl_scoped_state,mapping,fail_if_match_not_found);
};

bool getJointStateToOmplStateMapping(const sensor_msgs::JointState &joint_state, 
                                     const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                     ompl_ros_interface::RobotStateToOmplStateMapping &mapping,
                                     const bool &fail_if_match_not_found)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  bool joint_state_index_found = false;
  for(unsigned int j=0; j < num_manifolds; j++)
  {
    if(dynamic_cast<ompl::base::RealVectorStateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j).get()))
    {
      mapping.real_vector_index = j;
      joint_state_index_found = true;
      break;
    }
  }
  if(!joint_state_index_found && fail_if_match_not_found)
    return false;
  mapping.joint_state_mapping.resize(joint_state.name.size(),-1);  
  mapping.joint_mapping_type.resize(joint_state.name.size(),ompl_ros_interface::UNKNOWN);
  ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
  for(unsigned int i=0; i < joint_state.name.size(); i++)
  {
    bool joint_state_mapping_found = false;
    for(unsigned int j=0; j < real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(); j++)
    {
      if(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(j) == joint_state.name[i])
      {
        mapping.joint_state_mapping[i] = j;
        joint_state_mapping_found = true;
        mapping.joint_mapping_type[i] = ompl_ros_interface::REAL_VECTOR;
      } 
    }
    for(unsigned int j=0; j < num_manifolds; j++)
    {
      if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j)->getName() == joint_state.name[i])
      {
        mapping.joint_state_mapping[i] = j;
        joint_state_mapping_found = true;
        mapping.joint_mapping_type[i] = ompl_ros_interface::SO2;
        break;
      }
    }
    if(!joint_state_mapping_found && fail_if_match_not_found)
    {
      ROS_ERROR("Could not find mapping for joint_state %s",joint_state.name[i].c_str());
      return false;
    }
  }  
  return true;
};

bool getOmplStateToJointStateMapping(const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                     const sensor_msgs::JointState &joint_state,
                                     ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                                     const bool &fail_if_match_not_found)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  if(mapping.ompl_state_mapping.size() != num_manifolds)
    mapping.ompl_state_mapping.resize(num_manifolds,-1);
  if(mapping.mapping_type.size() != num_manifolds)
    mapping.mapping_type.resize(num_manifolds,ompl_ros_interface::UNKNOWN);
  bool ompl_real_vector_index_found = false;
  for(unsigned int j=0; j < num_manifolds; j++)
  {
    if(dynamic_cast<ompl::base::RealVectorStateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(j).get()))
    {
      mapping.real_vector_index = j;
      mapping.mapping_type[j] = ompl_ros_interface::REAL_VECTOR;
      ompl_real_vector_index_found = true;
      break;
    }
  }
  if(!ompl_real_vector_index_found)
    return false;

  ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
  mapping.real_vector_mapping.resize(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(),-1);
  for(unsigned int i=0; i < real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(); i++)
  {
    bool ompl_real_vector_mapping_found = false;
    for(unsigned int j=0; j < joint_state.name.size(); j++)
    {
      if(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i) == joint_state.name[j])
      {
        mapping.real_vector_mapping[i] = j;
        ompl_real_vector_mapping_found = true;
      } 
    }
    if(fail_if_match_not_found && !ompl_real_vector_mapping_found)
    {
      ROS_ERROR("Could not find mapping for joint_state %s",real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i).c_str());
      return false;
    }
  }  
  // Now do SO(2) separately
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(dynamic_cast<ompl::base::SO2StateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i).get()))
    {
      bool ompl_state_mapping_found = false;
      for(unsigned int j=0; j < joint_state.name.size(); j++)
      {
        if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName() == joint_state.name[j])
        {
          mapping.ompl_state_mapping[i] = j;
          mapping.mapping_type[i] = ompl_ros_interface::SO2;
          ompl_state_mapping_found = true;
          break;
        } 
      }
      if(fail_if_match_not_found && !ompl_state_mapping_found)
      {
        ROS_ERROR("Could not find mapping for ompl state %s",ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName().c_str());
        return false;
      }
    }
  }  
  return true;
};

bool getOmplStateToRobotStateMapping(const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                     const motion_planning_msgs::RobotState &robot_state, 
                                     ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                                     const bool &fail_if_match_not_found)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  mapping.ompl_state_mapping.resize(num_manifolds,-1);
  mapping.mapping_type.resize(num_manifolds,ompl_ros_interface::UNKNOWN);
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    bool ompl_state_mapping_found = false;
    for(unsigned int j=0; j < robot_state.multi_dof_joint_state.joint_names.size(); j++)
    {
      if(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName() == robot_state.multi_dof_joint_state.joint_names[j])
      {
        mapping.ompl_state_mapping[i] = j;
        mapping.mapping_type[i] = getMappingType((ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)).get());
        ompl_state_mapping_found = true;
        break;
      } 
    }
    if(fail_if_match_not_found && !ompl_state_mapping_found && !dynamic_cast<ompl::base::SO2StateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i).get()) && !dynamic_cast<ompl::base::RealVectorStateManifold*>(ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i).get()))
    {
      ROS_ERROR("Could not find mapping for ompl state %s",ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName().c_str());
      return false;
    }
  }
  return getOmplStateToJointStateMapping(ompl_scoped_state,robot_state.joint_state,mapping,fail_if_match_not_found);
};

bool omplStateToRobotState(const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                           const ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                           motion_planning_msgs::RobotState &robot_state)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(mapping.mapping_type[i] == ompl_ros_interface::SO2 && mapping.ompl_state_mapping[i] > -1)
      robot_state.joint_state.position[mapping.ompl_state_mapping[i]] = ompl_scoped_state->as<ompl::base::SO2StateManifold::StateType>(i)->value;
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE3 && mapping.ompl_state_mapping[i] > -1)
      ompl_ros_interface::SE3ManifoldToPoseMsg(*(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)),robot_state.multi_dof_joint_state.poses[mapping.ompl_state_mapping[i]]);
    else if(mapping.mapping_type[i] == ompl_ros_interface::REAL_VECTOR) // real vector value
    {
      //      ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
      ompl_ros_interface::omplRealVectorStateToJointState(*(ompl_scoped_state->as<ompl::base::RealVectorStateManifold::StateType>(i)),mapping,robot_state.joint_state);
    }
  }
  return true;
};

bool omplStateToRobotState(const ompl::base::State &ompl_state,
                           const ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                           motion_planning_msgs::RobotState &robot_state)
{
  const ompl::base::CompoundState *ompl_compound_state = static_cast<const ompl::base::CompoundState*> (&ompl_state);
  unsigned int num_manifolds = mapping.mapping_type.size();
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(mapping.mapping_type[i] == ompl_ros_interface::SO2 && mapping.ompl_state_mapping[i] > -1)
      robot_state.joint_state.position[mapping.ompl_state_mapping[i]] = ompl_compound_state->as<ompl::base::SO2StateManifold::StateType>(i)->value;
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE3 && mapping.ompl_state_mapping[i] > -1)
      ompl_ros_interface::SE3ManifoldToPoseMsg(*(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)),robot_state.multi_dof_joint_state.poses[mapping.ompl_state_mapping[i]]);
    else if(mapping.mapping_type[i] == ompl_ros_interface::REAL_VECTOR) // real vector value
    {
      //      ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
      ompl_ros_interface::omplRealVectorStateToJointState(*(ompl_compound_state->as<ompl::base::RealVectorStateManifold::StateType>(i)),mapping,robot_state.joint_state);
    }
  }
  return true;
};

bool omplRealVectorStateToJointState(const ompl::base::RealVectorStateManifold::StateType &ompl_state,
                                     const ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                                     sensor_msgs::JointState &joint_state)
{
  for(unsigned int i=0; i < mapping.real_vector_mapping.size(); i++)
    if(mapping.real_vector_mapping[i] > -1)
      joint_state.position[mapping.real_vector_mapping[i]] = ompl_state.values[i];
  return true;
};


bool robotStateToOmplState(const motion_planning_msgs::RobotState &robot_state,
                           const ompl_ros_interface::RobotStateToOmplStateMapping &mapping,
                           ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                           const bool &fail_if_match_not_found)
{
  return robotStateToOmplState(robot_state,mapping,ompl_scoped_state.get(),fail_if_match_not_found);
};

bool robotStateToOmplState(const motion_planning_msgs::RobotState &robot_state,
                           const ompl_ros_interface::RobotStateToOmplStateMapping &mapping,
                           ompl::base::State *ompl_state,
                           const bool &fail_if_match_not_found)
{
  for(unsigned int i=0; i < robot_state.multi_dof_joint_state.joint_names.size(); i++)
  {
    if(mapping.multi_dof_mapping[i] > -1)
      ompl_ros_interface::poseMsgToSE3Manifold(robot_state.multi_dof_joint_state.poses[i],
                                               *(ompl_state->as<ompl::base::CompoundState>()->as<ompl::base::SE3StateManifold::StateType>(mapping.multi_dof_mapping[i])));
  }
  if(mapping.real_vector_index > -1)
  {
    for(unsigned int i=0; i < robot_state.joint_state.name.size(); i++)
    {
      if(mapping.joint_mapping_type[i] == ompl_ros_interface::SO2 && mapping.joint_state_mapping[i] > -1)
        ompl_state->as<ompl::base::CompoundState>()->as<ompl::base::SO2StateManifold::StateType>(mapping.joint_state_mapping[i])->value = angles::normalize_angle(robot_state.joint_state.position[i]);
    }
  }
  if(mapping.real_vector_index > -1)
  {
    return jointStateToRealVectorState(robot_state.joint_state,
                                       mapping,
                                       *(ompl_state->as<ompl::base::CompoundState>()->as<ompl::base::RealVectorStateManifold::StateType>(mapping.real_vector_index)),
                                       fail_if_match_not_found);
  }
  return true;                                       
};

bool robotStateToOmplState(const motion_planning_msgs::RobotState &robot_state,
                           ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                           const bool &fail_if_match_not_found)
{
  ompl_ros_interface::RobotStateToOmplStateMapping mapping;
  if(!getRobotStateToOmplStateMapping(robot_state,ompl_scoped_state,mapping,fail_if_match_not_found))
    return false;
  return robotStateToOmplState(robot_state,mapping,ompl_scoped_state.get(),fail_if_match_not_found);
};

bool jointStateToRealVectorState(const sensor_msgs::JointState &joint_state,
                                 const ompl_ros_interface::RobotStateToOmplStateMapping &mapping,
                                 ompl::base::RealVectorStateManifold::StateType &real_vector_state,
                                 const bool &fail_if_match_not_found)
{
  for(unsigned int i=0; i < joint_state.name.size(); i++)
    if(mapping.joint_mapping_type[i] == ompl_ros_interface::REAL_VECTOR && mapping.joint_state_mapping[i] > -1)
      real_vector_state.as<ompl::base::RealVectorStateManifold::StateType>()->values[mapping.joint_state_mapping[i]] = joint_state.position[i];          
  return true;
};

void SE3ManifoldToPoseMsg(const ompl::base::SE3StateManifold::StateType &pose,
                          geometry_msgs::Pose &pose_msg)
{
  pose_msg.position.x = pose.getX();
  pose_msg.position.y = pose.getY();
  pose_msg.position.z = pose.getZ();
  ompl::base::SO3StateManifold::StateType quaternion;
  quaternion.x = pose.rotation().x;
  quaternion.y = pose.rotation().y;
  quaternion.z = pose.rotation().z;
  quaternion.w = pose.rotation().w;

  SO3ManifoldToQuaternionMsg(quaternion,pose_msg.orientation);
};

void SO3ManifoldToPoseMsg(const ompl::base::SO3StateManifold::StateType &quaternion,
                          geometry_msgs::Pose &pose_msg)
{
  SO3ManifoldToQuaternionMsg(quaternion,pose_msg.orientation);
};

void SE2ManifoldToPoseMsg(const ompl::base::SE2StateManifold::StateType &pose,
                          geometry_msgs::Pose &pose_msg)
{
  pose_msg.position.x = pose.getX();
  pose_msg.position.y = pose.getY();
  //TODO set theta
};


void SO3ManifoldToQuaternionMsg(const ompl::base::SO3StateManifold::StateType &quaternion,
                                geometry_msgs::Quaternion &quaternion_msg)
{
  quaternion_msg.x = quaternion.x;
  quaternion_msg.y = quaternion.y;
  quaternion_msg.z = quaternion.z;
  quaternion_msg.w = quaternion.w;
};

void poseMsgToSE3Manifold(const geometry_msgs::Pose &pose_msg,
                          ompl::base::SE3StateManifold::StateType &pose)
{
  pose.setX(pose_msg.position.x);
  pose.setY(pose_msg.position.y);
  pose.setZ(pose_msg.position.z);
  quaternionMsgToSO3Manifold(pose_msg.orientation,pose.rotation());  
};

void quaternionMsgToSO3Manifold(const geometry_msgs::Quaternion &quaternion_msg,
                                ompl::base::SO3StateManifold::StateType &quaternion)
{
  quaternion.x = quaternion_msg.x;
  quaternion.y = quaternion_msg.y;
  quaternion.z = quaternion_msg.z;
  quaternion.w = quaternion_msg.w;
};

/**
   @brief Convert a kinematic state to an ompl state given the appropriate mapping
   @param joint_state_group The kinematic state to convert from
   @param mapping The given mapping
   @param ompl_scoped_state The ompl state to convert to
*/
bool kinematicStateGroupToOmplState(const planning_models::KinematicState::JointStateGroup* joint_state_group, 
                                    const ompl_ros_interface::KinematicStateToOmplStateMapping &mapping,
                                    ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state)
{
  //  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  unsigned int num_states = joint_state_group->getDimension();

  std::vector<planning_models::KinematicState::JointState*> joint_states = joint_state_group->getJointStateVector();
  std::vector<std::string> joint_names = joint_state_group->getJointNames();
  for(unsigned int i=0; i < num_states; i++)
  {
    if(mapping.joint_mapping_type[i] == ompl_ros_interface::SO2)
    {
      std::vector<double> value = joint_states[i]->getJointStateValues();
      ompl_scoped_state->as<ompl::base::SO2StateManifold::StateType>(mapping.joint_state_mapping[i])->value = angles::normalize_angle(value[0]);
    }
    else if(mapping.joint_mapping_type[i] == ompl_ros_interface::SE2)
    {
      std::vector<double> value = joint_states[i]->getJointStateValues();
      ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(mapping.joint_state_mapping[i])->setX(value[0]);
      ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(mapping.joint_state_mapping[i])->setY(value[1]);
      ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(mapping.joint_state_mapping[i])->setYaw(value[2]);
    }
    else if(mapping.joint_mapping_type[i] == ompl_ros_interface::SE3)
    {
      std::vector<double> value = joint_states[i]->getJointStateValues();
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->setX(value[0]);
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->setY(value[1]);
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->setZ(value[2]);
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->rotation().x = value[3];
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->rotation().y = value[4];
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->rotation().z = value[5];
      ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(mapping.joint_state_mapping[i])->rotation().w = value[6];
    }
    else if(mapping.joint_mapping_type[i] == ompl_ros_interface::REAL_VECTOR)
    {
      std::vector<double> value = joint_states[i]->getJointStateValues();
      ompl_scoped_state->as<ompl::base::RealVectorStateManifold::StateType>(mapping.real_vector_index)->values[mapping.joint_state_mapping[i]] = value[0];
    }
  }
  return true;
};

/**
   @brief Convert an ompl state to a kinematic state given the appropriate mapping
   @param ompl_scoped_state The ompl state to convert to
   @param mapping The given mapping
   @param joint_state_group The kinematic state to convert from
*/
bool omplStateToKinematicStateGroup(const ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                                    const ompl_ros_interface::OmplStateToKinematicStateMapping &mapping,
                                    planning_models::KinematicState::JointStateGroup* joint_state_group)
{
  unsigned int num_manifolds = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  //  unsigned int num_states = joint_state_group->getDimension();

  std::vector<planning_models::KinematicState::JointState*> joint_states = joint_state_group->getJointStateVector();
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(mapping.mapping_type[i] == ompl_ros_interface::SO2)
    {
      std::vector<double> value;
      value.push_back(ompl_scoped_state->as<ompl::base::SO2StateManifold::StateType>(i)->value);
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(value);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE2)
    {
      std::vector<double> values;
      values.push_back(ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(i)->getX());
      values.push_back(ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(i)->getY());
      values.push_back(ompl_scoped_state->as<ompl::base::SE2StateManifold::StateType>(i)->getYaw());
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(values);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE3)
    {
      std::vector<double> values;
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->getX());
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->getY());
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->getZ());
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().x);
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().y);
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().z);
      values.push_back(ompl_scoped_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().w);
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(values);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::REAL_VECTOR)
    {
      ompl::base::StateManifoldPtr real_vector_manifold = ompl_scoped_state.getManifold()->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
      unsigned int real_vector_size = mapping.real_vector_mapping.size();
      for(unsigned int j=0; j < real_vector_size; j++)
      {
        std::vector<double> values;
        values.push_back(ompl_scoped_state->as<ompl::base::RealVectorStateManifold::StateType>(mapping.real_vector_index)->values[j]);
        joint_states[mapping.real_vector_mapping[j]]->setJointStateValues(values);
      }
    }
  }
  return true;
};

/**
   @brief Convert an ompl state to a kinematic state given the appropriate mapping
   @param ompl_state The ompl state to convert to
   @param mapping The given mapping
   @param joint_state_group The kinematic state to convert from
*/
bool omplStateToKinematicStateGroup(const ompl::base::State *ompl_state,
                                    const ompl_ros_interface::OmplStateToKinematicStateMapping &mapping,
                                    planning_models::KinematicState::JointStateGroup* joint_state_group)
{
  unsigned int num_manifolds = mapping.ompl_state_mapping.size();
  const ompl::base::CompoundState *ompl_compound_state = dynamic_cast<const ompl::base::CompoundState*> (ompl_state);

  std::vector<planning_models::KinematicState::JointState*> joint_states = joint_state_group->getJointStateVector();
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(mapping.mapping_type[i] == ompl_ros_interface::SO2)
    {
      std::vector<double> value;
      value.push_back(ompl_compound_state->as<ompl::base::SO2StateManifold::StateType>(i)->value);
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(value);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE2)
    {
      std::vector<double> values;
      values.push_back(ompl_compound_state->as<ompl::base::SE2StateManifold::StateType>(i)->getX());
      values.push_back(ompl_compound_state->as<ompl::base::SE2StateManifold::StateType>(i)->getY());
      values.push_back(ompl_compound_state->as<ompl::base::SE2StateManifold::StateType>(i)->getYaw());
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(values);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::SE3)
    {
      std::vector<double> values;
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->getX());
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->getY());
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->getZ());
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().x);
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().y);
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().z);
      values.push_back(ompl_compound_state->as<ompl::base::SE3StateManifold::StateType>(i)->rotation().w);
      joint_states[mapping.ompl_state_mapping[i]]->setJointStateValues(values);
    }
    else if(mapping.mapping_type[i] == ompl_ros_interface::REAL_VECTOR)
    {
      unsigned int real_vector_size = mapping.real_vector_mapping.size();
      for(unsigned int j=0; j < real_vector_size; j++)
      {
        std::vector<double> values;
        values.push_back(ompl_compound_state->as<ompl::base::RealVectorStateManifold::StateType>(mapping.real_vector_index)->values[j]);
        joint_states[mapping.real_vector_mapping[j]]->setJointStateValues(values);
      }
    }
  }
  return true;
};

/**
   @brief Create a robot trajectory message for a joint state group
   @param joint_state_group The input group
   @param robot_trajectory The output robot trajectory
*/
bool jointStateGroupToRobotTrajectory(planning_models::KinematicState::JointStateGroup* joint_state_group, 
                                      motion_planning_msgs::RobotTrajectory &robot_trajectory)
{
  const planning_models::KinematicModel::JointModelGroup* joint_model_group = joint_state_group->getJointModelGroup();
  std::vector<const planning_models::KinematicModel::JointModel*> joint_models = joint_model_group->getJointModels();
  for(unsigned int i=0; i < joint_models.size(); i++)
  {
		const planning_models::KinematicModel::RevoluteJointModel* revolute_joint = 
			dynamic_cast<const planning_models::KinematicModel::RevoluteJointModel*>(joint_models[i]);
		const planning_models::KinematicModel::PrismaticJointModel* prismatic_joint = 
			dynamic_cast<const planning_models::KinematicModel::PrismaticJointModel*>(joint_models[i]);
    const planning_models::KinematicModel::PlanarJointModel* planar_joint = 
      dynamic_cast<const planning_models::KinematicModel::PlanarJointModel*>(joint_models[i]);
    const planning_models::KinematicModel::FloatingJointModel* floating_joint = 
      dynamic_cast<const planning_models::KinematicModel::FloatingJointModel*>(joint_models[i]);
    if (revolute_joint || prismatic_joint)
      robot_trajectory.joint_trajectory.joint_names.push_back(joint_models[i]->getName());
    else if (planar_joint || floating_joint)
    {
      robot_trajectory.multi_dof_joint_trajectory.joint_names.push_back(joint_models[i]->getName());
      robot_trajectory.multi_dof_joint_trajectory.frame_ids.push_back(joint_models[i]->getParentFrameId());
      robot_trajectory.multi_dof_joint_trajectory.child_frame_ids.push_back(joint_models[i]->getChildFrameId());
    }
    else
      return false;  
  }
  return true;
};


bool omplPathGeometricToRobotTrajectory(const ompl::geometric::PathGeometric &path,
                                        const ompl::base::StateManifoldPtr &state_manifold,
                                        motion_planning_msgs::RobotTrajectory &robot_trajectory)
{
  if(robot_trajectory.joint_trajectory.joint_names.empty() && robot_trajectory.multi_dof_joint_trajectory.joint_names.empty())
  {
    ROS_ERROR("Robot trajectory needs to initialized before calling this function");
    return false;
  }
  ompl_ros_interface::OmplStateToRobotStateMapping mapping;
  if(!getOmplStateToRobotTrajectoryMapping(state_manifold,robot_trajectory,mapping))
    return false;
  if(!omplPathGeometricToRobotTrajectory(path,mapping,robot_trajectory))
    return false;
  return true;
};

bool omplPathGeometricToRobotTrajectory(const ompl::geometric::PathGeometric &path,
                                        const ompl_ros_interface::OmplStateToRobotStateMapping &mapping,
                                        motion_planning_msgs::RobotTrajectory &robot_trajectory)
{
  if(robot_trajectory.joint_trajectory.joint_names.empty() && robot_trajectory.multi_dof_joint_trajectory.joint_names.empty())
  {
    ROS_ERROR("Robot trajectory needs to initialized before calling this function");
    return false;
  }
  unsigned int num_points = path.states.size();
  unsigned int num_manifolds = mapping.ompl_state_mapping.size();
  bool multi_dof = false;
  bool single_dof = false;

  if(!robot_trajectory.multi_dof_joint_trajectory.joint_names.empty())
  {
    multi_dof = true;
    robot_trajectory.multi_dof_joint_trajectory.points.resize(num_points);
    for(unsigned int i=0; i < num_points; i++)
      {
	robot_trajectory.multi_dof_joint_trajectory.points[i].poses.resize(robot_trajectory.multi_dof_joint_trajectory.joint_names.size());
	robot_trajectory.multi_dof_joint_trajectory.points[i].time_from_start = ros::Duration(0.0);
      }
  }
  if(!robot_trajectory.joint_trajectory.joint_names.empty())
  {
    single_dof = true;
    robot_trajectory.joint_trajectory.points.resize(num_points);
    for(unsigned int i=0; i < num_points; i++)
      {      
	robot_trajectory.joint_trajectory.points[i].positions.resize(robot_trajectory.joint_trajectory.joint_names.size());
	robot_trajectory.joint_trajectory.points[i].time_from_start = ros::Duration(0.0);
      }
  }
  
  for(unsigned int i=0; i < num_points; i++)
  {
    for(unsigned int j=0; j < num_manifolds; j++)
    {
      if(mapping.mapping_type[j] == ompl_ros_interface::SO2)
        robot_trajectory.joint_trajectory.points[i].positions[mapping.ompl_state_mapping[j]] = path.states[i]->as<ompl::base::CompoundState>()->as<ompl::base::SO2StateManifold::StateType>(j)->value;
      else if(mapping.mapping_type[j] == ompl_ros_interface::SE2)
        ompl_ros_interface::SE2ManifoldToPoseMsg(*(path.states[i]->as<ompl::base::CompoundState>()->as<ompl::base::SE2StateManifold::StateType>(j)),robot_trajectory.multi_dof_joint_trajectory.points[i].poses[mapping.ompl_state_mapping[j]]);
      else if(mapping.mapping_type[j] == ompl_ros_interface::SE3)
        ompl_ros_interface::SE3ManifoldToPoseMsg(*(path.states[i]->as<ompl::base::CompoundState>()->as<ompl::base::SE3StateManifold::StateType>(j)),robot_trajectory.multi_dof_joint_trajectory.points[i].poses[mapping.ompl_state_mapping[j]]);
      else if(mapping.mapping_type[j] == ompl_ros_interface::SO3)
        ompl_ros_interface::SO3ManifoldToPoseMsg(*(path.states[i]->as<ompl::base::CompoundState>()->as<ompl::base::SO3StateManifold::StateType>(j)),robot_trajectory.multi_dof_joint_trajectory.points[i].poses[mapping.ompl_state_mapping[j]]);
      else // real vector value
      {
        ompl::base::State* real_vector_state = path.states[i]->as<ompl::base::CompoundState>()->as<ompl::base::RealVectorStateManifold::StateType>(mapping.real_vector_index);
        for(unsigned int k=0; k < mapping.real_vector_mapping.size(); k++)
          robot_trajectory.joint_trajectory.points[i].positions[mapping.real_vector_mapping[k]] = real_vector_state->as<ompl::base::RealVectorStateManifold::StateType>()->values[k];
      }
    }
  }
  return true;  
};

bool getOmplStateToRobotTrajectoryMapping(const ompl::base::StateManifoldPtr &state_manifold,
                                          const motion_planning_msgs::RobotTrajectory &robot_trajectory, 
                                          ompl_ros_interface::OmplStateToRobotStateMapping &mapping)
{
  unsigned int num_manifolds = state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  mapping.ompl_state_mapping.resize(num_manifolds,-1);
  mapping.mapping_type.resize(num_manifolds,ompl_ros_interface::UNKNOWN);
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    ROS_DEBUG("Trajectory state manifold %d has name %s",i,state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName().c_str());
    bool ompl_state_mapping_found = false;
    for(unsigned int j=0; j < robot_trajectory.multi_dof_joint_trajectory.joint_names.size(); j++)
    {
      if(state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName() == robot_trajectory.multi_dof_joint_trajectory.joint_names[j])
      {
        mapping.ompl_state_mapping[i] = j;
        mapping.mapping_type[i] = getMappingType((state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)).get());
        ompl_state_mapping_found = true;
        break;
      } 
    }
  }
  bool result = getOmplStateToJointTrajectoryMapping(state_manifold,robot_trajectory.joint_trajectory,mapping);
  for(int i=0; i < (int) num_manifolds; i++)
  {
    if(mapping.ompl_state_mapping[i] < 0 && i != (int) mapping.real_vector_index)
    {
      ROS_ERROR("Could not find mapping for manifold %s",state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName().c_str());
      return false;
    }
  };
  return result;
};

bool getOmplStateToJointTrajectoryMapping(const ompl::base::StateManifoldPtr &state_manifold,
                                          const trajectory_msgs::JointTrajectory &joint_trajectory,
                                          ompl_ros_interface::OmplStateToRobotStateMapping &mapping)
{
  unsigned int num_manifolds = state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifoldCount();
  if(mapping.ompl_state_mapping.size() != num_manifolds)
    mapping.ompl_state_mapping.resize(num_manifolds,-1);
  if(mapping.mapping_type.size() != num_manifolds)
    mapping.mapping_type.resize(num_manifolds,ompl_ros_interface::UNKNOWN);

  // Do SO(2) separately
  for(unsigned int i=0; i < num_manifolds; i++)
  {
    if(dynamic_cast<ompl::base::SO2StateManifold*>(state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i).get()))
    {
      bool ompl_state_mapping_found = false;
      for(unsigned int j=0; j < joint_trajectory.joint_names.size(); j++)
      {
        if(state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName() == joint_trajectory.joint_names[j])
        {
          mapping.ompl_state_mapping[i] = j;
          mapping.mapping_type[i] = ompl_ros_interface::SO2;
          ompl_state_mapping_found = true;
          ROS_DEBUG("Trajectory mapping for SO2 joint %s from %d to %d",joint_trajectory.joint_names[j].c_str(),i,j);
          break;
        } 
      }
      if(!ompl_state_mapping_found)
      {
        ROS_ERROR("Could not find mapping for ompl state %s",state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(i)->getName().c_str());
        return false;
      }
    }
  }  

  bool ompl_real_vector_index_found = false;
  for(unsigned int j=0; j < num_manifolds; j++)
  {
    if(dynamic_cast<ompl::base::RealVectorStateManifold*>(state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(j).get()))
    {
      mapping.real_vector_index = j;
      mapping.mapping_type[j] = ompl_ros_interface::REAL_VECTOR;
      ompl_real_vector_index_found = true;
      break;
    }
  }
  if(!ompl_real_vector_index_found)
  {
    ROS_WARN("No real vector manifold in ompl state manifold");
    return true;
  }

  ompl::base::StateManifoldPtr real_vector_manifold = state_manifold->as<ompl::base::CompoundStateManifold>()->getSubManifold(mapping.real_vector_index);
  mapping.real_vector_mapping.resize(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(),-1);
  for(unsigned int i=0; i < real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimension(); i++)
  {
    bool ompl_real_vector_mapping_found = false;
    for(unsigned int j=0; j < joint_trajectory.joint_names.size(); j++)
    {
      if(real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i) == joint_trajectory.joint_names[j])
      {
        mapping.real_vector_mapping[i] = j;
        ompl_real_vector_mapping_found = true;
        ROS_DEBUG("Trajectory mapping for revolute joint %s from %d to %d",joint_trajectory.joint_names[j].c_str(),i,j);
      } 
    }
    if(!ompl_real_vector_mapping_found)
    {
      ROS_ERROR("Could not find mapping for ompl state %s",real_vector_manifold->as<ompl::base::RealVectorStateManifold>()->getDimensionName(i).c_str());
      return false;
    }
  }  
  return true;
};

bool jointConstraintsToOmplState(const std::vector<motion_planning_msgs::JointConstraint> &joint_constraints,
                                 ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state)
{
  sensor_msgs::JointState joint_state = motion_planning_msgs::jointConstraintsToJointState(joint_constraints);
  for(unsigned int i=0; i < joint_state.name.size(); i++)
  {
    ROS_DEBUG("Joint %s: %f",joint_state.name[i].c_str(), joint_state.position[i]);
  }
  motion_planning_msgs::RobotState robot_state;
  robot_state.joint_state = joint_state;

  ompl_ros_interface::RobotStateToOmplStateMapping mapping;
  if(ompl_ros_interface::getJointStateToOmplStateMapping(joint_state,ompl_scoped_state,mapping))
    return ompl_ros_interface::robotStateToOmplState(robot_state,mapping,ompl_scoped_state);
  return false;
};

bool constraintsToOmplState(const motion_planning_msgs::Constraints &constraints,
                            ompl::base::ScopedState<ompl::base::CompoundStateManifold> &ompl_scoped_state,
                            const bool &fail_if_match_not_found)
{
  motion_planning_msgs::RobotState robot_state;
  robot_state.joint_state = motion_planning_msgs::jointConstraintsToJointState(constraints.joint_constraints);
  ROS_DEBUG("There are %d joint constraints",constraints.joint_constraints.size());
  for(unsigned int i=0; i < robot_state.joint_state.name.size(); i++)
    ROS_DEBUG("Joint Constraint:: Joint %s: %f",robot_state.joint_state.name[i].c_str(), robot_state.joint_state.position[i]);

  robot_state.multi_dof_joint_state = motion_planning_msgs::poseConstraintsToMultiDOFJointState(constraints.position_constraints,
                                                                                                constraints.orientation_constraints);

  ompl_ros_interface::RobotStateToOmplStateMapping mapping;
  if(ompl_ros_interface::getRobotStateToOmplStateMapping(robot_state,ompl_scoped_state,mapping,fail_if_match_not_found))
    return ompl_ros_interface::robotStateToOmplState(robot_state,mapping,ompl_scoped_state,fail_if_match_not_found);
  return false;
};

bool getRobotStateToJointModelGroupMapping(const motion_planning_msgs::RobotState &robot_state,
                                           const planning_models::KinematicModel::JointModelGroup *joint_model_group,
                                           ompl_ros_interface::RobotStateToKinematicStateMapping &mapping)
{
  std::vector<const planning_models::KinematicModel::JointModel*> joint_models = joint_model_group->getJointModels();
  mapping.joint_state_mapping.resize(robot_state.joint_state.name.size(),-1);
  mapping.multi_dof_mapping.resize(robot_state.multi_dof_joint_state.joint_names.size(),-1);

  for (unsigned int i = 0 ; i < robot_state.joint_state.name.size(); ++i)
  {
    for(unsigned int j=0; j < joint_models.size(); j++)
    {
      if(robot_state.joint_state.name[i] == joint_models[j]->getName())
      {
        mapping.joint_state_mapping[i] = j;
        break;
      }
    }
    if(mapping.joint_state_mapping[i] < 0)
    {
      ROS_ERROR("Mapping does not exist for joint %s",robot_state.joint_state.name[i].c_str());
      return false;
    }
  }
  for (unsigned int i = 0 ; i < robot_state.multi_dof_joint_state.joint_names.size(); ++i)
  {
    for(unsigned int j=0; j < joint_models.size(); j++)
    {
      if(robot_state.multi_dof_joint_state.joint_names[i] == joint_models[j]->getName())
      {
        mapping.multi_dof_mapping[i] = j;
        break;
      }
    }
    if(mapping.multi_dof_mapping[i] < 0)
    {
      ROS_ERROR("Mapping does not exist for joint %s",robot_state.multi_dof_joint_state.joint_names[i].c_str());
      return false;
    }
  }
  return true;
}

bool robotStateToJointStateGroup(const motion_planning_msgs::RobotState &robot_state,
                                 const ompl_ros_interface::RobotStateToKinematicStateMapping &mapping,
                                 planning_models::KinematicState::JointStateGroup *joint_state_group)
{
  std::vector<planning_models::KinematicState::JointState*> joint_states = joint_state_group->getJointStateVector();
  for(unsigned int i=0; i < robot_state.joint_state.name.size(); i++)
  {
    std::vector<double> value;
    value.push_back(robot_state.joint_state.position[i]);
    joint_states[mapping.joint_state_mapping[i]]->setJointStateValues(value);
  }    

  for(unsigned int i=0; i < robot_state.multi_dof_joint_state.joint_names.size(); i++)
  {
    std::vector<double> values;
    values.push_back(robot_state.multi_dof_joint_state.poses[i].position.x);
    values.push_back(robot_state.multi_dof_joint_state.poses[i].position.y);
    values.push_back(robot_state.multi_dof_joint_state.poses[i].position.z);

    values.push_back(robot_state.multi_dof_joint_state.poses[i].orientation.x);
    values.push_back(robot_state.multi_dof_joint_state.poses[i].orientation.y);
    values.push_back(robot_state.multi_dof_joint_state.poses[i].orientation.z);
    values.push_back(robot_state.multi_dof_joint_state.poses[i].orientation.w);

    int index = mapping.multi_dof_mapping[i];
    joint_states[index]->setJointStateValues(values);
  }
  return true;
}                                 
}
