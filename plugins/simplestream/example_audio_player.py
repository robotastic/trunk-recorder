import socket
import struct
import pyaudio
import time

TGID_in_stream = False         #When set to True, we expect a 4 byte long int with the TGID prior to the audio in each packet
TGID_to_play = 58917           #When TGID_in_stream is set to True, we'll only play audio if the received TGID matches this value
UDP_PORT = 9123                #UDP port to listen on
AUDIO_OUTPUT_DEVICE_INDEX = 2  #Audio device to play received audio on

# Set up a UDP server
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
UDPSock.settimeout(.2)

listen_addr = ("",UDP_PORT)
UDPSock.bind(listen_addr)

p = pyaudio.PyAudio()
chunk = int(3456/2)
FORMAT = pyaudio.paInt16
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
			data,addr = UDPSock.recvfrom(2048*2)
			if TGID_in_stream:
				tgid = int.from_bytes(data[0:4],"little")
				print(tgid," ",len(data))
				if (tgid == TGID_to_play or TGID_to_play == 0):
					stream.write(data[4:])
			else:
				stream.write(data)
				print(data)
		except socket.timeout:
			pass
