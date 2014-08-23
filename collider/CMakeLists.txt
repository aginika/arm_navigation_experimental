# Catkin User Guide: http://www.ros.org/doc/groovy/api/catkin/html/user_guide/user_guide.html
# Catkin CMake Standard: http://www.ros.org/doc/groovy/api/catkin/html/user_guide/standards.html
cmake_minimum_required(VERSION 2.8.3)
project(collider)
# Load catkin and all dependencies required for this package
# TODO: remove all from COMPONENTS that are not catkin packages.
find_package(catkin REQUIRED COMPONENTS roscpp tf visualization_msgs arm_navigation_msgs std_msgs std_srvs actionlib actionlib_msgs pcl_ros pcl_conversions image_transport image_geometry robot_self_filter planning_environment octomap_ros octomap_msgs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(Eigen REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread signals)
add_definitions(-Wall -DNDEBUG -DEIGEN_NO_DEBUG -O3 -msse3 -mssse3 -funroll-loops)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OCTOMAP REQUIRED octomap)

include_directories(include ${Boost_INCLUDE_DIR} ${catkin_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${OpenMP_INCLUDE_DIRS} ${Eigen_INCLUDE_DIRS})


add_service_files(
  FILES
  OccupancyBBXQuery.srv
  OccupancyBBXSizeQuery.srv
  OccupancyPointQuery.srv
)

# add_action_files(
#   FILES
#   ConstructStaticCollisionMap.action
# )

generate_messages(
  DEPENDENCIES geometry_msgs std_msgs
)


find_package(ASSIMP QUIET)
if (NOT ASSIMP_FOUND)
  pkg_check_modules(ASSIMP assimp)
endif()
if (ASSIMP_FOUND)
  if( ${ASSIMP_VERSION} STRGREATER "2.0.0" )
    set(IS_ASSIMP3 1)
    add_definitions(-DIS_ASSIMP3)
    message(STATUS "Found assimp v3")
  else()
    message(STATUS "Found assimp v2")
  endif()
  include_directories(${ASSIMP_INCLUDE_DIRS})
  link_directories(${ASSIMP_LIBRARY_DIRS})
else()
  message(STATUS "could not find assimp (perhaps available thorugh ROS package?), so assuming assimp v2")
  set(ASSIMP_LIBRARIES assimp)
  set(ASSIMP_LIBRARY_DIRS)
  set(ASSIMP_CXX_FLAGS)
  set(ASSIMP_CFLAGS_OTHER)
  set(ASSIMP_LINK_FLAGS)
  set(ASSIMP_INCLUDE_DIRS)
  set(IS_ASSIMP3 0) # most likely not
endif()


# catkin_package parameters: http://ros.org/doc/groovy/api/catkin/html/dev_guide/generated_cmake_api.html#catkin-package
# TODO: fill in what other packages will need to use this package
catkin_package(
    DEPENDS  eigen opencv2 pcl openmp
    CATKIN_DEPENDS roscpp tf visualization_msgs arm_navigation_msgs std_msgs std_srvs actionlib actionlib_msgs pcl_ros pcl_conversions image_transport image_geometry robot_self_filter planning_environment octomap_ros octomap_msgs
    INCLUDE_DIRS # TODO include
    LIBRARIES # TODO
)

add_executable(collider_node src/collider.cpp src/collider_node.cpp)
target_link_libraries(collider_node ${catkin_LIBRARIES}
                                    ${OpenCV_LIBRARIES}
                                    ${ASSIMP_LIBRARIES}
                                    ${Boost_LIBRARIES}
                                    ${OpenMP_LIBRARIES}
)