cmake_minimum_required(VERSION 2.8.3)
project(scan_matching)

add_compile_options(-std=c++14)


find_package(catkin REQUIRED COMPONENTS
        nav_msgs
        roscpp
        rospy
        std_msgs
        tf
        geometry_msgs
        )

find_package(Eigen3 REQUIRED)

catkin_package(
        INCLUDE_DIRS include
        CATKIN_DEPENDS nav_msgs roscpp rospy std_msgs
)


include_directories(
        include
        ${catkin_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
)


add_executable(student_scan_matcher src/scan_match.cpp src/transform.cpp src/correspond.cpp src/visualization.cpp)
target_link_libraries(student_scan_matcher ${catkin_LIBRARIES})
#add_dependencies(scan_matcher emu_imu_scan_matching_generate_messages_cpp)