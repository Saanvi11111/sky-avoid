# sky-avoid v1.0.0

A high-performance, low-latency C++20 flight routing and dynamic obstacle avoidance engine built completely from scratch. 

Instead of processing static textbook graphs, `sky-avoid` optimizes and prunes spatial networks on-the-fly when real-time localized threat vectors (like convective storm fronts or restricted airspace blocks) collide with active flight paths.

## Key Technical Specifications
* **Custom Routing Core:** Implements Dijkstra's Shortest Path algorithm utilizing custom spatial vectors and an optimized Min-Heap Priority Queue.
* **Dynamic Spatial Intercepts:** Employs linear interpolation vectors to check flight paths against circular geometric hazard radiuses at runtime.
* **Dual Execution Modes:** Support for interactive manual CLI prompts and automated machine-readable raw JSON generation (`--json`) for enterprise shell script integration.

## Compilation and Local Execution
Compile natively using standard C++20 guidelines:
```bash
clang++ -std=c++20 main.cpp -o sky_avoid
```

### Run Interactive Manual Mode:
```bash
./sky_avoid
```

### Run Automated Enterprise Service Pipeline:
```bash
./sky_avoid --json STL ORD --storm 40.11 -88.24 0.5
```
