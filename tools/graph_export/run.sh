#!/bin/bash

config_file=${1:-configs/defconfig.conf}
output_file=${2:-outputs/graph.json}

# Generate the graph using NGS
ngs_graph_file="raw_ngs_graph"


if [[ ! -f "outputs/${ngs_graph_file}.ngs" || ! -f "outputs/${ngs_graph_file}.ngs.dot" ]]; then
	java -Xms2G -Xmx30G -Dconfig.file=$config_file -jar NetGameSim/target/scala-3.2.2/netmodelsim.jar $ngs_graph_file;
fi


# Run graph enrichment

python tools/graph_export/enrichment.py "outputs/${ngs_graph_file}.ngs" "$output_file"
