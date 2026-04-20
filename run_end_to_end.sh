#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

usage() {
    echo "End-to-End Graph to Algorithm Run Script"
    echo
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --config CONFIG      NGS config file (default: configs/example.conf)"
    echo "  --graph FILE         Input graph.json file (skip generation)"
    echo "  --part FILE          Input partition.json file (skip partitioning)"
    echo "  --ranks NUM          Number of MPI ranks (default: 2)"
    echo "  --source NODE        Source node for Dijkstra (default: 0)"
    echo "  --rounds NUM         Number of rounds for Leader Election (default: 10)"
    echo "  --seed NUM           Seed for weight generation (default: 50)"
    echo "  --output-dir DIR     Output directory (default: outputs/)"
    echo "  --skip-build         Skip building the project"
    echo "  --step-by-step       Run each step separately and wait"
    echo "  --help               Show this help message"
    echo
    echo "Examples:"
    echo "  $0                           # Run full pipeline with defaults"
    echo "  $0 --graph mygraph.json     # Use existing graph, just run algorithms"
    echo "  $0 --step-by-step           # Run step-by-step with explanations"
}

generate_graph() {
    local config="$1"
    local output_dir="$2"
    local seed="$3"
    local graph_output="$4"
    
    echo "=========================================="
    echo "Step 1: Generate graph with NetGameSim"
    echo "=========================================="
    echo "Config: $config"
    echo "Output: $graph_output"
    echo "Seed: $seed"
    echo
    
    mkdir -p "$output_dir"
    
    ./tools/graph_export/run.sh -c "$config" -o "$graph_output" -d "$output_dir" -s "$seed"
    
    echo
    echo "Graph generated successfully!"
    echo "Output file: $graph_output"
    echo
}

partition_graph() {
    local graph_file="$1"
    local ranks="$2"
    local part_file="$3"
    
    echo "=========================================="
    echo "Step 2: Partition graph across MPI ranks"
    echo "=========================================="
    echo "Graph: $graph_file"
    echo "Ranks: $ranks"
    echo "Output: $part_file"
    echo
    
    ./tools/partition/run.sh -g "$graph_file" -r "$ranks" -o "$part_file"
    
    echo
    echo "Partition created successfully!"
    echo "Output file: $part_file"
    echo
}

build_project() {
    local skip="$1"
    
    if [[ "$skip" == "true" ]]; then
        echo "Skipping build..."
        return
    fi
    
    echo "=========================================="
    echo "Step 3: Build the project"
    echo "=========================================="
    echo
    
    make clean_build
    
    echo
    echo "Build completed successfully!"
    echo
}

run_algorithms() {
    local graph_file="$1"
    local part_file="$2"
    local ranks="$3"
    local source_node="$4"
    local rounds="$5"
    
    echo "=========================================="
    echo "Step 4: Run algorithms"
    echo "=========================================="
    echo "Graph: $graph_file"
    echo "Partition: $part_file"
    echo "Ranks: $ranks"
    echo
    
    echo "--- Running Dijkstra (source node $source_node) ---"
    mpirun -n "$ranks" ./build/ngs_mpi --graph "$graph_file" --part "$part_file" --algo dijkstra --source "$source_node"
    
    echo
    echo "--- Running Leader Election ($rounds rounds) ---"
    mpirun -n "$ranks" ./build/ngs_mpi --graph "$graph_file" --part "$part_file" --algo leader --rounds "$rounds"
    
    echo
    echo "=========================================="
    echo "End-to-End run completed!"
    echo "=========================================="
}

step_by_step() {
    local config="$1"
    local ranks="$2"
    local source_node="$3"
    local rounds="$4"
    local seed="$5"
    local output_dir="$6"
    
    local graph_file="$output_dir/example_graph.json"
    local part_file="$output_dir/example_part.json"
    
    echo "Running step-by-step mode. Press Enter after each step..."
    
    echo
    echo "=== Step 1: Generate Graph ==="
    read -p "Press Enter to generate graph with NetGameSim..."
    generate_graph "$config" "$output_dir" "$seed" "$graph_file"
    
    echo "=== Step 2: Partition Graph ==="
    read -p "Press Enter to partition the graph across $ranks ranks..."
    partition_graph "$graph_file" "$ranks" "$part_file"
    
    echo "=== Step 3: Build Project ==="
    read -p "Press Enter to build the project..."
    build_project "false"
    
    echo "=== Step 4: Run Algorithms ==="
    read -p "Press Enter to run the algorithms..."
    run_algorithms "$graph_file" "$part_file" "$ranks" "$source_node" "$rounds"
}

config_file="configs/example.conf"
ranks=2
source_node=0
rounds=10
seed=50
output_dir="outputs"
skip_build="false"
mode="auto"
graph_input=""
part_input=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --config)
            config_file="$2"
            shift 2
            ;;
        --graph)
            graph_input="$2"
            shift 2
            ;;
        --part)
            part_input="$2"
            shift 2
            ;;
        --ranks)
            ranks="$2"
            shift 2
            ;;
        --source)
            source_node="$2"
            shift 2
            ;;
        --rounds)
            rounds="$2"
            shift 2
            ;;
        --seed)
            seed="$2"
            shift 2
            ;;
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --skip-build)
            skip_build="true"
            shift
            ;;
        --step-by-step)
            mode="step"
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ ! -f "NetGameSim/target/scala-3.2.2/netmodelsim.jar" ]]; then
    echo "Error: NetGameSim JAR not found!"
    echo "Please initialize the submodule:"
    echo "  git submodule update --init --recursive"
    echo "Or build NetGameSim from the NetGameSim directory."
    exit 1
fi

if [[ "$mode" == "step" ]]; then
    step_by_step "$config_file" "$ranks" "$source_node" "$rounds" "$seed" "$output_dir"
    exit 0
fi

graph_file="$graph_input"
part_file="$part_input"

if [[ -z "$graph_file" ]]; then
    graph_file="$output_dir/example_graph.json"
    generate_graph "$config_file" "$output_dir" "$seed" "$graph_file"
fi

if [[ -z "$part_file" ]]; then
    part_file="$output_dir/example_part.json"
    partition_graph "$graph_file" "$ranks" "$part_file"
fi

build_project "$skip_build"

run_algorithms "$graph_file" "$part_file" "$ranks" "$source_node" "$rounds"