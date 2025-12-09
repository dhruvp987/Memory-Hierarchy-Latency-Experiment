#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

// 50 million iterations should allow the program to run enough
// to provide decent measurements
#define ITERATIONS 50000000

typedef struct Node {
	struct Node* next;
} Node;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " [path to csv]" << std::endl;
		return 1;
	}
	const std::string outputCsvName = argv[1];

	size_t workingSetSizes[] = {
    		49152,   // 48 KB
    		65536,   // 64 KB
    		98304,   // 96 KB
    		131072,  // 128 KB
    		196608,  // 192 KB
    		262144,  // 256 KB
    		393216,  // 384 KB
    		524288,  // 512 KB
    		786432,  // 768 KB
    		1048576, // 1 MB
    		1572864, // 1.5 MB
    		2097152, // 2 MB
    		3145728, // 3 MB
    		4194304, // 4 MB
    		6291456, // 6 MB
    		8388608, // 8 MB
    		12582912,// 12 MB
    		16777216,// 16 MB
    		25165824,// 24 MB
    		33554432,// 32 MB
    		50331648,// 48 MB
    		67108864 // 64 MB
	};
	int numWorkingSets = 22;

	size_t strideSizes[] = {
    		1,       // 1 B
    		2,       // 2 B
    		4,       // 4 B
    		8,       // 8 B
    		16,      // 16 B
    		32,      // 32 B
    		64,      // 64 B
    		128,     // 128 B
    		256,     // 256 B
    		512,     // 512 B
    		1024,    // 1 KB
    		2048,    // 2 KB
    		4096,    // 4 KB
    		8192,    // 8 KB
    		16384,   // 16 KB
    		32768    // 32 KB
	};
	int numStrides = 16;

	double avg_latency[numWorkingSets][numStrides];

	for (int i = 0; i < numWorkingSets; i++) {
		size_t wSetSize = workingSetSizes[i];

		// Use pointer chasing to avoid prefetching,
		// because prefetching will not allow us to test latency
		
		size_t numNodes = wSetSize / sizeof(Node);
		Node* nodes = new Node[numNodes];

		// Fill nodes array with nodes
		for (int j = 0; j < numNodes; j++) {
			nodes[j] = {NULL};
		}

		for (int j = 0; j < numStrides; j++) {
			size_t stride = strideSizes[j];
			
			int jumpLength = stride / sizeof(Node);
			if (jumpLength == 0) jumpLength = 1;

			int numJumps = numNodes / jumpLength;

			// Create the linked list of nodes
			int idx = 0;
			while (idx < numNodes) {
				int nextIdx = (idx + jumpLength) % numNodes;
				(nodes+idx)->next = nodes + nextIdx;
				idx += jumpLength;
			}
			// Set last jumped node to first node
			idx -= jumpLength;
			(nodes+idx)->next = nodes;

			// Warmup pages and TLB
			Node* p = nodes;
			for (int k = 0; k < numJumps; k++) {
				p = p->next;
			}

			// Measure memory access latency
			p = nodes;
			auto start = std::chrono::steady_clock::now();
			for (int k = 0; k < ITERATIONS; k++) {
				p = p->next;
			}
			auto end = std::chrono::steady_clock::now();

			// Calculate latency in nanoseconds
			std::chrono::duration<double> diff = end - start;
			std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff);
			double latency = ns.count();

			avg_latency[i][j] = latency / ITERATIONS;
			std::cout
				<< "Working set size of " << workingSetSizes[i] 
				<< " bytes, stride size of " << strideSizes[j] 
				<< " bytes, average latency: " << avg_latency[i][j]
				<< " ns" << std::endl;
		}

		delete[] nodes;
	}

	std::ofstream csvFile(outputCsvName);

    	csvFile << "WorkingSetSize";
   	for (int j = 0; j < numStrides; j++) {
        	csvFile << "," << strideSizes[j];
    	}
    	csvFile << "\n";

    	for (int i = 0; i < numWorkingSets; i++) {
        	csvFile << workingSetSizes[i];
        	for (int j = 0; j < numStrides; j++) {
            		csvFile << "," << std::fixed << std::setprecision(2) << avg_latency[i][j];
        	}
        	csvFile << "\n";
    	}

    	csvFile.close();
    	std::cout << "Written to " << outputCsvName << std::endl;
}
