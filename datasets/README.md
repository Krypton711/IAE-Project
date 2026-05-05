# Graph Datasets for Diameter Benchmarking

## Recommended Sources

### 1. SNAP (Stanford Network Analysis Platform)
**URL:** https://snap.stanford.edu/data/

The gold standard for graph benchmarking. Files are in edge-list format (`u v` per line, comment lines start with `#`).

**Recommended graphs:**

| Graph | Nodes | Edges | Type | Notes |
|-------|-------|-------|------|-------|
| `email-Enron` | 36,692 | 183,831 | Communication | Small, connected |
| `soc-Epinions1` | 75,879 | 508,837 | Social | Trust network |
| `com-DBLP` | 317,080 | 1,049,866 | Collaboration | Well-studied |
| `com-Amazon` | 334,863 | 925,872 | Co-purchase | Product network |
| `com-Youtube` | 1,134,890 | 2,987,624 | Social | Medium-large |
| `roadNet-CA` | 1,965,206 | 2,766,607 | Road | High diameter |
| `roadNet-TX` | 1,379,917 | 1,921,660 | Road | High diameter |
| `roadNet-PA` | 1,088,092 | 1,541,898 | Road | High diameter |
| `com-LiveJournal` | 3,997,962 | 34,681,189 | Social | Large |
| `wiki-Talk` | 2,394,385 | 5,021,410 | Communication | Large |

### 2. Network Repository
**URL:** https://networkrepository.com/

5000+ graphs in many domains. Download `.edges` files directly.

Good for variety: biological, infrastructure, web, social networks.

### 3. KONECT (Koblenz Network Collection)
**URL:** http://konect.cc/

Many real-world networks with metadata.

### 4. DIMACS Challenge Graphs
**URL:** http://www.diag.uniroma1.it/challenge9/download.shtml

Road networks from the 9th DIMACS Implementation Challenge.

## How to Download

### SNAP Example
```bash
# Create datasets directory
mkdir -p datasets

# Download a few SNAP graphs
cd datasets
wget https://snap.stanford.edu/data/email-Enron.txt.gz && gunzip email-Enron.txt.gz
wget https://snap.stanford.edu/data/soc-Epinions1.txt.gz && gunzip soc-Epinions1.txt.gz
wget https://snap.stanford.edu/data/com-dblp.ungraph.txt.gz && gunzip com-dblp.ungraph.txt.gz
wget https://snap.stanford.edu/data/com-amazon.ungraph.txt.gz && gunzip com-amazon.ungraph.txt.gz
wget https://snap.stanford.edu/data/roadNet-CA.txt.gz && gunzip roadNet-CA.txt.gz
```

## File Format

The benchmarker expects **edge-list** format:
```
# Optional comment lines (starting with # or %)
0 1
0 2
1 3
2 3
...
```
- One edge per line: `source destination` (whitespace-separated)
- Node IDs are integers (they get remapped internally)
- Self-loops are automatically ignored
- Duplicate edges are automatically handled
- Supported extensions: `.txt`, `.edges`, `.csv`, `.tsv`, `.el`

## Running the Benchmark

```bash
# Build
make

# Run on a single graph
./build/benchmark datasets/email-Enron.txt

# Run on all graphs in a directory
./build/benchmark datasets/

# Skip exact computation for large graphs
./build/benchmark --naive-limit 30000 datasets/

# Custom output file
./build/benchmark --output results/my_run.csv datasets/
```
