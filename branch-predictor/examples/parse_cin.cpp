/* Example
 * Parsing Branch Traces
 */

#include<iostream>
#include<fstream>
#include<string>

using namespace std;

int main(int argc, char *argv[]) {

  // Temporary variables
  unsigned long long addr;
  string behavior;
  unsigned long long target;

  // Open file for reading
  ifstream infile("test_input.txt");

  // The following loop will read a hexadecimal number and
  // a string each time and then output them
  while(infile >> std::hex >> addr >> behavior >> std::hex >> target) {
    cout << addr;
    if(behavior == "T") {
      cout << " -> taken, ";
    }else {
      cout << " -> not taken, ";
    }
	cout << "target=" << target << endl;
  }

  return 0;
}
