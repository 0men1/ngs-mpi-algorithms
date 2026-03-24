import sys
import json

def generateGraph(ngs_file: str, output_file: str):
    print(f"Generating graph. NGS file: {ngs_file}")

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
        w = edge.get("cost", 0.0)

        adj_list[u].append({"v": v, "w": w})

    output_data = {
        "metadata": {
            "num_nodes": len(adj_list)
        },
        "adjacency_list": adj_list
    }

    with open(output_file, 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"Successfully enriched graph. Exported to {output_file}")

def main():
    print("Running enrichment")
    if len(sys.argv) != 3:
        sys.exit("Usage: python enrich.py <input.ngs> <output.json>")

    ngs_file: str = sys.argv[1]
    output_file: str = sys.argv[2]
    generateGraph(ngs_file, output_file)

if __name__ == "__main__":
    main()
