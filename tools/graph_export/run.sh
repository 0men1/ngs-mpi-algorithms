#!/bin/bash

config_file=""
output_file=""

Help()
{
   echo "Graph generation and enrichment wrapper."
   echo
   echo "Usage: $0 -c <config_file> -o <output.json>"
   echo
   echo "Required Options:"
   echo "  -c     Path to NGS config file"
   echo "  -o     Path to output graph.json file"
   echo "  -h     Show this help message"
   echo
   echo "Example:"
   echo "  $0 -c configs/example.conf -o outputs/graph.json"
}

while getopts ":c:o:h:" option; do
    case $option in 
    h) 
        Help
        exit;;
    c)
        config_file=$OPTARG;;
    o)
        output_file=$OPTARG;;
    \?)
        echo "Error: Invalid option"
        Help
        exit 1;;
    esac
done

if [[ -z "$config_file" || -z "$output_file" ]]; then
    echo "Error: Missing required arguments."
    Help
    exit 1
fi

if [ ! -f "$config_file" ]; then
    echo "Error: Config file not found: $config_file"
    exit 1
fi

# Extract variables from config using awk
seed=$(awk -F '=' '/^[[:space:]]*seed[[:space:]]*=/ {gsub(/[^0-9]/, "", $2); print $2; exit}' "$config_file")
target_dir=$(awk -F '=' '/^[[:space:]]*outputDirectory[[:space:]]*=/ {gsub(/[" \r]/, "", $2); print $2; exit}' "$config_file")
file_name=$(awk -F '=' '/^[[:space:]]*fileName[[:space:]]*=/ {gsub(/[" \r]/, "", $2); print $2; exit}' "$config_file")

if [[ -z "$seed" || -z "$target_dir" || -z "$file_name" ]]; then
    echo "Error: Config file missing required fields (seed, outputDirectory, or fileName)"
    exit 1
fi

# Strip .ngs extension for the java execution argument
raw_ngs_base="${file_name%.*}"
target_dir="${target_dir%/}"

java -Xms2G -Xmx30G -Dconfig.file="$config_file" -DNGSimulator.outputDirectory="$target_dir" -jar NetGameSim/target/scala-3.2.2/netmodelsim.jar "$raw_ngs_base"

input_for_enrichment="${target_dir}/${file_name}"

python tools/graph_export/enrichment.py "$input_for_enrichment" "$output_file" "$seed"
