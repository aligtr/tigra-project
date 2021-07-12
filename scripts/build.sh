#!/usr/bin/env bash

export CMAKE_PREFIX_PATH=/usr/local/lib/cmake/:$CMAKE_PREFIX_PATH

catkin build \
    tigra_msgs \
    tigra_description \
    tigra_software \
    tigra_vision \
    tigra_maps \
    ackermann_raw_controller_plugin \
    ublox \
    elp_stereo_camera \
    viso2 \
    orb_slam2_ros \
    rtabmap_ros \
    stereo_image_proc \
    camera_calibration \
    --cmake-args -DOpenCV_DIR="/usr/local/lib/cmake/opencv4" 

    # cv_bridge \
