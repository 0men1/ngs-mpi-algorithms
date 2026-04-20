#!/bin/bash

config_file=""
output_file="outputs/graph.json"

Help()
{
   echo "Graph generation and enrichment wrapper."
   echo "Syntax: ./tools/graph_export/run.sh [-c config_file | -g ngs_graph_path] [-o output_file] [-h]"
   echo "options:"
   echo "c     Sets the path to config file for NGS (runs generation)."
   echo "o     Sets the path to output graph.json (default: outputs/graph.json)"
   echo "h     Prints Help."
}

while getopts ":c:g:o:h:" option; do
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
        exit 1;;
    esac
done

if [[ -n "$config_file" ]]; then
    # Extract variables from config using awk
    seed=$(awk -F '=' '/^[[:space:]]*seed[[:space:]]*=/ {gsub(/[^0-9]/, "", $2); print $2; exit}' "$config_file")
    target_dir=$(awk -F '=' '/^[[:space:]]*outputDirectory[[:space:]]*=/ {gsub(/[" \r]/, "", $2); print $2; exit}' "$config_file")
    file_name=$(awk -F '=' '/^[[:space:]]*fileName[[:space:]]*=/ {gsub(/[" \r]/, "", $2); print $2; exit}' "$config_file")

    # Apply defaults if extraction fails
    [[ -z "$seed" ]] && seed=67
    [[ -z "$target_dir" ]] && target_dir="outputs"
    [[ -z "$file_name" ]] && file_name="raw_ngs_graph.ngs"

    # Strip .ngs extension for the java execution argument
    raw_ngs_base="${file_name%.*}"
    target_dir="${target_dir%/}"

    java -Xms2G -Xmx30G -Dconfig.file="$config_file" -DNGSimulator.outputDirectory="$target_dir" -jar NetGameSim/target/scala-3.2.2/netmodelsim.jar "$raw_ngs_base"
    
    input_for_enrichment="${target_dir}/${file_name}"
fi

python tools/graph_export/enrichment.py "$input_for_enrichment" "$output_file" "$seed"
