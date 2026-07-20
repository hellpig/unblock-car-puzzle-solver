/*
  This code solves puzzles similar to "Unblock Me" and "Sliding Cars".
  This code cannot handle oddly shaped cars.

  First, change the parameters at the top of this file.

  Then, compile and run this code via...
    g++ -std=c++17 -O3 solve.cpp
    ./a.out

  To animate the results saved in output.txt, run...
    ./animate.py
  Close the animation figure to enter interactive mode!

  On Windows, set up MinGW-w64 or Cygwin, then run...
    g++ -std=c++17 -O3 solve.cpp
    .\a.exe
    python animate.py

  (c) 2021 Bradley Knockel
*/


#include <iostream>   // std::cout, std::endl, std::flush, std::fixed
#include <iomanip>    // std::setw(), std::setprecision
#include <fstream>    // std::ofstream
#include <chrono>     // for timing the code
#include <vector>     // for variable-length (on heap) 1D arrays
#include <array>

// We want hash tables to greatly speed up uniqueConfigurations!
//   In 2026, ChatGPT gave me this idea!
#include <unordered_map>
#include <functional>


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// parameters to set /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    /*
        0 -> stop after finding the first solution
        1 -> keep searching for the optimal solution
    */
    const int searchForOptimal = 1;


    /*
        0 -> count every move
        1 -> sequential moves of the same car collectively count as 1 move
    */
    const int smartCount = 1;


    /*
        Enter the maximum number of moves allowed in the search.
        Change to something like 100 if searchForOptimal equals 0.

        You will probably never need to change this,
          although choosing an appropriate value can significantly reduce the search time.
        You *might* need to change this if...
         - the puzzle is very difficult, requiring a larger nMax
         - the puzzle is sparsely filled with cars, requiring a smaller nMax
    */
    int nMax = 60;


    /*
        Insert level

        Use -1 for immovable walls, 0 for empty, and positive integers for cars.
        Number the cars consecutively from 1 through cars, using each number for exactly one car.

        Note: Before the search, 1 is subtracted from every cell so that each car's
          internal label matches its zero-based index in carDataInitial[].
    */
/*
    int lStart[] = {1,  1,  0,  2,  2,  2,
                    0,  0,  0, 10,  3,  3,
                    8,  4,  4, 10,  0, 11,
                    8,  0,  9,  5,  5, 11,
                    6,  6,  9,  0,  0, 11,
                    0,  0,  9,  7,  7,  7};   // win condition is {2, 5, 4}
*/
    int lStart[] = {1,  1,  0,  5,  0,  6,
                    7,  8,  0,  5,  0,  6,
                    7,  8,  2,  2,  9, 10,
                   11,  3,  3, 12,  9, 10,
                   11,  0, 13, 12,  9,  0,
                   11,  0, 13,  4,  4,  0};   // win condition is {2, 5, 2}


    /* set according to lStart[] */
    const int size[] = {6, 6};   // {rows, columns}
    #define length 36
    #define cars 13



    /*
        {row, column, car}
        The specified edge cell must be occupied by the specified car to win.
        Rows and columns use zero-based indexing.
        Note that this array will be modified before it is used
    */
    int winCondition[] = {2, 5, 2};


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// end of parameters /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////



/* define a struct to conceptually group different ways of representing the puzzle state */
struct PuzzleState {
  std::array<int, length> l;   // could do `int l[length]` instead since PuzzleState would allow it to be passed by value
  std::array<int, cars> s;
};



/* Data shared by move(), the recursive search */

int carDataInitial[cars][5];  // 5 columns: {index, row, column, vertical=1 (else 0), length}
std::vector<int> wallData;    // indices of walls
std::vector<int> sol;         // current best solution

// uniqueConfigurations maps each previously encountered s[] state to the
//   smallest move count (n) at which that state has been reached.
//   s[] is defined in move().
// ChatGPT wrote this in 2026.

struct StateHash {
  std::size_t operator()(const std::array<int, cars>& state) const noexcept {
    std::size_t hash = 0;
    for (int position : state)
      hash = hash * 31 + std::hash<int>{}(position);
    return hash;
  }
};

std::unordered_map<std::array<int, cars>, int, StateHash> uniqueConfigurations;





/*
  The function tryMoves() will be used several times in move()
  Moves car i (tryBoth=1 if both directions should be tried, else tryBoth=0)
*/

void move(PuzzleState puzzleState, std::vector<int> p, int n, int c, int d);

inline void tryMoves(const PuzzleState& puzzleState, const std::vector<int>& p, int n, int i, int d, bool tryBoth) {

  if (carDataInitial[i][3]) {  // vertical
    if ((tryBoth || d == 1) && carDataInitial[i][1] - puzzleState.s[i] - 1 >= 0 && puzzleState.l[carDataInitial[i][0] - (puzzleState.s[i] + 1) * size[1]] == -1)
      move(puzzleState, p, n, i, 1);  // move up
    if ((tryBoth || d == 2) && carDataInitial[i][1] - puzzleState.s[i] + carDataInitial[i][4] < size[0] && puzzleState.l[carDataInitial[i][0] + (carDataInitial[i][4] - puzzleState.s[i]) * size[1]] == -1)
      move(puzzleState, p, n, i, 2);  // move down
  } else {  // horizontal
    if ((tryBoth || d == 3) && carDataInitial[i][2] + puzzleState.s[i] - 1 >= 0 && puzzleState.l[carDataInitial[i][0] + puzzleState.s[i] - 1] == -1)
      move(puzzleState, p, n, i, 3);  // move left
    if ((tryBoth || d == 4) && carDataInitial[i][2] + puzzleState.s[i] + carDataInitial[i][4] < size[1] && puzzleState.l[carDataInitial[i][0] + puzzleState.s[i] + carDataInitial[i][4]] == -1)
      move(puzzleState, p, n, i, 4);  // move right
  }
}




/*
 move() is the recursive function that solves the puzzle

 puzzleState.l[] is the level array
 puzzleState.s[] is the position array of all cars relative to carDataInitial[]
   (up and right are positive; down and left are negative)

 p[] is a vector where each group of 2 elements is {c, d}; will eventually be used to make sol[]

 n is the current move count under the selected counting rule
 c identifies the car being moved (-1 = start)
 d identifies the car's movement direction
     1=up 2=down 3=left 4=right 0=start
*/

void move(PuzzleState puzzleState, std::vector<int> p, int n, int c, int d) {

  // update puzzleState
  if (d==1) {
    puzzleState.s[c]++;
    puzzleState.l[carDataInitial[c][0] - puzzleState.s[c]*size[1]] = c;
    puzzleState.l[carDataInitial[c][0] + (carDataInitial[c][4] - puzzleState.s[c])*size[1]] = -1;
  } else if (d==2) {
    puzzleState.s[c]--;
    puzzleState.l[carDataInitial[c][0] + (carDataInitial[c][4] - puzzleState.s[c] - 1)*size[1]] = c;
    puzzleState.l[carDataInitial[c][0] - (puzzleState.s[c] + 1)*size[1]] = -1;
  } else if (d==3) {
    puzzleState.s[c]--;
    puzzleState.l[carDataInitial[c][0] + puzzleState.s[c]] = c;
    puzzleState.l[carDataInitial[c][0] + carDataInitial[c][4] + puzzleState.s[c]] = -1;
  } else if (d==4) {
    puzzleState.s[c]++;
    puzzleState.l[carDataInitial[c][0] + carDataInitial[c][4] + puzzleState.s[c] - 1] = c;
    puzzleState.l[carDataInitial[c][0] + puzzleState.s[c] - 1] = -1;
  }

  // Avoid re-exploring configurations reached with an equal or lower move count.
  int go = 1;   // go=0 allows for certain roads to still be followed as opposed to using the return statement
  if (d) {

    // ChatGPT updated the following in 2026 to use a hash table.
    // A newly encountered state is inserted. If the same state is later reached
    // with a smaller n, its stored move count is updated.

    auto [it, inserted] = uniqueConfigurations.emplace(puzzleState.s, n);

    if (!inserted) {  // puzzleState.s was encountered previously
      int N = it->second;

      if (n > N)
        return;

      if (n == N) {
        if (smartCount)
          go = 0;
        else
          return;
      }

      it->second = n;
    }

  }

  // check if you've won!
  p.push_back(c);
  p.push_back(d);
  if (puzzleState.l[winCondition[0]] == winCondition[1]) {

    // sol[] becomes all but the first two elements of p[]
    sol.clear();
    sol.reserve(p.size() - 2);
    for (int j=2; j<p.size(); j++)
        sol.push_back(p[j]);

    nMax = n-1;
    if (!searchForOptimal)
        nMax = 0;
    std::cout << "\b\b\b\b\b" << std::setw(5) << n << std::flush;
    return;
  }

  // try to keep moving same car in the same direction
  int i = c;
  if (smartCount && c >= 0) {
      tryMoves(puzzleState, p, n, i, d, false);
  }
  if (!go) return;

  // check if we should give up
  n++;
  if (n > nMax) return;

  // try to keep moving same car in the same direction
  if (c >= 0 && !smartCount) {
      tryMoves(puzzleState, p, n, i, d, false);
  }

  // move other cars in all directions
  for (i = 0; i<cars; i++) {
    if (i!=c) {
        tryMoves(puzzleState, p, n, i, d, true);  // notice that true is now passed
    }
  }

  return;
}









int main() {



  /**************
      parse level info
      I don't try to catch all invalid parameters!
  **************/

  if (size[0]*size[1] != length || length != sizeof(lStart)/sizeof(lStart[0])) {
    std::cout << "Error: incorrect level dimensions" << std::endl;
    return -1;
  }
  for (int i=0; i<length; i++) {
    if (lStart[i] < -1 || lStart[i] > cars) {
      std::cout << "Error: level data has invalid integers" << std::endl;
      return -1;
    }
  }

  // track parsed cells and car labels while validating lStart[]
  std::vector<int> usedIndices;
  usedIndices.reserve(length);
  std::vector<int> usedCars;
  usedCars.reserve(cars);

  // create wallData[] and carDataInitial[]
  for (int i=0; i<length; i++) {

    int go = 1;
    for (int j=0; j<usedIndices.size(); j++) {
      if (i==usedIndices[j]) {
        go = 0;
        break;
      }
    }
    if (!go) continue;

    if (lStart[i]>0) {

      for (int j=0; j<usedCars.size(); j++) {
        if (usedCars[j]==lStart[i]) {
                std::cout << "Error: cars have invalid shape" << std::endl;
                return -1;
        }
      }
      usedCars.push_back(lStart[i]);

      carDataInitial[lStart[i]-1][0] = i;
      carDataInitial[lStart[i]-1][1] = i / size[1];
      carDataInitial[lStart[i]-1][2] = i % size[1];

      if (i+size[1]<length && lStart[i+size[1]]==lStart[i]) {  // vertical

                // get carLength and update usedIndices[]
                int carLength = 2;
                usedIndices.push_back(i+size[1]);
                int j = i + 2*size[1];
                while (j < length && lStart[j] == lStart[i]) {
                  usedIndices.push_back(j);
                  j += size[1];
                  carLength++;
                }

                carDataInitial[lStart[i]-1][3] = 1;
                carDataInitial[lStart[i]-1][4] = carLength;

      } else if (i+1<length && lStart[i+1]==lStart[i]) {   // horizontal

                // get carLength and update usedIndices[]
                int nextRowIndex = (i / size[1] + 1) * size[1];  // first index of next row
                int carLength = 2;
                usedIndices.push_back(i+1);
                int j = i+2;
                while (j < nextRowIndex && lStart[j] == lStart[i]) {
                  usedIndices.push_back(j);
                  j++;
                  carLength++;
                }

                carDataInitial[lStart[i]-1][3] = 0;
                carDataInitial[lStart[i]-1][4] = carLength;

      } else {
                std::cout << "Error: cars have invalid shape!" << std::endl;
                return -1;
      }

    } else if (lStart[i]==-1) {
              wallData.push_back(i);
    }

  }

  // check winCondition[]
  if (carDataInitial[winCondition[2] - 1][3]) {  // vertical
    if ( carDataInitial[winCondition[2] - 1][2] != winCondition[1] || !(winCondition[0] == 0 || winCondition[0] == size[0]-1) ) {
       std::cout << "Error: winCondition[] is invalid" << std::endl;
       return -1;
    }
  } else {
    if ( carDataInitial[winCondition[2] - 1][1] != winCondition[0] || !(winCondition[1] == 0 || winCondition[1] == size[1]-1) ) {
       std::cout << "Error: winCondition[] is invalid" << std::endl;
       return -1;
    }
  }

  // reject a level that already satisfies the win condition
  if (lStart[winCondition[0] * size[1] + winCondition[1]] == winCondition[2]) {
    std::cout << "Error: the level already satisfies winCondition[]" << std::endl;
    return -1;
  }




  /**************
      Do it!
      And print the number of different configurations explored
  **************/

  if (searchForOptimal) {
    std::cout << "\nOptimal solution will be output soon.\n"
       << "Each row is [car, direction].\n"
       << "For the direction: 1=up, 2=down, 3=left, and 4=right.\n"
       << "When the code is finished, the following number will be correct.\n"
       << "Number of moves in optimal solution:          "
       << std::flush;
  } else {
    std::cout << "\nSolution will be output soon.\n"
       << "Each row is [car, direction].\n"
       << "For the direction: 1=up, 2=down, 3=left, and 4=right.\n"
       << "Number of moves in solution:          "
       << std::flush;
  }

  // make some crap for move(): temp[] and puzzleState
  std::vector<int> temp;
  PuzzleState puzzleState;
  for (int i=0; i<length; i++)
    puzzleState.l[i] = lStart[i] - 1;  // subtract 1 so that car label matches index of carDataInitial[]
  for (int i=0; i<cars; i++)
    puzzleState.s[i] = 0;

  // put winCondition[] in a better format for move()
  winCondition[0] = winCondition[0] * size[1] + winCondition[1];
  winCondition[1] = winCondition[2] - 1;

  // try to speed things up
  uniqueConfigurations.reserve(10000);

  // run and time move()
  std::chrono::high_resolution_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
  move(puzzleState, temp, 0, -1, 0);
  std::chrono::high_resolution_clock::time_point timeEnd = std::chrono::high_resolution_clock::now();

  // print results contained in sol[]
  std::cout << std::fixed << std::setprecision(3);  // floats now always print 3 decimal values; a persistent change, unlike std::setw()
  std::cout << '\n';
  int solRows = sol.size()/2;
  for (int i=0; i < solRows; i++)
    std::cout << std::setw(5) << sol[2*i] + 1    // add 1 when printing so that car labels again match lStart[]
        << std::setw(3) << sol[2*i + 1] << '\n';
  std::cout << "Elapsed time is "
      << std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart).count()/1000.0
      << " s" << std::endl;

  std::cout << "Number of unique configurations explored: " << uniqueConfigurations.size() << std::endl;

  if (!solRows) {
    std::cout << "No solution found. Perhaps you should make nMax larger." << std::endl;
    return -1;
  }





  /**************
      Output to file so that Python can animate it!
  **************/

  // massage sol into a more convenient form: triplets of {car, direction, count}

  std::vector<int> solution;
  solution.reserve(solRows * 3);

  // create solution[]
  if (smartCount) {

    // create same[]
    std::vector<int> same;
    same.reserve(solRows);
    same.push_back(0);
    for (int i=1; i<solRows; i++) {
        if ( sol[2*i]==sol[2*(i-1)] && sol[2*i + 1]==sol[2*(i-1) + 1] )
          same.push_back(1);
        else
          same.push_back(0);
    }

    int j = -1;  // j is the current triplet of solution[]
    for (int i=0; i<solRows; i++) {
        if (same[i])
            solution[3*j + 2]++;
        else {
            j++;
            solution.push_back(sol[i*2]);
            solution.push_back(sol[i*2 + 1]);
            solution.push_back(1);   // starts as 1 but can increase
        }
    }

  } else {
    for (int i=0; i<solRows; i++) {
      solution.push_back(sol[2*i]);
      solution.push_back(sol[2*i + 1]);
      solution.push_back(1);
    }
  }


  // output to file

  std::ofstream myfile("output.txt");
  if (!myfile) {
    std::cout << "Error: could not open output.txt for writing" << std::endl;
    return -1;
  }

  // size[]
  myfile << "[" << size[0] << "," << size[1] << "]\n";

  // cars
  myfile << cars << "\n";

  // carDataInitial[]
  for (int i=0; i<cars; i++)
      myfile << "[" << carDataInitial[i][0] << "," << carDataInitial[i][1]
          << "," << carDataInitial[i][2] << "," << carDataInitial[i][3] << ","
          << carDataInitial[i][4] << "]\n";

  // solution[]
  myfile << "[" << solution[0];
  for (int i=1; i<solution.size(); i++)
      myfile << "," << solution[i];
  myfile << "]\n";

  // wallData[]
  myfile << "[";
  if (wallData.size()) {
    myfile << wallData[0];
    for (int i=1; i<wallData.size(); i++)
        myfile << "," << wallData[i];
  }
  myfile << "]\n";

  myfile.close();




  return 0;

}
