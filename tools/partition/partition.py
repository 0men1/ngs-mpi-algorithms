import sys
import json


def partitionGraph(graph_file: str, ranks: int, output_file: str):
    num_nodes: int=0
    with open(graph_file, 'r') as f:
        graph_data = json.load(f)
        num_nodes = int(graph_data["metadata"]["num_nodes"])

    partition: dict = {}
    for nodeId in range(num_nodes):
        groupId = int((nodeId * ranks) / num_nodes)
        partition[nodeId] = groupId

    with open(output_file, 'w') as f:
        json.dump(partition, f, indent=2)

    print(f"number of nodes: {num_nodes}")
    print("Successfully partitioned graph nodes into MPI ranks")

def main():
    if len(sys.argv) != 4:
        sys.exit("Usage: python partition.py <graph.json> <ranks> <output.json>")

    graph_file: str=sys.argv[1]
    ranks: int=int(sys.argv[2])
    output_file: str=sys.argv[3]

    partitionGraph(graph_file, ranks, output_file)

    print("Running partition")

if __name__ == "__main__":
    main()
