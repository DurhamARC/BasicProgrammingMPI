#include <stdio.h>
#include "mandel.h"
#include <omp.h>

int main(){

  #pragma omp parallel
  {
    #pragma omp single
    printf("running with %d threads\n", omp_get_num_threads());
  }

  //let's test 100 numbers from -1 to 1 in each dimension

  const int maxIterations = 500;

  int totalNumbersTested   = 0; //should be 500 * 500
  int totalNumbersInMandel = 0;

  const double xStart = -1;
  const double xEnd   =  1;
  const double yStart = -1;
  const double yEnd   =  1;

  const int numbersToTestInEachDim = 1500;

  /*
  the compiler is not able to plan ahead when we use floats as our step
  in the for loop. so, we make an array of doubles, which we index
  with an integer. see the implementation in the header file
  */

  double xToTest[numbersToTestInEachDim];
  double yToTest[numbersToTestInEachDim];

  populateArray(xToTest, xStart, xEnd, numbersToTestInEachDim);
  populateArray(yToTest, yStart, yEnd, numbersToTestInEachDim);

  //master region to issue the tasks
  #pragma omp parallel reduction(+:totalNumbersTested, totalNumbersInMandel)
  {
    #pragma omp master //only master thread in here
    {
      for (int i = 0; i < numbersToTestInEachDim; i++){
        for (int j = 0; j < numbersToTestInEachDim; j++){

          //use our pre-written function to test if this number is in the mandelBrot set or not
          #pragma omp task shared(xToTest, yToTest, maxIterations, i, j, totalNumbersInMandel, totalNumbersTested) default(none)
          {
            bool isXYInMandelBrotSet = isMandelBrot(xToTest[i], yToTest[j], maxIterations);
            if (isXYInMandelBrotSet)
              //one at a time here. can also call 
              // #pragma omp atomic
              #pragma omp critical
              totalNumbersInMandel++;

            //increment our counter
            //one at a time here. can also call 
            // #pragma omp atomic
            #pragma omp critical
            totalNumbersTested++;
          }
        }
      }
    }
  }

  double percentage = 100 * totalNumbersInMandel / totalNumbersTested;
  printf("finished. we tested %d complex numbers, and %d were mandel numbers. this is %.2f percent\n", totalNumbersTested, totalNumbersInMandel, percentage);
}