import socket
import struct
import pyaudio
import time
import json

TGID_in_stream = False         #When set to True, we expect a 4 byte long int with the TGID prior to the audio in each packet
JSON_in_stream = False         #When set to True, we expect a 4 byte long int with the length of the JSON metadata followed by the JSON metadata prior to the audio
TGID_to_play = 58917           #When TGID_in_stream is set to True, we'll only play audio if the received TGID matches this value
UDP_PORT = 9123                #UDP port to listen on
AUDIO_OUTPUT_DEVICE_INDEX = 4  #Audio device to play received audio on
DEFAULT_AUDIO_SAMPLE_RATE = 8000 

# Set up a UDP server
UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
UDPSock.settimeout(.2)

listen_addr = ("",UDP_PORT)
UDPSock.bind(listen_addr)

p = pyaudio.PyAudio()
print(p.get_host_api_info_by_index(0))

def create_audio_stream(rate=DEFAULT_AUDIO_SAMPLE_RATE):
	stream = p.open(format = pyaudio.paInt16,
				channels = 1,
				rate = rate,
				input = False,
				output = True,
				frames_per_buffer = int(3456/2),
				output_device_index = AUDIO_OUTPUT_DEVICE_INDEX,)
	return stream

streams = {}
if TGID_to_play != 0:
	streams[TGID_to_play] = create_audio_stream()

while True:
		try:
			data,addr = UDPSock.recvfrom(2048*2)
			audio_start_byte = 0
			playit = False
			this_audio_sample_rate = DEFAULT_AUDIO_SAMPLE_RATE
			if JSON_in_stream:
				json_length = int.from_bytes(data[0:4],"little")
				audio_start_byte = json_length + 4
				json_bytes = data[4:json_length+4]
				json_string = json_bytes.decode('utf8')
				# Load the JSON to a Python list & dump it back out as formatted JSON
				json_data = json.loads(json_string)
				tgid = json_data['talkgroup']
				this_audio_sample_rate = json_data['audio_sample_rate']
				if tgid == TGID_to_play or TGID_to_play == 0:
					playit = True
					#print(json.dumps(json_data, indent=4, sort_keys=True))
			elif TGID_in_stream:
				tgid = int.from_bytes(data[0:4],"little")
				print(tgid," ",len(data))
				if (tgid == TGID_to_play or TGID_to_play == 0):
					playit = True
					audio_start_byte = 4
			else:
				tgid = TGID_to_play
				playit = True
			if playit == True:
				#stream.write(data[4:])
				if tgid not in streams:
					streams[tgid] = create_audio_stream(rate=this_audio_sample_rate)
				streams[tgid].write(data[audio_start_byte:])
				print(json.dumps(json_data, indent=4, sort_keys=True))
				#print(data)
				print("received",len(data),"bytes and writing",len(data)-audio_start_byte,"audio bytes")
		except socket.timeout:
			pass
