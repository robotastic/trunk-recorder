import socket
import struct
import pyaudio
import time

TGID_in_stream = False
TGID_to_play = 58916
UDP_PORT = 9123
AUDIO_OUTPUT_DEVICE_INDEX = 2

# Set up a UDP server
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
UDPSock.settimeout(.2)

listen_addr = ("",UDP_PORT)
UDPSock.bind(listen_addr)

p = pyaudio.PyAudio()
chunk = int(3456/4)
FORMAT = pyaudio.paFloat32
CHANNELS = 1
RATE = 8000
stream = p.open(format = FORMAT,
				channels = CHANNELS,
				rate = RATE,
				input = False,
				output = True,
				frames_per_buffer = chunk,
				output_device_index = AUDIO_OUTPUT_DEVICE_INDEX,)

while True:
		try:
			data,addr = UDPSock.recvfrom(2048*4)
			if TGID_in_stream:
				tgid = int.from_bytes(data[0:4],"little")
				print(tgid," ",len(data))
				if (tgid == TGID_to_play or TGID_to_play == 0):
					stream.write(data[4:])
			else:
				stream.write(data)
				print(len(data))
		except socket.timeout:
			pass
