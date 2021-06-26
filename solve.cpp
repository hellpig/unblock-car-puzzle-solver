/*
  This code solves "Unblock Me" or "Sliding Cars" sort of puzzles.
  This code cannot handle oddly shaped cars.

  First, change the parameters at the top of this file.

  Then, compile and run this code via...
    g++ -O3 solve.cpp
    ./a.out

  To animate the results (saved to output.txt), then run...
    ./animate.py
  Close the animation figure to enter interactive mode!

  If on Windows, setup Mingw-w64 or Cygwin, then do...
    g++ -O3 solve.cpp
    a.exe
    python3 animate.py

  (c) 2021 Bradley Knockel
*/


#include <iostream>   // std::cout, std::endl, std::flush
#include <iomanip>    // std::setw()
#include <fstream>    // std::ofstream
#include <chrono>     // for timing the code
#include <vector>     // for variable-length (on heap) 1D arrays



/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// parameters to set /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

    /*
        0 -> display first solution found
        1 -> keep searching for the optimal solution
    */
    const int searchForOptimal = 1;


    /*
        0 -> count every move
        1 -> sequential moves of the same car collectively count as 1 move
    */
    const int smartCount = 1;


    /*
        Enter max number of moves allowed in search for a solution.
        Change to something like 100 if searchForOptimal equals 0.

        You will probably never need to change this,
          though setting this well speeds up the code!
        You *might* need to change this if...
         - the puzzle is very difficult, requiring a larger nMax
         - the puzzle is sparsely filled with cars, requiring a smaller nMax
    */
    int nMax = 60;


    /*
        Insert level

        Use -1 for immovable walls, 0 for empty, and positive integers for cars.
        Each car must have its own unique positive integer 1 to number of cars.

        Note that this array will have 1 subtracted from each value before being fed into move()
    */
    int lStart[] = {1,  1,  0,  2,  2,  2,
                    0,  0,  0, 10,  3,  3,
                    8,  4,  4, 10,  0, 11,
                    8,  0,  9,  5,  5, 11,
                    6,  6,  9,  0,  0, 11,
                    0,  0,  9,  7,  7,  7};



    /* set according to lStart[] */
    const int size[] = {6, 6};   // {rows, columns}
    #define length 36
    #define cars 11



    /*
        [row column car]
        Keep in mind that first row or first column is 0
        Note that this array will be modified before it is used
    */
    int winCondition[] = {2, 5, 4};


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// end of parameters /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////



/* create struct in order to pass arrays by value */
struct wrapper{
  int l[length];
  int s[cars];
};


/* make a few more things global */
int carDataInitial[cars][5];  // 5 columns: {index, row, column, vertical=1 (else 0), length}
std::vector<int> wallData;    // indices of walls
std::vector<int> sol;         // current best solution
const int carsPlus1 = cars + 1;





/*
  A macro to be used several times in move()
  Moves car i (do=1 if both directions should be tried, else do=0)
*/

#define tryy(do) ({\
    if (carDataInitial[i][3]) {          /* vertical */    \
        if ( (do || d==1) && carDataInitial[i][1]-wrapped.s[i]-1 >= 0 && wrapped.l[carDataInitial[i][0] - (wrapped.s[i]+1)*size[1]] == -1 )\
            o = move(wrapped,o,p,n,i,1);  /* move up */    \
        if ( (do || d==2) && carDataInitial[i][1]-wrapped.s[i]+carDataInitial[i][4] < size[0] && wrapped.l[carDataInitial[i][0] + (carDataInitial[i][4]-wrapped.s[i])*size[1]] == -1 )\
            o = move(wrapped,o,p,n,i,2);  /* move down */  \
    } else {\
        if ( (do || d==3) && carDataInitial[i][2]+wrapped.s[i]-1 >= 0 && wrapped.l[carDataInitial[i][0] + wrapped.s[i] - 1] == -1 )\
            o = move(wrapped,o,p,n,i,3);  /* move left */  \
        if ( (do || d==4) && carDataInitial[i][2]+wrapped.s[i]+carDataInitial[i][4] < size[1] && wrapped.l[carDataInitial[i][0] + wrapped.s[i] + carDataInitial[i][4]] == -1 )\
            o = move(wrapped,o,p,n,i,4);  /* move right */ \
    }\
})




/*
 move() is the recursive function that solves the puzzle
 returns o

 wrapped.l[] is the level array
 wrapped.s[] is position array of all cars relative to variable carDataInitial[] (up or right is positive)

 o[] is a vector where each group of (cars + 1) elements is an old s[] with its corresponding n attached
 p[] is a vector where each group of 2 elements is {c, d}; will eventually be used to make sol[]

 n is current step
 c is which car is being moved (-1 = start)
 d is which direction the car is being moved
     1=up 2=down 3=left 4=right 0=start
*/

std::vector<int> move(wrapper wrapped, std::vector<int> o, std::vector<int> p, int n, int c, int d) {

  // update wrapped
  if (d==1) {
    wrapped.s[c]++;
    wrapped.l[carDataInitial[c][0] - wrapped.s[c]*size[1]] = c;
    wrapped.l[carDataInitial[c][0] + (carDataInitial[c][4] - wrapped.s[c])*size[1]] = -1;
  } else if (d==2) {
    wrapped.s[c]--;
    wrapped.l[carDataInitial[c][0] + (carDataInitial[c][4] - wrapped.s[c] - 1)*size[1]] = c;
    wrapped.l[carDataInitial[c][0] - (wrapped.s[c] + 1)*size[1]] = -1;
  } else if (d==3) {
    wrapped.s[c]--;
    wrapped.l[carDataInitial[c][0] + wrapped.s[c]] = c;
    wrapped.l[carDataInitial[c][0] + carDataInitial[c][4] + wrapped.s[c]] = -1;
  } else if (d==4) {
    wrapped.s[c]++;
    wrapped.l[carDataInitial[c][0] + carDataInitial[c][4] + wrapped.s[c] - 1] = c;
    wrapped.l[carDataInitial[c][0] + wrapped.s[c] - 1] = -1;
  }

  // going down roads already traveled is a waste of time
  int go = 1;   // go=0 allows for certain roads to still be followed as opposed to using the return statement
  if (d) {

    // search for another time the same s[] has occurred
    int ind = 0;
    for (int j=0; j < o.size()/carsPlus1; j++) {
        int stop = 0;
        for (int k=0; k < cars; k++) {
           if (wrapped.s[k] != o[j*carsPlus1 + k]) {
              stop = 1;
              break;
           }
        }
        if (stop) continue;
        ind = j*carsPlus1 + cars;
        break;
    }

    if (ind) {
      int N = o[ind];

      if (n>N)
          return o;
      if (n==N) {
          if (smartCount)
              go = 0;
          else
              return o;
      }

      // replace the entry in o[]
      o[ind] = n;

    } else {

      // make o[] longer
      o.reserve(o.size() + carsPlus1);
      for (int j=0; j<cars; j++)
          o.push_back(wrapped.s[j]);
      o.push_back(n);
    }

  }

  // check if you've won!
  p.push_back(c);
  p.push_back(d);
  if (wrapped.l[winCondition[0]] == winCondition[1]) {

    // sol[] becomes all but the first two elements of p[]
    sol.clear();
    sol.reserve(p.size() - 2);
    for (int j=2; j<p.size(); j++)
        sol.push_back(p[j]);

    nMax = n-1;
    if (!searchForOptimal)
        nMax = 0;
    std::cout << "\b\b\b\b\b" << std::setw(5) << n << std::flush;
    return o;
  }

  // try to keep moving same car in the same direction
  int i = c;
  if (smartCount && c >= 0) {
      tryy(0);
  }
  if (!go) return o;

  // check if we should give up
  n++;
  if (n > nMax) return o;

  // try to keep moving same car in the same direction
  if (c >= 0 && !smartCount) {
      tryy(0);
  }

  // move other cars in all directions
  for (i = 0; i<cars; i++) {
    if (i!=c) {
        tryy(1);
    }
  }

  return o;
}









int main() {



  /**************
      parse level info
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

  // the following are needed to check that lStart[] is nice
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

                // get lengt and update usedIndices[]
                int lengt = 2;
                usedIndices.push_back(i+size[1]);
                int j = i + 2*size[1];
                while (j < length && lStart[j] == lStart[i]) {
                  usedIndices.push_back(j);
                  j += size[1];
                  lengt++;
                }

                carDataInitial[lStart[i]-1][3] = 1;
                carDataInitial[lStart[i]-1][4] = lengt;

      } else if (i+1<length && lStart[i+1]==lStart[i]) {   // horizontal

                // get lengt and update usedIndices[]
                int leng = (i / size[1] + 1) * size[1];  // first index of next row
                int lengt = 2;
                usedIndices.push_back(i+1);
                int j = i+2;
                while (j < leng && lStart[j] == lStart[i]) {
                  usedIndices.push_back(j);
                  j++;
                  lengt++;
                }

                carDataInitial[lStart[i]-1][3] = 0;
                carDataInitial[lStart[i]-1][4] = lengt;

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





  /**************
      Do it!
      And return the number of different configurations explored
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

  // make some crap for move()
  std::vector<int> temp(carsPlus1, 0);
  std::vector<int> temp2;
  wrapper wrapped;
  for (int i=0; i<length; i++)
    wrapped.l[i] = lStart[i] - 1;  // subtract 1 so that car label matches index of carDataInitial[]
  for (int i=0; i<cars; i++)
    wrapped.s[i] = 0;

  // put winCondition[] in a better format for move()
  winCondition[0] = winCondition[0] * size[1] + winCondition[1];
  winCondition[1] = winCondition[2] - 1;

  // run and time move()
  std::chrono::high_resolution_clock::time_point timeStart = std::chrono::high_resolution_clock::now();
  temp = move(wrapped, temp, temp2, 0, -1, 0);
  std::chrono::high_resolution_clock::time_point timeEnd = std::chrono::high_resolution_clock::now();

  // print results contained in sol[]
  std::cout << '\n';
  int solRows = sol.size()/2;
  for (int i=0; i < solRows; i++)
    std::cout << std::setw(5) << sol[2*i] + 1 << std::setw(3) << sol[2*i + 1] << '\n';   // add 1 so that car labels again match lStart[]
  std::cout << "Elapsed time is " << std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart).count() << " sec" << std::endl;

  // use temp[] lol
  std::cout << "Number of unique configurations explored: " << temp.size() / carsPlus1 << std::endl;

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

  std::ofstream myfile;
  myfile.open("output.txt");

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
