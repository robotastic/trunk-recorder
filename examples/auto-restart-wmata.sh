#!/bin/bash

RECORDER490="./recorder --config=config-wmata-490.json"
RECORDER496="./recorder --config=config-wmata-496.json"

COUNTER=0
until  1; do
	echo "Starting at 490MHz"
	$RECORDER490
	echo "Server $RECORDER crashed with exit code $?. Try $COUNTER.  Respawning.." >&2
	sleep 5
	echo "Starting at 490MHz"
	$RECORDER496
	echo "Server $RECORDER crashed with exit code $?. Try $COUNTER.  Respawning.." >&2
	sleep 5

	let COUNTER+=1
done