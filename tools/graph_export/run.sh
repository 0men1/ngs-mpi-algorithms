#!/bin/bash

config_file=configs/defconfig.conf
output_file=outputs/graph.json

Help()
{
   # Display Help
   echo "Add description of the script functions here."
   echo
   echo "Syntax: ./tools/graph_export/run.sh [-c|o|h]"
   echo "options:"
   echo "c     Sets the path to config file for NGS."
   echo "o     Sets the path to output graph.json"
   echo "h     Prints Help."
   echo
}


while getopts ":c:o:h:" option; do
	case $option in 
		h) # help
			Help
			exit;;
		c)
			config_file=$OPTARG;;
		o)
			output_file=$OPTARG;;
		/?)
			echo "Error: Invalid option"
			exit;;
	esac
done


# Extract seed from config file
SEED=$(grep -E '^\s*seed\s*=' "$config_file" | sed 's/.*=\s*//' | tr -d ' ')

if [ -z "$SEED" ]; then
    echo "Warning: Could not extract seed from config, using default 0"
    SEED=0
fi

# Generate the graph using NGS
ngs_graph_file="raw_ngs_graph"

java -Xms2G -Xmx30G -Dconfig.file=$config_file -jar NetGameSim/target/scala-3.2.2/netmodelsim.jar $ngs_graph_file;

# Run graph enrichment with seed
python tools/graph_export/enrichment.py "outputs/${ngs_graph_file}.ngs" "$output_file" "$SEED"
