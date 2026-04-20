import sys
import json
from collections import deque
import random


def checkConnected(adj_list: dict) -> bool:
    visited=set()
    start_node = next(iter(adj_list))
    q=deque()
    q.append(start_node)
    visited.add(start_node)

    while q:
        nodeId = q.popleft()
        for edges in adj_list[nodeId]:
            neighbor = edges["v"]
            if neighbor not in visited:
                visited.add(neighbor)
                q.append(neighbor)

    return len(visited) == len(adj_list)


def generateGraph(ngs_file: str, output_file: str, seed: int = 0):
    print(f"Generating graph. NGS file: {ngs_file}, seed: {seed}")
    random.seed(seed)

    adj_list = {}

    with open(ngs_file, 'r') as f:
        nodes_line = f.readline().strip()
        edges_line = f.readline().strip()

    nodes_data = json.loads(nodes_line)
    edges_data = json.loads(edges_line)

    for node in nodes_data:
        adj_list[node["id"]] = []

    for edge in edges_data:
        u = edge["fromNode"]["id"]
        v = edge["toNode"]["id"]

        w = random.uniform(1.0, 20.0)

        adj_list[u].append({"v": v, "w": w})
        adj_list[v].append({"v": u, "w": w})

    if not checkConnected(adj_list):
        sys.exit("The generated graph is not connected. Failed to output graph JSON\n")

    output_data = {
        "metadata": {
            "num_nodes": len(adj_list),
            "seed": seed
        },
        "adjacency_list": adj_list
    }

    with open(output_file, 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"Successfully enriched graph. Exported to {output_file}")

def main():
    print("Running enrichment")
    if len(sys.argv) < 3:
        sys.exit("Usage: python enrich.py <input.ngs> <output.json> [seed]")

    ngs_file: str = sys.argv[1]
    output_file: str = sys.argv[2]
    seed: int = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    generateGraph(ngs_file, output_file, seed)

if __name__ == "__main__":
    main()
