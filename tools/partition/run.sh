#!/bin/bash

Help()
{
   echo "Graph Partitioning Tool"
   echo
   echo "Usage: $0 -g <graph.json> -r <ranks> -o <part.json>"
   echo
   echo "Required Options:"
   echo "  -g     Path to input graph.json file"
   echo "  -r     Number of MPI ranks"
   echo "  -o     Path to output partition.json file"
   echo "  -h     Show this help message"
   echo
   echo "Example:"
   echo "  $0 -g outputs/graph.json -r 4 -o outputs/part.json"
}

graph_json=""
ranks=""
output=""

while getopts ":h:g:r:o:" option; do
	case $option in 
		h)
			Help
			exit;;
		g)
			graph_json=$OPTARG;;
		r)
			ranks=$OPTARG;;
		o)
			output=$OPTARG;;
		/?)
			echo "Error: Invalid option"
			Help
			exit 1;;
	esac
done

if [[ -z "$graph_json" || -z "$ranks" || -z "$output" ]]; then
    echo "Error: Missing required arguments."
    Help
    exit 1
fi

if [ ! -f "$graph_json" ]; then
    echo "Error: Graph file not found: $graph_json"
    exit 1
fi

python tools/partition/partition.py "$graph_json" "$ranks" "$output"
