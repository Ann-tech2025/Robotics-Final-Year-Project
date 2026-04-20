#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry

class OdomBridge(Node):
    def __init__(self):
        super().__init__('odom_bridge')
        # Subscribe to your filtered odometry
        self.sub = self.create_subscription(
            Odometry,
            'odometry/filtered',   # your existing topic
            self.callback,
            10
        )
        # Publish on standard /odom for SLAM
        self.pub = self.create_publisher(Odometry, 'odom', 10)

    def callback(self, msg):
        # Fix the frame IDs for SLAM
        msg.header.frame_id = "odom"
        msg.child_frame_id = "base_link"
        self.pub.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = OdomBridge()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
