#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<utility>
#include <math.h>

#define TAKEN 1
#define NOT_TAKEN 0

#define STRONGLY_TAKEN 3
#define WEAKLY_TAKEN 2
#define WEAKLY_NOT_TAKEN 1
#define STRONGLY_NOT_TAKEN 0

using namespace std;

pair<long, long> branch_target_buffer(vector<pair<unsigned long long, string>> branches, vector<unsigned long long> targets) {
	//make predictor table of size 512
	int predictor_table[512];
	//make branch target buffer of size 512
	unsigned long long btb[512];
	long correct = 0;
	long num_accesses = 0;

	//set all initial values in predictor table to taken (1)
	for (int i = 0; i < 512; i++) {
		predictor_table[i] = TAKEN;
		btb[i] = 0;
	}

	//loop through all the branch instructions
	for (unsigned int j = 0; j < branches.size(); j++) {
		unsigned long long addr = branches[j].first;
		string behavior = branches[j].second;
		//get last n bits of address depending on size of table
		int last_n_bits = (addr & ((1 << ((int)log2(512))) - 1));
		//if prediction is taken
		if (predictor_table[last_n_bits % 512] == TAKEN) {
			//access the btb
			num_accesses++;
			if (btb[last_n_bits % 512] == targets[j]) {
				correct++;
			}
			//if actual outcome was taken, our prediction was correct
			if (behavior == "T") {
				btb[last_n_bits % 512] = targets[j];
			}
			//otherwise switch the prediction
			else {
				predictor_table[last_n_bits % 512] = NOT_TAKEN;
			}
		}
		//if prediction is not taken
		else {
			//if actual outcome was not taken, our prediction was correct
			if (behavior == "NT") {
				continue;
			}
			//otherwise switch the prediction
			else {
				predictor_table[last_n_bits % 512] = TAKEN;

			}

		}
	}
	pair<long, long> retvals(num_accesses, correct);
	return retvals;

}

long tournament_predictor(vector<pair<unsigned long long, string>> branches) {
	//initialize bimodal predictor table, gshare predictor table, and selector table
	int bimodal_table[2048];
	int gshare_table[2048];
	int selector_table[2048];
	long num_correct = 0;
	unsigned int ghr = 0;

	//set all initial values in selector table to prefer gshare
	for (int i = 0; i < 2048; i++) {
		selector_table[i] = STRONGLY_NOT_TAKEN;
		bimodal_table[i] = STRONGLY_TAKEN;
		gshare_table[i] = STRONGLY_TAKEN;
	}

	//loop through all the branch instructions
	for (pair<unsigned long long, string> branch : branches) {
		bool bimodal_correct = false;
		bool gshare_correct = false;
		unsigned long long addr = branch.first;
		string behavior = branch.second;
		//get last 11 bits of address
		unsigned int pc_index = (addr & ((1 << 11) - 1)) % 2048;
		//get index to use for gshare table 
		unsigned int g_index = (pc_index ^ ghr) % 2048;
		//take care of bimodal predictor table
		//if bimodal prediction is taken
		if (bimodal_table[pc_index] == STRONGLY_TAKEN || bimodal_table[pc_index] == WEAKLY_TAKEN) {
			//the bimodal prediction was correct
			if (behavior == "T") {
				//if bimodal prediction was weakly taken, set to strongly taken
				if (bimodal_table[pc_index] == WEAKLY_TAKEN) {
					(bimodal_table[pc_index])+=1;
				}
				bimodal_correct = true;
			}
			//if our bimodal prediction was incorrect, set ST to WT or WT to WNT
			else {
				(bimodal_table[pc_index])-=1;
			}
		}
		//if bimodal prediction is not taken
		else {
			//our prediction was correct
			if (behavior == "NT") {
				//if prediction was weakly not taken, set to strongly not taken
				if (bimodal_table[pc_index] == WEAKLY_NOT_TAKEN) {
					(bimodal_table[pc_index])-=1;
				}
				bimodal_correct = true;
			}
			//if our prediction was incorrect, set SNT to WNT or WNT to WT
			else {
				(bimodal_table[pc_index])+=1;

			}

		}

		//take care of gshare table
		if (gshare_table[g_index] == STRONGLY_TAKEN || gshare_table[g_index] == WEAKLY_TAKEN) {
			//our prediction was correct
			if (behavior == "T") {
				//if prediction was weakly taken, set to strongly taken
				if (gshare_table[g_index] == WEAKLY_TAKEN) {
					(gshare_table[g_index])+=1;
				}
				gshare_correct = true;
				ghr = ((ghr << 1) | 1) & ((1 << (11)) - 1);

			}
			//if our prediction was incorrect, set ST to WT or WT to WNT
			else {
				(gshare_table[g_index])-=1;
				ghr = (ghr << 1) & ((1 << (11)) - 1);

			}
		}
		//if prediction is not taken
		else {
			//our prediction was correct
			if (behavior == "NT") {
				//if prediction was weakly not taken, set to strongly not taken
				if (gshare_table[g_index] == WEAKLY_NOT_TAKEN) {
					(gshare_table[g_index])-=1;
				}
				gshare_correct = true;
				ghr = (ghr << 1) & ((1 << (11)) - 1);

			}
			//if our prediction was incorrect, set SNT to WNT or WNT to WT
			else {
				(gshare_table[g_index])+=1;
				ghr = ((ghr << 1) | 1) & ((1 << (11)) - 1);
			}
			
		}

		//take care of selector table
		if ((gshare_correct && bimodal_correct)) {
			num_correct++;
			continue;
		}
		else if (!gshare_correct && !bimodal_correct) {
			continue;
		}
		//if selector is 2 or 3, we prefer bimodal
		if (selector_table[pc_index] == STRONGLY_TAKEN || selector_table[pc_index] == WEAKLY_TAKEN) {
			//the prediction to prefer bimodal was correct
			if (bimodal_correct) {
				//if selector weakly preferred bimodal, set to strongly prefer bimodal
				if (selector_table[pc_index] == WEAKLY_TAKEN) {
					(selector_table[pc_index])+=1;
				}
				num_correct++;
			}
			//if the prediction to prefer bimodal was incorrect, set strongly prefer bimodal to weakly prefer bimodal or weakly prefer bimodal to weakly prefer gshare
			else {
				(selector_table[pc_index])-=1;
			}
		}
		//if selector is 0 or 1, we prefer gshare
		else {
			//the prediction to prefer gshare  was correct
			if (gshare_correct) {
				//if selector weakly preferred gshare, set to strongly prefer gshare
				if (selector_table[pc_index] == WEAKLY_NOT_TAKEN) {
					(selector_table[pc_index])-=1;
				}
				num_correct++;
			}
			//if the prediction to prefer gshare was incorrect, set strongly prefer gshare to weakly prefer gshare or weakly prefer gshare to weakly prefer bimodal
			else {
				(selector_table[pc_index])+=1;
			}

		}
	}
	return num_correct;	
}

long gshare_predictor(vector<pair<unsigned long long, string>> branches, int ghr_bits) {
	//make predictor table of size 2048
	int predictor_table[2048];
	long num_correct = 0;
	unsigned int ghr = 0;

	//set all initial values in predictor table to strongly taken (3)
	for (int i = 0; i < 2048; i++) {
		predictor_table[i] = STRONGLY_TAKEN;
	}

	//loop through all the branch instructions
	for (pair<unsigned long long, string> branch : branches) {
		unsigned long long addr = branch.first;
		string behavior = branch.second;
		//get last 11 bits of address
		int last_n_bits = (addr & ((1 << 11) - 1));
		unsigned int index = (last_n_bits ^ ghr) % 2048;
		//if prediction is taken
		if (predictor_table[index] == STRONGLY_TAKEN || predictor_table[index] == WEAKLY_TAKEN) {
			//our prediction was correct
			if (behavior == "T") {
				//if prediction was weakly taken, set to strongly taken
				if (predictor_table[index] == WEAKLY_TAKEN) {
					(predictor_table[index])+=1;
				}
				num_correct++;
				ghr = ((ghr << 1) | 1) & ((1 << (ghr_bits)) - 1);

			}
			//if our prediction was incorrect, set ST to WT or WT to WNT
			else {
				(predictor_table[index])-=1;
				ghr = (ghr << 1) & ((1 << (ghr_bits)) - 1);

			}
		}
		//if prediction is not taken
		else {
			//our prediction was correct
			if (behavior == "NT") {
				//if prediction was weakly not taken, set to strongly not taken
				if (predictor_table[index] == WEAKLY_NOT_TAKEN) {
					(predictor_table[index])-=1;
				}
				num_correct++;
				ghr = (ghr << 1) & ((1 << (ghr_bits)) - 1);

			}
			//if our prediction was incorrect, set SNT to WNT or WNT to WT
			else {
				(predictor_table[index])+=1;
				ghr = ((ghr << 1) | 1) & ((1 << (ghr_bits)) - 1);

			}
			
		}
	}
	return num_correct;
}


long bimodal_predictor2(vector<pair<unsigned long long, string>> branches, int table_size) {
	//make predictor table of size specified in parameter
	int predictor_table[table_size];
	long num_correct = 0;

	//set all initial values in predictor table to strongly taken (3)
	for (int i = 0; i < table_size; i++) {
		predictor_table[i] = STRONGLY_TAKEN;
	}

	//loop through all the branch instructions
	for (pair<unsigned long long, string> branch : branches) {
		unsigned long long addr = branch.first;
		string behavior = branch.second;
		//get last n bits of address depending on size of table
		int last_n_bits = (addr & ((1 << ((int)log2(table_size))) - 1));
		//if prediction is taken
		if (predictor_table[last_n_bits % table_size] == STRONGLY_TAKEN || predictor_table[last_n_bits % table_size] == WEAKLY_TAKEN) {
			//our prediction was correct
			if (behavior == "T") {
				//if prediction was weakly taken, set to strongly taken
				if (predictor_table[last_n_bits % table_size] == WEAKLY_TAKEN) {
					(predictor_table[last_n_bits % table_size])+=1;
				}
				num_correct++;
			}
			//if our prediction was incorrect, set ST to WT or WT to WNT
			else {
				(predictor_table[last_n_bits % table_size])-=1;
			}
		}
		//if prediction is not taken
		else {
			//our prediction was correct
			if (behavior == "NT") {
				//if prediction was weakly not taken, set to strongly not taken
				if (predictor_table[last_n_bits % table_size] == WEAKLY_NOT_TAKEN) {
					(predictor_table[last_n_bits % table_size])-=1;
				}
				num_correct++;
			}
			//if our prediction was incorrect, set SNT to WNT or WNT to WT
			else {
				(predictor_table[last_n_bits % table_size])+=1;

			}

		}
	}
	return num_correct;
}


long bimodal_predictor1(vector<pair<unsigned long long, string>> branches, int table_size) {
	//make predictor table of size specified in parameter
	int predictor_table[table_size];
	long num_correct = 0;

	//set all initial values in predictor table to taken (1)
	for (int i = 0; i < table_size; i++) {
		predictor_table[i] = TAKEN;
	}

	//loop through all the branch instructions
	for (pair<unsigned long long, string> branch : branches) {
		unsigned long long addr = branch.first;
		string behavior = branch.second;
		//get last n bits of address depending on size of table
		int last_n_bits = (addr & ((1 << ((int)log2(table_size))) - 1));

		//if prediction is taken
		if (predictor_table[last_n_bits % table_size] == TAKEN) {
			//if actual outcome was taken, our prediction was correct
			if (behavior == "T") {
				num_correct++;
			}
			//otherwise switch the prediction
			else {
				predictor_table[last_n_bits % table_size] = NOT_TAKEN;
			}
		}
		//if prediction is not taken
		else {
			//if actual outcome was not taken, our prediction was correct
			if (behavior == "NT") {
				num_correct++;
			}
			//otherwise switch the prediction
			else {
				predictor_table[last_n_bits % table_size] = TAKEN;

			}

		}
	}
	return num_correct;
}
int main(int argc, char *argv[]) {
  // Temporary variables
  unsigned int num_branches = 0;
  unsigned int num_taken = 0;
  unsigned int num_not_taken = 0;
  unsigned long long addr;
  string behavior, line;
  unsigned long long target;
  vector<pair<unsigned long long, string>> branches;
  vector<unsigned long long> targets;

  // Open file for reading
  ifstream infile(argv[1]);
  // The following loop will read a line at a time
  while(getline(infile, line)) {
	  num_branches++;
    // Now we have to parse the line into it's two pieces
    stringstream s(line);
    s >> std::hex >> addr >> behavior >> std::hex >> target;
    // Now we can output it
    //cout  << addr;
   // cout << "last 4 bits: " << (addr & ((1 << ((int)log2(16))) - 1));
    if(behavior == "T") {
      //cout << " -> taken, ";
      num_taken++;
    }else {
      //cout << " -> not taken, ";
      num_not_taken++;
    }
    branches.push_back(make_pair(addr, behavior));
    targets.push_back(target);
	//cout << "target=" << target << endl;
  }

  ofstream outfile;
  outfile.open(argv[2]);
  //part 1: always taken
  outfile << num_taken << "," << num_branches << ";\n";

  //part 2: always not taken
  outfile << num_not_taken << "," << num_branches << ";\n";

  //part 3: bimodal predictor with single bit of history
  outfile << bimodal_predictor1(branches, 16) << "," << num_branches << "; " << bimodal_predictor1(branches, 32) << "," << num_branches << "; " << bimodal_predictor1(branches, 128) << "," << num_branches << "; " << bimodal_predictor1(branches, 256) << "," << num_branches << "; " << bimodal_predictor1(branches, 512) << "," << num_branches << "; " << bimodal_predictor1(branches, 1024) << "," << num_branches << "; " << bimodal_predictor1(branches, 2048) << "," << num_branches << ";\n";

  //part 4: bimodal predictor with 2-bit saturating counters
outfile << bimodal_predictor2(branches, 16) << "," << num_branches << "; " << bimodal_predictor2(branches, 32) << "," << num_branches << "; " << bimodal_predictor2(branches, 128) << "," << num_branches << "; " << bimodal_predictor2(branches, 256) << "," << num_branches << "; " << bimodal_predictor2(branches, 512) << "," << num_branches << "; " << bimodal_predictor2(branches, 1024) << "," << num_branches << "; " << bimodal_predictor2(branches, 2048) << "," << num_branches << ";\n";

  //part 5: gshare predictor
outfile << gshare_predictor(branches, 3) << "," << num_branches << "; " << gshare_predictor(branches, 4) << "," << num_branches << "; " << gshare_predictor(branches, 5) << "," << num_branches << "; " << gshare_predictor(branches, 6) << "," << num_branches << "; " << gshare_predictor(branches, 7) << "," << num_branches << "; " << gshare_predictor(branches, 8) << "," << num_branches << "; " << gshare_predictor(branches, 9) << "," << num_branches << "; " << gshare_predictor(branches, 10) << "," << num_branches << "; " << gshare_predictor(branches, 11) << "," << num_branches << ";\n";

 //part 6: tournament predictor
outfile << tournament_predictor(branches) << "," << num_branches << ";\n";

//part 7: branch target buffer
outfile << branch_target_buffer(branches, targets).second <<  "," << branch_target_buffer(branches, targets).first << ";\n";

outfile.close();
  return 0;
}
