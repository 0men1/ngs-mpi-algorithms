import sys
import re
import json



def generateGraph(dotFile: str, output_file: str):
    print(f"generating graph. dotfile: {dotFile}")

    # adjlist: integer_node_id -> list of tuple(dest (float), weight(float))
    adj_list = {}
    pattern=re.compile(r'"(\d+)"\s*->\s*"(\d+)"\s*\["weight"="([0-9.]+)"\]')

    with open(dotFile, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                u=float(match.group(1))
                v=float(match.group(2))
                w=float(match.group(3))

                if u not in adj_list:
                    adj_list[u]=[]
                if v not in adj_list:
                    adj_list[v]=[]

                adj_list[u].append((v, w))

    output_data=adj_list

    with open(output_file, 'w') as f:
        json.dump(output_data, f, indent=2)

    print(f"Successfully enriched graph. Exported to {output_file}")

def main():
    print("Running enrichment")
    if len(sys.argv) != 3:
        sys.exit("Usage: python3 enrich.py <input.dot> <output.json>")

    dot_file: str = sys.argv[1]
    output_file: str = sys.argv[2]
    generateGraph(dot_file, output_file)

if __name__ == "__main__":
    main()

