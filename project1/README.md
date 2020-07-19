# Branch Prediction Simulator

### The goal of this project is to measure the effectiveness of various branch direction prediction (“taken” or “non-taken”) schemes on several traces of conditional branch instructions. Each trace contains a large number of branch instructions, and for each branch, the program counter (expressed as a word address) and the actual outcome of the branch are recorded in one line of the trace file. Several trace files are provided for evaluating the predictor designs.
### The goal is to write a program in C or C++ that would use these traces to measure the accuracy of various dynamic branch predictors that we studied in class. The branch outcomes from the trace file should be used to train the predictors.

#### The following predictors have been implemented: 
1. Always Taken
2. Always Non-Taken
3. Bimodal Predictor with a single bit of history
4. Bimodal Precitor with 2-bit saturating counters
5. Gshare Predictor
6. Tournament Predictor
7. Branch Target Buffer

To run the file on the command line:
```
make
./predictors [input_trace.txt] [output.txt]
```
where input_trace.txt is the file containing branch trace and output.txt is the file to place output statistics.
