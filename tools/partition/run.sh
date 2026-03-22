#!/bin/bash

#./tools/partition/run.sh outputs/graph.json --ranks 8 --out outputs/part.json

graph_json=${1:-outputs/graph.json}
ranks=${2:-8}
output=${3:-outputs/part.json}


if [ -f $graph_json ]; then
	python tools/partition/partition.py $graph_json $ranks $output
fi
