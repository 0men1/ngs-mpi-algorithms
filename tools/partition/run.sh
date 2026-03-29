#!/bin/bash

#./tools/partition/run.sh outputs/graph.json --ranks 8 --out outputs/part.json

Help()
{
   # Display Help
   echo "Add description of the script functions here."
   echo
   echo "Syntax: ./tools/partition/run.sh [-g|h|r|o]"
   echo "options:"
   echo "g     Sets the path to graph.json."
   echo "r     Sets the number of ranks"
   echo "o     Sets the path to output part.json"
   echo "h     Prints Help."
   echo
}


graph_json=outputs/graph.json
ranks=8
output=outputs/part.json

while getopts ":h:g:r:o" option; do
	case $option in 
		h) # help
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
			exit;;
	esac
done


if [ -f $graph_json ]; then
	python tools/partition/partition.py $graph_json $ranks $output
fi
