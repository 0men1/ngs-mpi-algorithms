#!/bin/bash

config_file=""
ngs_graph_path=""
output_file="outputs/graph.json"

Help()
{
   echo "Graph generation and enrichment wrapper."
   echo
   echo "Syntax: ./tools/graph_export/run.sh [-c config_file | -g ngs_graph_path] [-o output_file] [-h]"
   echo "options:"
   echo "c     Sets the path to config file for NGS (runs generation)."
   echo "g     Sets the path to an existing NGS graph file (skips generation)."
   echo "o     Sets the path to output graph.json (default: outputs/graph.json)"
   echo "s     Sets the seed to generate the weights"
   echo "h     Prints Help."
   echo
}

while getopts ":c:g:o:s:h:" option; do
	case $option in 
	h) 
		Help
		exit;;
	g)
		ngs_graph_path=$OPTARG;;
	c)
		config_file=$OPTARG;;
	o)
		output_file=$OPTARG;;
	s)
		seed=$OPTARG;;
	\?)
		echo "Error: Invalid option"
		exit 1;;
    esac
done

if [[ -z "$config_file" && -z "$ngs_graph_path" ]]; then
	echo "Error: You must provide either a config file (-c) or an existing NGS graph file (-g)."
	Help
	exit 1
fi

if [[ -n "$config_file" && -n "$ngs_graph_path" ]]; then
	echo "Error: Options -c and -g are mutually exclusive."
	exit 1
fi

if [[ -z "$seed" ]]; then
	echo "Seed not provided. Using default value "67""
	seed=67
fi

if [[ -n "$config_file" ]]; then
    echo "Config file provided. Generating graph using NGS..."
    
    # Define intermediate output name for NGS
    raw_ngs_base="raw_ngs_graph"
    
    java -Xms2G -Xmx30G -Dconfig.file="$config_file" -jar NetGameSim/target/scala-3.2.2/netmodelsim.jar "$raw_ngs_base"
    
    # NetGameSim appends .ngs to the output file name
    input_for_enrichment="outputs/${raw_ngs_base}.ngs"
else
    echo "NGS graph file provided. Skipping NGS generation..."
    input_for_enrichment="$ngs_graph_path"
fi

echo "Running enrichment..."
python tools/graph_export/enrichment.py "$input_for_enrichment" "$output_file" "$seed"
