#!/usr/bin/env python

import asyncio
import websockets
import socket
import json
import functools
from threading import Thread
import rospy
import numpy as np

from sensor_msgs.msg import Imu

from tf.transformations import quaternion_from_euler


def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(("10.255.255.255", 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = "127.0.0.1"
    finally:
        s.close()
    return IP


class ImuState:
    def __init__(self, frame_id, accel_cov, gyro_cov, orient_cov):
        self.pub = rospy.Publisher("imu", Imu, queue_size=10)
        self.state = Imu()
        self.state_initial_update = [False] * 3

        self.state.linear_acceleration_covariance = accel_cov
        self.state.angular_velocity_covariance = gyro_cov
        self.state.orientation_covariance = orient_cov
        self.state.header.frame_id = frame_id
        
    def update_orient(self, x, y, z):
        q = quaternion_from_euler(x, y, z)

        self.state.orientation.x = q[0]
        self.state.orientation.y = q[1]
        self.state.orientation.z = q[2]
        self.state.orientation.w = q[3]

        self.state_initial_update[0] = True
        self.publish()

    def update_accel(self, x, y, z):
        self.state.linear_acceleration.x = x
        self.state.linear_acceleration.y = y
        self.state.linear_acceleration.z = z

        self.state_initial_update[1] = True
        self.publish()

    def update_gyro(self, x, y, z):
        self.state.angular_velocity.x = x
        self.state.angular_velocity.y = y
        self.state.angular_velocity.z = z

        self.state_initial_update[2] = True
        self.publish()

    def publish(self):
        if not all(self.state_initial_update):
            return

        self.state.header.stamp = rospy.Time.now()

        self.pub.publish(self.state)

async def ws_handler(websocket, path, extra_argument):
    imu_state = extra_argument["imu_state"]

    async for message in websocket:
        if path == "//sensors":
            message = json.loads(message)

            try:
                # Sample: {"timestamp":1624386177562,"sensors":[
                        # {"name":"Accelerometer","value0":0.09303284,"value1":-0.3250122,"value2":9.86026},
                        # {"name":"Gyroscope","value0":3.6621094E-4,"value1":2.746582E-4,"value2":-0.0011138916},
                        # {"name":"Orientation","value0":209.60489,"value1":1.9141718,"value2":0.57536465},
                        # {"name":"Location","value0":null,"value1":null,"value2":null}]}
                sensors_data = message.get("sensors")
                if sensors_data is None:
                    continue

                # X on phone goes right from screen
                # Y goes up
                def orientation_handler(data):
                    z = data["value0"]
                    x = data["value1"]
                    y = data["value2"]
                    imu_state.update_orient(
                        np.deg2rad(x), 
                        np.deg2rad(y), 
                        np.deg2rad(z) 
                    )

                def gyroscope_handler(data):
                    x = data["value0"]
                    y = data["value1"]
                    z = data["value2"]
                    imu_state.update_gyro(x, y, z)

                def accelerometer_handler(data):
                    x = data["value0"]
                    y = data["value1"]
                    z = data["value2"]
                    imu_state.update_accel(x, y, z)

                name_2_obj = {
                    "Accelerometer": accelerometer_handler,
                    "Orientation": orientation_handler,
                    "Gyroscope": gyroscope_handler,
                }

                for msg in sensors_data:
                    handler = name_2_obj.get(msg["name"])
                    if handler is None:
                        continue
                    handler(msg)
            except Exception as e:
                rospy.logerr(f'Exception: {e}')


# async def _server(stop, state):
#     bound_handler = functools.partial(ws_handler, extra_argument=state)
#     async with websockets.serve(bound_handler, "0.0.0.0", 5000):
#         await stop

# ws_server = await websockets.serve(hello, "localhost", 8765)
# await ws_server.server.serve_forever()


def start_server(loop, server):
    loop.run_until_complete(server)
    loop.run_forever()


def main():
    hostname = socket.gethostname()
    IPAddr = get_ip()
    print("Your Computer Name is: " + hostname)
    print("Your Computer IP Address is: " + IPAddr)

    rospy.init_node("phone_data_sender")

    
    # ROS params
    param_port = rospy.get_param('~port', 5000)

    param_accel_cov = rospy.get_param('~accel_cov', [0.01,0,0, 0,0.01,0, 0,0,0.01])
    param_gyro_cov = rospy.get_param('~gyro_cov', [0.0025,0,0, 0,0.0025,0, 0,0,0.0025])
    param_orient_cov = rospy.get_param('~orient_cov', [0.001,0,0, 0,0.001,0, 0,0,0.001])

    imu_state = ImuState(frame_id="imu", accel_cov=param_accel_cov, gyro_cov=param_gyro_cov, orient_cov=param_orient_cov)

    bound_handler = functools.partial(
        ws_handler, extra_argument={"imu_state": imu_state}
    )
    
    loop = asyncio.new_event_loop()
    # stop = loop.create_future()
    # loop.run_until_complete(_server(stop, internal_state))
    _server = websockets.serve(bound_handler, "0.0.0.0", param_port, loop=loop)

    # start_server(loop, _server)
    t = Thread(target=start_server, args=(loop, _server))
    t.start()

    # asyncio.get_event_loop().run_until_complete(
    #     websockets.serve(bound_handler, "0.0.0.0", 5000)
    # )
    # asyncio.get_event_loop().run_forever()

    try:
        rospy.spin()
    finally:
        loop.stop()
        # stop.set_result()


if __name__ == "__main__":
    main()
