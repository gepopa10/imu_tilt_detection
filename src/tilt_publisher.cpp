#include "imu_tilt_detection/msg/tilt_status.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include <cmath>
#include <deque>

class TiltPublisher : public rclcpp::Node
{
public:
    TiltPublisher() : Node("tilt_publisher")
    {
        publisher_ = this->create_publisher<imu_tilt_detection::msg::TiltStatus>("/tilt/status", 10);
        subscription_ = this->create_subscription<sensor_msgs::msg::Imu>("/imu/data_raw", 10, std::bind(&TiltPublisher::publish_message, this, std::placeholders::_1));
    }

private:
    void publish_message(const sensor_msgs::msg::Imu & msg)
    {
    	const auto x = msg.linear_acceleration.x;
    	const auto y = msg.linear_acceleration.y;
    	const auto z = msg.linear_acceleration.z;
    	
    	const auto roll = atan2(y, sqrt(pow(x,2) + pow(z,2))) * (180 / M_PI); //around y axis
    	const auto pitch = atan2(x, sqrt(pow(y,2) + pow(z,2))) * (180 / M_PI); //around x axis
    	
        auto message = imu_tilt_detection::msg::TiltStatus();
        message.header = msg.header;
        message.roll = roll;
        message.pitch = pitch;
        
        roll > 15 || pitch > 15 ? message.tilted = true : message.tilted = false;
        
        if(_tilted_values.size() +1 > _nb_values_for_mean){
            _tilted_values.pop_front();
            _tilted_values.push_back(message.tilted);
        }else{
            _tilted_values.push_back(message.tilted);
        }
        
        double sum = std::accumulate(_tilted_values.begin(), _tilted_values.end(), 0.0);
        
        if (sum > _tilted_values.size() / 2.0) {
            message.tilted = true;
        }else{
            message.tilted = false;
        }
        
        RCLCPP_INFO(this->get_logger(), "Publishing: '%s' sum: '%f'", message.tilted ? "True" : "False", sum);
        publisher_->publish(message);
    }

    rclcpp::Publisher<imu_tilt_detection::msg::TiltStatus>::SharedPtr publisher_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr subscription_; 
    const double pi = M_PI;
    std::deque<bool> _tilted_values;
    const size_t _nb_values_for_mean = 100;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TiltPublisher>());
    rclcpp::shutdown();
    return 0;
}
