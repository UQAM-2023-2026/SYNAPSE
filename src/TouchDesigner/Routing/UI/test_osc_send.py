
import argparse
from pythonosc import udp_client

def send_test_message(ip, port):
    print(f"Sending OSC message to {ip}:{port}")
    client = udp_client.SimpleUDPClient(ip, port)
    client.send_message("/test/dashboard", [1.23, "hello", True])
    print("Message sent.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="127.0.0.1", help="The ip of the OSC server")
    parser.add_argument("--port", type=int, default=6970, help="The port the OSC server is listening on")
    args = parser.parse_args()

    send_test_message(args.ip, args.port)
