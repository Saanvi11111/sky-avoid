#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <climits>
#include <cmath>
#include <fstream>  
#include <sstream>  

// ==========================================
// DATA STRUCTURE CONFIGURATIONS
// ==========================================
struct Coordinate {
    double lat;
    double lon;
    double distanceTo(const Coordinate& other) const {
        return std::sqrt(std::pow(lat - other.lat, 2) + std::pow(lon - other.lon, 2));
    }
};

struct AirspaceNode {
    std::string name;
    Coordinate coords;
};

struct JetwayEdge {
    std::string destination;
    double default_distance;
};

struct WeatherCell {
    std::string id;
    Coordinate center;
    double radius;

    bool intersectsPath(const Coordinate& src, const Coordinate& dest) const {
        if (src.distanceTo(center) <= radius || dest.distanceTo(center) <= radius) {
            return true; 
        }
        for (int i = 1; i <= 10; ++i) {
            double t = i / 10.0;
            double check_lat = src.lat + t * (dest.lat - src.lat);
            double check_lon = src.lon + t * (dest.lon - src.lon);
            Coordinate check_point = {check_lat, check_lon};
            if (check_point.distanceTo(center) <= radius) return true; 
        }
        return false;
    }
};

class SkyAvoidEngine {
private:
    std::unordered_map<std::string, AirspaceNode> nodes;
    std::unordered_map<std::string, std::vector<JetwayEdge>> graph;
    std::vector<WeatherCell> active_storms;

public:
    void registerAirport(const std::string& name, double lat, double lon) {
        nodes[name] = {name, {lat, lon}};
    }

    void connectJetway(const std::string& src, const std::string& dest) {
        if (nodes.find(src) == nodes.end() || nodes.find(dest) == nodes.end()) return;
        double dist = nodes[src].coords.distanceTo(nodes[dest].coords);
        graph[src].push_back({dest, dist});
    }

    void injectStorm(const std::string& id, double lat, double lon, double radius) {
        active_storms.push_back({id, {lat, lon}, radius});
    }

    void clearStorms() { active_storms.clear(); }

    bool loadAirports(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string name, lat_str, lon_str;
            std::getline(ss, name, ','); std::getline(ss, lat_str, ','); std::getline(ss, lon_str, ',');
            registerAirport(name, std::stod(lat_str), std::stod(lon_str));
        }
        return true;
    }

    bool loadRoutes(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string src, dest;
            std::getline(ss, src, ','); std::getline(ss, dest, ',');
            connectJetway(src, dest);
        }
        return true;
    }

    // ==========================================
    // UPDATED ROUTING WITH DUAL-OUTPUT CHANNELS
    // ==========================================
    void calculateRoute(const std::string& start, const std::string& end, bool json_mode) {
        if (nodes.find(start) == nodes.end() || nodes.find(end) == nodes.end()) {
            if (json_mode) std::cout << "{\"status\":\"error\",\"message\":\"Missing nodes\"}\n";
            else std::cout << "❌ Error: Invalid airport identifiers.\n";
            return;
        }

        std::unordered_map<std::string, double> distances;
        std::unordered_map<std::string, std::string> previous;
        auto comp = [](std::pair<std::string, double> left, std::pair<std::string, double> right) { return left.second > right.second; };
        std::priority_queue<std::pair<std::string, double>, std::vector<std::pair<std::string, double>>, decltype(comp)> min_heap(comp);

        for (const auto& pair : nodes) distances[pair.first] = INT_MAX;
        distances[start] = 0.0; min_heap.push({start, 0.0});

        while (!min_heap.empty()) {
            std::string current = min_heap.top().first;
            double current_dist = min_heap.top().second;
            min_heap.pop();
            if (current == end) break;
            if (current_dist > distances[current]) continue;

            Coordinate src_coords = nodes[current].coords;
            for (const auto& edge : graph[current]) {
                Coordinate dest_coords = nodes[edge.destination].coords;
                bool edge_is_blocked = false;
                for (const auto& storm : active_storms) {
                    if (storm.intersectsPath(src_coords, dest_coords)) { edge_is_blocked = true; break; }
                }
                if (edge_is_blocked) continue;

                double new_dist = current_dist + edge.default_distance;
                if (new_dist < distances[edge.destination]) {
                    distances[edge.destination] = new_dist;
                    previous[edge.destination] = current;
                    min_heap.push({edge.destination, new_dist});
                }
            }
        }

        if (distances[end] == INT_MAX) {
            if (json_mode) std::cout << "{\"status\":\"routing_failure\",\"path\":[]}\n";
            else std::cout << "❌ ROUTING FAILURE: No open trajectories available.\n";
            return;
        }

        std::vector<std::string> path;
        for (std::string at = end; at != ""; at = previous[at]) {
            path.push_back(at); if (at == start) break;
        }

        // OUTPUT FORMAT SWITCH ENGINE
        if (json_mode) {
            // Print clean, raw machine-interpretable string data layout
            std::cout << "{\"status\":\"success\",\"distance\":" << distances[end] << ",\"path\":[";
            for (int i = path.size() - 1; i >= 0; i--) {
                std::cout << "\"" << path[i] << "\"" << (i == 0 ? "" : ",");
            }
            std::cout << "]}\n";
        } else {
            std::cout << "✈️ DYNAMIC ROUTE SECURED: ";
            for (int i = path.size() - 1; i >= 0; i--) std::cout << path[i] << (i == 0 ? "" : " -> ");
            std::cout << " (Distance: " << distances[end] << ")\n";
        }
    }
};

// ==========================================
// ARGUMENT-AWARE ENTRYPOINT EXECUTABLE
// ==========================================
int main(int argc, char* argv[]) {
    SkyAvoidEngine engine;

    // Check if the script was triggered using automated enterprise --json flag args
    if (argc >= 4 && std::string(argv[1]) == "--json") {
        if (!engine.loadAirports("airports.csv") || !engine.loadRoutes("routes.csv")) return 1;
        std::string src = argv[2];
        std::string dest = argv[3];
        
        // Check for static runtime storm injectors passed from command line args
        if (argc >= 8 && std::string(argv[4]) == "--storm") {
            engine.injectStorm("CLI_STORM", std::stod(argv[5]), std::stod(argv[6]), std::stod(argv[7]));
        }
        
        engine.calculateRoute(src, dest, true); // Execute straight to JSON pipe
        return 0; // Exit cleanly without executing the loop
    }

    // Default Interactive Human CLI Shell Flow
    std::cout << "================================================\n";
    std::cout << "   SKY-AVOID DYNAMIC ENGINE INITIALIZED v1.0   \n";
    std::cout << "================================================\n";
    if (!engine.loadAirports("airports.csv") || !engine.loadRoutes("routes.csv")) return 1;
    std::cout << "✅ Systems Active. Type 'HELP' for available terminal prompts.\n\n";

    std::string line;
    while (std::cout << "sky-avoid> " && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line == "EXIT" || line == "exit") break;

        std::stringstream ss(line);
        std::string command; ss >> command;

        if (command == "HELP" || command == "help") {
            std::cout << "\n--- Terminal Prompts ---\n  ROUTE [SRC] [DEST]\n  STORM [ID] [LAT] [LON] [R]\n  CLEAR\n  EXIT\n\n";
        } else if (command == "ROUTE" || command == "route") {
            std::string src, dest; ss >> src >> dest;
            engine.calculateRoute(src, dest, false);
        } else if (command == "STORM" || command == "storm") {
            std::string id; double lat, lon, radius; ss >> id >> lat >> lon >> radius;
            engine.injectStorm(id, lat, lon, radius);
            std::cout << "⚠️ Hazard Constraint Locked: " << id << "\n";
        } else if (command == "CLEAR" || command == "clear") {
            engine.clearStorms();
            std::cout << "✅ System Constraints Wiped.\n";
        } else {
            std::cout << "❌ Unknown prompt. Type HELP.\n";
        }
    }
    return 0;
}
