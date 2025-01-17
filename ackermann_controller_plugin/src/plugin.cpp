#include "plugin.h"

#include <boost/assign.hpp>

using namespace std;

namespace gazebo
{

    TigraPlugin::TigraPlugin()
    {
        target_angle_ = 0.0;
        target_speed_mps_ = 0.0;
        throttle_cmd_ = 0.0;
        current_steering_angle_ = 0.0;
        rollover_ = false;

        x_ = 0;
        y_ = 0;
        yaw_ = 0;

        cout << "TigraPlugin plugin created!" << endl;
    }

    void TigraPlugin::Load(physics::ModelPtr model, sdf::ElementPtr sdf)
    {
        // Gazebo initialization
        steer_fl_joint_ = model->GetJoint("joint_left_wheel_1_steer_joint");
        steer_fr_joint_ = model->GetJoint("joint_right_wheel_1_steer_joint");
        wheel_rl_joint_ = model->GetJoint("joint_left_wheel_1_speed_joint");
        wheel_rr_joint_ = model->GetJoint("joint_right_wheel_1_speed_joint");
        // wheel_fl_joint_ = model->GetJoint("joint_left_wheel_2_speed_joint");
        // wheel_fr_joint_ = model->GetJoint("joint_right_wheel_2_speed_joint");

        assert(steer_fl_joint_);
        assert(steer_fr_joint_);
        assert(wheel_rl_joint_);
        assert(wheel_rr_joint_);
        // assert(wheel_fl_joint_);
        // assert(wheel_fr_joint_);

        if (sdf->HasElement("robotName"))
        {
            sdf::ParamPtr sdf_robot_name = sdf->GetElement("robotName")->GetValue();
            if (sdf_robot_name)
            {
                sdf_robot_name->Get(robot_name_);
            }
            else
            {
                robot_name_ = std::string("");
            }
        }
        else
        {
            robot_name_ = std::string("");
        }

        if (sdf->HasElement("maxSteerRad"))
        {
            sdf->GetElement("maxSteerRad")->GetValue()->Get(max_steer_rad_);
        }
        else
        {
            max_steer_rad_ = M_PI * 30 / 180;
        }

        if (sdf->HasElement("wheelbase"))
        {
            sdf->GetElement("wheelbase")->GetValue()->Get(wheelbase_);
        }
        else
        {
            wheelbase_ = 1.0;
        }

        if (sdf->HasElement("wheelRadius"))
        {
            sdf->GetElement("wheelRadius")->GetValue()->Get(wheel_radius_);
        }
        else
        {
            wheel_radius_ = 0.25;
        }

        if (sdf->HasElement("trackWidth"))
        {
            sdf->GetElement("trackWidth")->GetValue()->Get(track_width_);
        }
        else
        {
            track_width_ = 0.8;
        }

        if (sdf->HasElement("tfFreq"))
        {
            sdf->GetElement("tfFreq")->GetValue()->Get(tf_freq_);
        }
        else
        {
            tf_freq_ = 10.0;
        }

        if (sdf->HasElement("odomFrame"))
        {
            sdf->GetElement("odomFrame")->GetValue()->Get(odom_frame_);
        }
        else
        {
            odom_frame_ = "wheel_odom";
        }

        if (sdf->HasElement("pubTf"))
        {
            sdf->GetElement("pubTf")->GetValue()->Get(pub_tf_);
        }
        else
        {
            pub_tf_ = false;
        }

        base_frame_id_ = "base_footprint";
        publish_period_ = 1. / 100;

        update_connection_ = event::Events::ConnectWorldUpdateBegin(boost::bind(&TigraPlugin::OnUpdate, this, _1));

        mps2rpm = 60 / wheel_radius_ / (2 * M_PI);
        mps2rps = 1.0 / wheel_radius_;

        steer_fl_joint_->SetParam("fmax", 0, 99999.0);
        steer_fr_joint_->SetParam("fmax", 0, 99999.0);

        // ROS initialization
        n_ = ros::NodeHandle(robot_name_);

        sub_vel_cmd_ = n_.subscribe("cmd_vel", 1, &TigraPlugin::onCmdVel, this);

        /* Prepare publishers */
        double pose_cov_list[6] = {0.001, 0.001, 0.001, 0.001, 0.001, 0.03};
        double twist_cov_list[6] = {0.001, 0.001, 0.001, 0.001, 0.001, 0.03};

        // Setup odometry realtime publisher + odom message constant fields
        odom_pub_.reset(new realtime_tools::RealtimePublisher<nav_msgs::Odometry>(n_, "odom", 100));
        odom_pub_->msg_.header.frame_id = odom_frame_id_;
        odom_pub_->msg_.child_frame_id = base_frame_id_;
        odom_pub_->msg_.pose.pose.position.z = 0;
        // odom_pub_->msg_.pose.covariance = boost::assign::list_of
        //                                   (static_cast<double>(pose_cov_list[0])) (0)  (0)  (0)  (0)  (0)
        //                                   (0)  (static_cast<double>(pose_cov_list[1])) (0)  (0)  (0)  (0)
        //                                   (0)  (0)  (static_cast<double>(pose_cov_list[2])) (0)  (0)  (0)
        //                                   (0)  (0)  (0)  (static_cast<double>(pose_cov_list[3])) (0)  (0)
        //                                   (0)  (0)  (0)  (0)  (static_cast<double>(pose_cov_list[4])) (0)
        //                                   (0)  (0)  (0)  (0)  (0)  (static_cast<double>(pose_cov_list[5]));
        odom_pub_->msg_.twist.twist.linear.y = 0;
        odom_pub_->msg_.twist.twist.linear.z = 0;
        odom_pub_->msg_.twist.twist.angular.x = 0;
        odom_pub_->msg_.twist.twist.angular.y = 0;
        // odom_pub_->msg_.twist.covariance = boost::assign::list_of
        //                                    (static_cast<double>(twist_cov_list[0])) (0)  (0)  (0)  (0)  (0)
        //                                    (0)  (static_cast<double>(twist_cov_list[1])) (0)  (0)  (0)  (0)
        //                                    (0)  (0)  (static_cast<double>(twist_cov_list[2])) (0)  (0)  (0)
        //                                    (0)  (0)  (0)  (static_cast<double>(twist_cov_list[3])) (0)  (0)
        //                                    (0)  (0)  (0)  (0)  (static_cast<double>(twist_cov_list[4])) (0)
        //                                    (0)  (0)  (0)  (0)  (0)  (static_cast<double>(twist_cov_list[5]));
        tf_odom_pub_.reset(new realtime_tools::RealtimePublisher<tf::tfMessage>(n_, "/tf", 100));
        tf_odom_pub_->msg_.transforms.resize(1);
        tf_odom_pub_->msg_.transforms[0].transform.translation.z = 0.0;
        tf_odom_pub_->msg_.transforms[0].child_frame_id = base_frame_id_;
        tf_odom_pub_->msg_.transforms[0].header.frame_id = odom_frame_id_;

        cout << "TigraPlugin plugin loaded!" << endl;
    }

    void TigraPlugin::OnUpdate(const common::UpdateInfo &info)
    {
        if (last_update_time_ == common::Time(0))
        {
            last_update_time_ = info.simTime;
            return;
        }

        double time_step_ = (info.simTime - last_update_time_).Double();
        last_update_time_ = info.simTime;

        updateCurrentState();
        updateOdometry(time_step_);

        driveUpdate();
        steeringUpdate(time_step_);
    }

    void TigraPlugin::updateOdometry(double time_step)
    {
        const double linear = cur_virtual_speed_rps_ / mps2rps * time_step;
        const double angular = linear * tan(cur_virtual_steering_rad_) / wheelbase_;
        const double curvature_radius = wheelbase_ / cos(M_PI / 2.0 - cur_virtual_steering_rad_);

        linear_ = linear;
        angular_ = angular;

        if (fabs(curvature_radius) > 0.0001)
        {
            const double elapsed_distance = linear;
            const double elapsed_angle = elapsed_distance / curvature_radius;
            const double x_curvature = curvature_radius * sin(elapsed_angle);
            const double y_curvature = curvature_radius * (cos(elapsed_angle) - 1.0);
            const double wheel_heading = yaw_ + cur_virtual_steering_rad_;
            y_ += x_curvature * sin(wheel_heading) + y_curvature * cos(wheel_heading);
            x_ += x_curvature * cos(wheel_heading) - y_curvature * sin(wheel_heading);
            yaw_ += elapsed_angle;
        }

        /*** Update publishers ***/
        if (last_odom_update_time_ + publish_period_ > last_update_time_)
        {
            return;
        }

        last_odom_update_time_ += publish_period_;

        const geometry_msgs::Quaternion orientation(tf::createQuaternionMsgFromYaw(yaw_));

        if (odom_pub_->trylock())
        {
            odom_pub_->msg_.header.stamp = ros::Time(last_update_time_.Double());
            odom_pub_->msg_.pose.pose.position.x = x_ + wheelbase_ * (1.0 - cos(yaw_));
            odom_pub_->msg_.pose.pose.position.y = y_ - wheelbase_ * sin(yaw_);
            odom_pub_->msg_.pose.pose.orientation = orientation;
            odom_pub_->msg_.twist.twist.linear.x = 0;
            odom_pub_->msg_.twist.twist.angular.z = 0;
            odom_pub_->unlockAndPublish();
        }

        if (tf_odom_pub_->trylock())
        {
            geometry_msgs::TransformStamped &odom_frame = tf_odom_pub_->msg_.transforms[0];
            odom_frame.header.stamp = ros::Time(last_update_time_.Double());
            odom_frame.transform.translation.x = x_ + wheelbase_ * (1.0 - cos(yaw_));
            odom_frame.transform.translation.y = y_ - wheelbase_ * sin(yaw_);
            odom_frame.transform.rotation = orientation;
            tf_odom_pub_->unlockAndPublish();
        }
    }

    void TigraPlugin::driveUpdate()
    {
        double ref_rotation_speed_rps = target_speed_mps_ * mps2rps;

        double cur_right_rspeed_rps = wheel_rr_joint_->GetVelocity(0);
        double cur_left_rpeed_rps = wheel_rl_joint_->GetVelocity(0);

        double radius = wheelbase_ / tan(cur_virtual_steering_rad_);

        double ref_right_rspeed = ref_rotation_speed_rps * (1 + (0.5 * track_width_ / radius));
        double ref_left_rspeed = ref_rotation_speed_rps * (1 - (0.5 * track_width_ / radius));

        // ROS_INFO_STREAM( "Target speeds: " << ref_left_rspeed << " / " << ref_right_rspeed );

        double right_rspeed_error = ref_right_rspeed - cur_right_rspeed_rps;
        double left_rspeed_error = ref_left_rspeed - cur_left_rpeed_rps;

        wheel_rl_joint_->SetVelocity(0, ref_right_rspeed);
        wheel_rr_joint_->SetVelocity(0, ref_left_rspeed);
    }

    void TigraPlugin::updateCurrentState()
    {
        double t_cur_lsteer = tan(steer_fl_joint_->Position(0));
        double t_cur_rsteer = tan(steer_fr_joint_->Position(0));

        // std::atan(wheelbase_ * std::tan(steering_angle)/std::abs(wheelbase_ + it->lateral_deviation_ * std::tan(steering_angle)));

        double virt_lsteer = atan(wheelbase_ * t_cur_lsteer / (wheelbase_ + t_cur_lsteer * 0.5 * track_width_));
        double virt_rsteer = atan(wheelbase_ * t_cur_rsteer / (wheelbase_ - t_cur_rsteer * 0.5 * track_width_));

        // ROS_INFO_STREAM( "Steerings: " << virt_lsteer << " / " << virt_rsteer );

        cur_virtual_steering_rad_ = (virt_lsteer + virt_rsteer) / 2;

        cur_virtual_speed_rps_ = (wheel_rl_joint_->GetVelocity(0) + wheel_rr_joint_->GetVelocity(0)) / 2;

        // ROS_INFO_STREAM( "Estimated state: " << cur_virtual_steering_rad_ << " / " << cur_virtual_speed_rps_ );
    }

    void TigraPlugin::steeringUpdate(double time_step)
    {
        // Arbitrarily set maximum steering rate to 800 deg/s
        double max_inc = time_step * MAX_STEERING_RATE;

        // if ((target_angle_ - current_steering_angle_) > max_inc)
        // {
        //     current_steering_angle_ += max_inc;
        // }
        // else if ((target_angle_ - current_steering_angle_) < -max_inc)
        // {
        //     current_steering_angle_ -= max_inc;
        // }

        // ROS_INFO_STREAM( "Target steering: " << target_angle_ );

        // Compute Ackermann steering angles for each wheel
        double t_alph = tan(target_angle_);
        double left_steer = atan(wheelbase_ * t_alph / (wheelbase_ - 0.5 * track_width_ * t_alph));
        double right_steer = atan(wheelbase_ * t_alph / (wheelbase_ + 0.5 * track_width_ * t_alph));

#if GAZEBO_MAJOR_VERSION >= 9
        steer_fl_joint_->SetParam("vel", 0, STEER_P_RATE * (left_steer - steer_fl_joint_->Position(0)));
        steer_fr_joint_->SetParam("vel", 0, STEER_P_RATE * (right_steer - steer_fr_joint_->Position(0)));
#else
        steer_fl_joint_->SetParam("vel", 0, STEER_P_RATE * (left_steer - steer_fl_joint_->GetAngle(0).Radian()));
        steer_fr_joint_->SetParam("vel", 0, STEER_P_RATE * (right_steer - steer_fr_joint_->GetAngle(0).Radian()));
#endif
    }

    void TigraPlugin::onCmdVel(const geometry_msgs::Twist &command)
    {
        target_angle_ = command.angular.z * TIGRA_STEERING_RATIO;
        if (target_angle_ > max_steer_rad_)
        {
            target_angle_ = max_steer_rad_;
        }
        else if (target_angle_ < -max_steer_rad_)
        {
            target_angle_ = -max_steer_rad_;
        }

        target_speed_mps_ = command.linear.x;
    }

    void TigraPlugin::Reset()
    {
    }

    TigraPlugin::~TigraPlugin()
    {
        n_.shutdown();
    }

} // namespace gazebo
