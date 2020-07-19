#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include <queue>
#include <algorithm>

using namespace std;

vector<pair<string, unsigned long long>> instructions;

struct cache_line {
	int valid = 0;
	unsigned long tag = -1;
	int lru = 0;
};

unsigned long extract_bits(unsigned long long addr, int num_bits, int pos) {
	return (((1 << num_bits) - 1) & (addr >> (pos - 1)));
}

int fully_associative_hc() {
	int cache_size = 16384;
	int line_size = 32;
	int associativity = cache_size / line_size;
	int num_sets = cache_size / line_size / associativity;
	int cache_hits = 0;
	int num_idx_bits = (int)(log2(num_sets));
	int num_os_bits = (int)(log2(line_size));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;

	//all hot cold bits initialized to 0
	int hotcold[associativity - 1] = {0};

	//create a cache with num_sets sets and # ways specified in parameter
	struct cache_line cache[num_sets][associativity];
	
	for (auto insn: instructions) {
		//get index and tag bits
		unsigned long index = extract_bits(insn.second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(insn.second, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match = false;
		int hit_idx = -1;
		int hc_idx = 0;

		for (int i = 0; i < associativity; i++) {
			if (cache[index][i].tag == tag) {
				//cache hit
				cache_hits++;
				hit_idx = i;
				match = true;
				hc_idx = hit_idx + 511;
				while (hc_idx != 0) {
					if (hc_idx % 2 == 0) {
						hc_idx = (hc_idx - 2) / 2;
						hotcold[hc_idx] = 0;
					}
					else {
						hc_idx = (hc_idx - 1) / 2;
						hotcold[hc_idx] = 1;
					}
				}
			}
		}
		//cache miss
		if (!match) {
			hc_idx = 0;
			for (int j = 0; j < 9; j++) {
				if (hotcold[hc_idx] == 0) {
					hotcold[hc_idx] = 1;
					hc_idx = hc_idx * 2 + 1;
				}
				else {
					hotcold[hc_idx] = 0;
					hc_idx = hc_idx * 2 + 2;
				}
			}
			cache[index][hc_idx - 511].tag = tag;
			if (cache[index][hc_idx - 511].valid == 0) {
				cache[index][hc_idx - 511].valid = 1;
			}
		}
	}
	return cache_hits;
}	

/*Set-Associative Cache with next-line prefetching on a miss*/
int set_associative_pfm(int associativity) {
	int cache_size = 16384;
	int line_size = 32;
	int num_sets = cache_size / line_size / associativity;
	int cache_hits = 0;
	int num_idx_bits = (int)(log2(num_sets));
	int num_os_bits = (int)(log2(line_size));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;
	
	//create a cache with num_sets sets and # ways specified in parameter
	struct cache_line cache[num_sets][associativity];
	
	//for every set, use a queue-like structure that keeps track of lru
	vector<vector<int>> lru_queues;

	//initialize queue to be 0, ..., num_sets - 1
	for (int i = 0; i < num_sets; i++) {
		vector<int> temp;
		for (int j = 0; j < associativity; j++) {
			temp.push_back(j);
		}
		lru_queues.push_back(temp);
	}

	for (unsigned int i = 0; i < instructions.size(); i++) {
		//get index and tag bits
		unsigned long index = extract_bits(instructions[i].second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(instructions[i].second, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match = false;
		for (int j = 0; j < associativity; j++) {
			if (cache[index][j].tag == tag && cache[index][j].valid == 1) {
				//cache hit
				cache_hits++;
				//find the way that gave a hit and put it at the back of the queue
				vector<int>::iterator lru_idx = find(lru_queues[index].begin(), lru_queues[index].end(), j);
				lru_queues[index].erase(lru_idx);
				lru_queues[index].push_back(j);
				match = true;
			}
		}
		if (!match) {
			//get the least recently used way from the front of the queue
			int lru = lru_queues[index][0];
			cache[index][lru].tag = tag;
			if (cache[index][lru].valid == 0) {
				cache[index][lru].valid = 1;
			}
			//move the lru way from the front to the back of the queue because we just used it
			lru_queues[index].erase(lru_queues[index].begin());
			lru_queues[index].push_back(lru);

			unsigned long new_addr = instructions[i].second + line_size;
			unsigned long index2 = extract_bits(new_addr, num_idx_bits, 1+num_os_bits);
			unsigned long tag2 = extract_bits(new_addr, num_tag_bits, 1+num_os_bits+num_idx_bits);

			bool match2 = false;
			for (int k = 0; k < associativity; k++) {
				if (cache[index2][k].tag == tag2 && cache[index2][k].valid == 1) {
					//cache hit
					//cache_hits++;
					//find the way that gave a hit and put it at the back of the queue
					vector<int>::iterator lru_idx = find(lru_queues[index2].begin(), lru_queues[index2].end(), k);
					lru_queues[index2].erase(lru_idx);
					lru_queues[index2].push_back(k);
					match2 = true;
				}
			}

			if (!match2) {
				//get the least recently used way from the front of the queue
				int lru = lru_queues[index2][0];
				cache[index2][lru].tag = tag2;
				if (cache[index2][lru].valid == 0) {
					cache[index2][lru].valid = 1;
				}
				//move the lru way from the front to the back of the queue because we just used it
				lru_queues[index2].erase(lru_queues[index2].begin());
				lru_queues[index2].push_back(lru);
			}

		}

			
	}

	return cache_hits;
}	

/*Set-Associative Cache with next-line prefetching*/
int set_associative_pf(int associativity) {
	int cache_size = 16384;
	int line_size = 32;
	int num_sets = cache_size / line_size / associativity;
	int cache_hits = 0;
	int num_idx_bits = (int)(log2(num_sets));
	int num_os_bits = (int)(log2(line_size));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;
	
	//create a cache with num_sets sets and # ways specified in parameter
	struct cache_line cache[num_sets][associativity];
	
	//for every set, use a queue-like structure that keeps track of lru
	vector<vector<int>> lru_queues;

	//initialize queue to be 0, ..., num_sets - 1
	for (int i = 0; i < num_sets; i++) {
		vector<int> temp;
		for (int j = 0; j < associativity; j++) {
			temp.push_back(j);
		}
		lru_queues.push_back(temp);
	}

	for (unsigned int i = 0; i < instructions.size(); i++) {
		//get index and tag bits
		unsigned long index = extract_bits(instructions[i].second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(instructions[i].second, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match = false;
		for (int j = 0; j < associativity; j++) {
			if (cache[index][j].tag == tag && cache[index][j].valid == 1) {
				//cache hit
				cache_hits++;
				//find the way that gave a hit and put it at the back of the queue
				vector<int>::iterator lru_idx = find(lru_queues[index].begin(), lru_queues[index].end(), j);
				lru_queues[index].erase(lru_idx);
				lru_queues[index].push_back(j);
				match = true;
			}
		}
		if (!match) {
			//get the least recently used way from the front of the queue
			int lru = lru_queues[index][0];
			cache[index][lru].tag = tag;
			if (cache[index][lru].valid == 0) {
				cache[index][lru].valid = 1;
			}
			//move the lru way from the front to the back of the queue because we just used it
			lru_queues[index].erase(lru_queues[index].begin());
			lru_queues[index].push_back(lru);
		}

		unsigned long new_addr = instructions[i].second + line_size;
		unsigned long index2 = extract_bits(new_addr, num_idx_bits, 1+num_os_bits);
		unsigned long tag2 = extract_bits(new_addr, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match2 = false;
		for (int k = 0; k < associativity; k++) {
			if (cache[index2][k].tag == tag2 && cache[index2][k].valid == 1) {
				//cache hit
				//cache_hits++;
				//find the way that gave a hit and put it at the back of the queue
				vector<int>::iterator lru_idx = find(lru_queues[index2].begin(), lru_queues[index2].end(), k);
				lru_queues[index2].erase(lru_idx);
				lru_queues[index2].push_back(k);
				match2 = true;
			}
		}
		if (!match2) {
			//get the least recently used way from the front of the queue
			int lru = lru_queues[index2][0];
			cache[index2][lru].tag = tag2;
			if (cache[index2][lru].valid == 0) {
				cache[index2][lru].valid = 1;
			}
			//move the lru way from the front to the back of the queue because we just used it
			lru_queues[index2].erase(lru_queues[index2].begin());
			lru_queues[index2].push_back(lru);
		}
	
	}

	return cache_hits;
}	

/*Set-Associative Cache with no allocation on a write miss*/
int set_associative_noalloc(int associativity) {
	int cache_size = 16384;
	int line_size = 32;
	int num_sets = cache_size / line_size / associativity;
	int cache_hits = 0;
	int num_idx_bits = (int)(log2(num_sets));
	int num_os_bits = (int)(log2(line_size));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;
	
	//create a cache with num_sets sets and # ways specified in parameter
	struct cache_line cache[num_sets][associativity];
	
	//for every set, use a queue-like structure that keeps track of lru
	vector<vector<int>> lru_queues;

	//initialize queue to be 0, ..., num_sets - 1
	for (int i = 0; i < num_sets; i++) {
		vector<int> temp;
		for (int j = 0; j < associativity; j++) {
			temp.push_back(j);
		}
		lru_queues.push_back(temp);
	}

	for (auto insn: instructions) {
		//get index and tag bits
		unsigned long index = extract_bits(insn.second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(insn.second, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match = false;
		for (int i = 0; i < associativity; i++) {
			if (cache[index][i].tag == tag) {
				//cache hit
				cache_hits++;
				//find the way that gave a hit and put it at the back of the queue
				vector<int>::iterator lru_idx = find(lru_queues[index].begin(), lru_queues[index].end(), i);
				lru_queues[index].erase(lru_idx);
				lru_queues[index].push_back(i);
				match = true;
			}
		}
		if (!match) {
			if (insn.first == "L") { 
				//get the least recently used way from the front of the queue
				int lru = lru_queues[index][0];
				cache[index][lru].tag = tag;
				if (cache[index][lru].valid == 0) {
					cache[index][lru].valid = 1;
				}
				//move the lru way from the front to the back of the queue because we just used it
				lru_queues[index].erase(lru_queues[index].begin());
				lru_queues[index].push_back(lru);
			}
		}	
	}

	return cache_hits;
}

/*Set-Associative Cache*/
int set_associative(int associativity) {
	int cache_size = 16384;
	int line_size = 32;
	int num_sets = cache_size / line_size / associativity;
	int cache_hits = 0;
	int num_idx_bits = (int)(log2(num_sets));
	int num_os_bits = (int)(log2(line_size));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;
	
	//create a cache with num_sets sets and # ways specified in parameter
	struct cache_line cache[num_sets][associativity];
	
	//for every set, use a queue-like structure that keeps track of lru
	vector<vector<int>> lru_queues;

	//initialize queue to be 0, ..., num_sets - 1
	for (int i = 0; i < num_sets; i++) {
		vector<int> temp;
		for (int j = 0; j < associativity; j++) {
			temp.push_back(j);
		}
		lru_queues.push_back(temp);
	}

	for (auto insn: instructions) {
		//get index and tag bits
		unsigned long index = extract_bits(insn.second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(insn.second, num_tag_bits, 1+num_os_bits+num_idx_bits);

		bool match = false;
		for (int i = 0; i < associativity; i++) {
			if (cache[index][i].tag == tag) {
				//cache hit
				cache_hits++;
				//find the way that gave a hit and put it at the back of the queue
				vector<int>::iterator lru_idx = find(lru_queues[index].begin(), lru_queues[index].end(), i);
				lru_queues[index].erase(lru_idx);
				lru_queues[index].push_back(i);
				match = true;
			}
		}
		if (!match) {
			//get the least recently used way from the front of the queue
			int lru = lru_queues[index][0];
			cache[index][lru].tag = tag;
			if (cache[index][lru].valid == 0) {
				cache[index][lru].valid = 1;
			}
			//move the lru way from the front to the back of the queue because we just used it
			lru_queues[index].erase(lru_queues[index].begin());
			lru_queues[index].push_back(lru);
		}
	}
	return cache_hits;
}	

/*Direct-Mapped Cache*/
int direct_mapped(int cache_size) {
	//number of cache lines = cache size / line size
	int num_lines = cache_size / 32;
	
	//number of cache hits
	int cache_hits = 0;

	//create new cache
	struct cache_line cache[num_lines];
	
	//number of bits to extract from address for index, offset, and tag
	int num_idx_bits = (int)(log2(num_lines));
	int num_os_bits = (int)(log2(32));
	int num_tag_bits = 32 - num_idx_bits - num_os_bits;

	for (auto insn : instructions) {
		//extract bits from address
		//unsigned long offset = extract_bits(insn.second, num_os_bits, 1);

		unsigned long index = extract_bits(insn.second, num_idx_bits, 1+num_os_bits);
		unsigned long tag = extract_bits(insn.second, num_tag_bits, 1+num_os_bits+num_idx_bits);
		
		//if the cache line is not valid, insert the tag and set valid to 1
		if (cache[index].valid == 0) {
			cache[index].valid = 1;
			cache[index].tag = tag;
		}
		else {
			//if cache line is valid and tags match, it's a cache hit
			//otherwise, update the tag
			if (cache[index].tag == tag) {
				cache_hits++;	
			}
			else {
				cache[index].tag = tag;
			}
		}
	}
	return cache_hits;
}

int main(int argc, char *argv[]) {
	//Temp variables to hold traces
	string ls,line;
	unsigned long long address = 0;
	unsigned long num_accesses = 0;

	/* Open input file and read one line at a time,
	populating global instructios vector */
	ifstream input(argv[1]);
	while (getline(input, line)) {
		num_accesses++;
		stringstream s(line);
		s >> ls >> std::hex >> address;
		instructions.push_back(make_pair(ls, address));
	}

	ofstream output;
	output.open(argv[2]);
	
	//line 1: direct-mapped cache: 1KB, 4KB, 16KB, 32KB
  	output << direct_mapped(1024) << "," << num_accesses << "; " << direct_mapped(4096) << "," << num_accesses << "; " << direct_mapped(16384) << "," << num_accesses << "; " << direct_mapped(32768) << "," << num_accesses << ";" << endl; 

	//line 2: set-associative cache: 2, 4, 8, 16
  	output << set_associative(2) << "," << num_accesses << "; " << set_associative(4) << "," << num_accesses << "; " << set_associative(8) << "," << num_accesses << "; " << set_associative(16) << "," << num_accesses << ";" << endl; 

	//line 3: fully-associative cache with LRU policy
	/* NOTE: No additional function needed - fully-associative cache 
	 * with LRU is the same as a set-associative cache with N ways 
	 * where N = # lines */
	output << set_associative(512) << "," << num_accesses << ";" << endl; 

	//line 4: fully-associative cache with hot-cold LRU approximation policy
	output << fully_associative_hc() << "," << num_accesses << ";" << endl; 
	
	//line 5: set-associative cache with no allocation on a write miss: 2, 4, 8, 16
  	output << set_associative_noalloc(2) << "," << num_accesses << "; " << set_associative_noalloc(4) << "," << num_accesses << "; " << set_associative_noalloc(8) << "," << num_accesses << "; " << set_associative_noalloc(16) << "," << num_accesses << ";" << endl;

	//line 6: set-associative cache with next-line prefetching: 2, 4, 8, 16
  	output << set_associative_pf(2) << "," << num_accesses << "; " << set_associative_pf(4) << "," << num_accesses << "; " << set_associative_pf(8) << "," << num_accesses << "; " << set_associative_pf(16) << "," << num_accesses << ";" << endl;

	//line 7: set-associative cache with next-line prefetching on a miss: 2, 4, 8, 16
  	output << set_associative_pfm(2) << "," << num_accesses << "; " << set_associative_pfm(4) << "," << num_accesses << "; " << set_associative_pfm(8) << "," << num_accesses << "; " << set_associative_pfm(16) << "," << num_accesses << ";" << endl; 

	output.close();
}
