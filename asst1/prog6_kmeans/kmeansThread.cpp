#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <immintrin.h>
#include <omp.h>

#include "CycleTimer.h"

using namespace std;

const int maxThreads=32;

typedef struct {
  // Control work assignments
  int start, end;

  // Shared by all functions
  double *data;
  double *clusterCentroids;
  int *clusterAssignments;
  double *currCost;
  int M, N, K,numThreads,threadId,mstart,mend;
} WorkerArgs;


/**
 * Checks if the algorithm has converged.
 * 
 * @param prevCost Pointer to the K dimensional array containing cluster costs 
 *    from the previous iteration.
 * @param currCost Pointer to the K dimensional array containing cluster costs 
 *    from the current iteration.
 * @param epsilon Predefined hyperparameter which is used to determine when
 *    the algorithm has converged.
 * @param K The number of clusters.
 * 
 * NOTE: DO NOT MODIFY THIS FUNCTION!!!
 */
static bool stoppingConditionMet(double *prevCost, double *currCost,
                                 double epsilon, int K) {
  for (int k = 0; k < K; k++) {
    if (abs(prevCost[k] - currCost[k]) > epsilon)
      return false;
  }
  return true;
}

/**
 * Computes L2 distance between two points of dimension nDim.
 * 
 * @param x Pointer to the beginning of the array representing the first
 *     data point.
 * @param y Poitner to the beginning of the array representing the second
 *     data point.
 * @param nDim The dimensionality (number of elements) in each data point
 *     (must be the same for x and y).
 */
double dist(double *x, double *y, int nDim) {
  double accum = 0.0;
  for (int i = 0; i < nDim; i++) {
    accum += pow((x[i] - y[i]), 2);
  }
  return sqrt(accum);
}

/**
 * Assigns each data point to its "closest" cluster centroid.
 */
// void computeAssignments(WorkerArgs *const args) {
//   double *minDist = new double[args->M];
  
//   // Initialize arrays
//   for (int m =0; m < args->M; m++) {
//     minDist[m] = 1e30;
//     args->clusterAssignments[m] = -1;
//   }

//   // Assign datapoints to closest centroids
//   for (int k = args->start; k < args->end; k++) {
//     for (int m = 0; m < args->M; m++) {
//       double d = dist(&args->data[m * args->N],
//                       &args->clusterCentroids[k * args->N], args->N);
//       if (d < minDist[m]) {
//         minDist[m] = d;
//         args->clusterAssignments[m] = k;
//       }
//     }
//   }

//   free(minDist);
// }


void computeAssignmentsThreads(WorkerArgs &args) {
  const int numThreads = min(std::thread::hardware_concurrency(), (unsigned int)maxThreads);
  thread workThreads[maxThreads];
  WorkerArgs workerArgs[maxThreads];
  
  double* minDist = new double[args.M];
  // 初始化最小距离
  for(int m=0; m<args.M; m++) {
      minDist[m] = 1e30;
      args.clusterAssignments[m] = -1;
  }
  
  // 按数据点划分工作，而不是按簇划分
  for(int i=0; i<numThreads; i++) {
      workerArgs[i] = args;
      workerArgs[i].mstart = i * (args.M / numThreads);
      workerArgs[i].mend = (i == numThreads-1) ? args.M : (i+1) * (args.M / numThreads);
  }
  
  auto worker = [](WorkerArgs* args, double* minDist) {
      for(int m=args->mstart; m<args->mend; m++) {
          for(int k=0; k<args->K; k++) {
              double d = dist(&args->data[m * args->N], 
                             &args->clusterCentroids[k * args->N], args->N);
              if(d < minDist[m]) {
                  minDist[m] = d;
                  args->clusterAssignments[m] = k;
              }
          }
      }
  };
  
  for(int i=0; i<numThreads; i++) {
      workThreads[i] = thread(worker, &workerArgs[i], minDist);
  }
  
  for(int i=0; i<numThreads; i++) {
      workThreads[i].join();
  }
  
  delete[] minDist;
}


/**
 * Given the cluster assignments, computes the new centroid locations for
 * each cluster.
 */
void computeCentroids(WorkerArgs *const args) {
  int *counts = new int[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    counts[k] = 0;
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] = 0.0;
    }
  }


  // Sum up contributions from assigned examples
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] +=
          args->data[m * args->N + n];
    }
    counts[k]++;
  }

  // Compute means
  for (int k = 0; k < args->K; k++) {
    counts[k] = max(counts[k], 1); // prevent divide by 0
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] /= counts[k];
    }
  }

  free(counts);
}

/**
 * Computes the per-cluster cost. Used to check if the algorithm has converged.
 */
void computeCost(WorkerArgs *const args) {
  double *accum = new double[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    accum[k] = 0.0;
  }

  // Sum cost for all data points assigned to centroid
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    accum[k] += dist(&args->data[m * args->N],
                     &args->clusterCentroids[k * args->N], args->N);
  }

  // Update costs
  for (int k = args->start; k < args->end; k++) {
    args->currCost[k] = accum[k];
  }

  free(accum);
}

/**
 * Computes the K-Means algorithm, using std::thread to parallelize the work.
 *
 * @param data Pointer to an array of length M*N representing the M different N 
 *     dimensional data points clustered. The data is layed out in a "data point
 *     major" format, so that data[i*N] is the start of the i'th data point in 
 *     the array. The N values of the i'th datapoint are the N values in the 
 *     range data[i*N] to data[(i+1) * N].
 * @param clusterCentroids Pointer to an array of length K*N representing the K 
 *     different N dimensional cluster centroids. The data is laid out in
 *     the same way as explained above for data.
 * @param clusterAssignments Pointer to an array of length M representing the
 *     cluster assignments of each data point, where clusterAssignments[i] = j
 *     indicates that data point i is closest to cluster centroid j.
 * @param M The number of data points to cluster.
 * @param N The dimensionality of the data points.
 * @param K The number of cluster centroids.
 * @param epsilon The algorithm is said to have converged when
 *     |currCost[i] - prevCost[i]| < epsilon for all i where i = 0, 1, ..., K-1
 */
void kMeansThread(double *data, double *clusterCentroids, int *clusterAssignments,
               int M, int N, int K, double epsilon) {

  // Used to track convergence
  double *prevCost = new double[K];
  double *currCost = new double[K];

  // The WorkerArgs array is used to pass inputs to and return output from
  // functions.
  WorkerArgs args;
  args.data = data;
  args.clusterCentroids = clusterCentroids;
  args.clusterAssignments = clusterAssignments;
  args.currCost = currCost;
  args.M = M;
  args.N = N;
  args.K = K;

  // Initialize arrays to track cost
  for (int k = 0; k < K; k++) {
    prevCost[k] = 1e30;
    currCost[k] = 0.0;
  }

  /* Main K-Means Algorithm Loop */
  int iter = 0;
  double AssignmentsTime = 0;
  double CentroidsTime = 0;
  double CostTime = 0;
  while (!stoppingConditionMet(prevCost, currCost, epsilon, K)) {
    // Update cost arrays (for checking convergence criteria)
    for (int k = 0; k < K; k++) {
      prevCost[k] = currCost[k];
    }

    // Setup args struct
    args.start = 0;
    args.end = K;

    double startTime = CycleTimer::currentSeconds();
    // computeAssignments(&args);
    computeAssignmentsThreads(args);
    double endTime= CycleTimer::currentSeconds();
    AssignmentsTime += (endTime-startTime);

    startTime = CycleTimer::currentSeconds();
    computeCentroids(&args);
    endTime= CycleTimer::currentSeconds();
    CentroidsTime += (endTime-startTime);

    startTime = CycleTimer::currentSeconds();
    computeCost(&args);
    endTime= CycleTimer::currentSeconds();
    CostTime += (endTime-startTime);

    iter++;
  }
  printf("[Assignments Time]: %.3f ms\n", AssignmentsTime * 1000 / iter);
  printf("[Centroids Time]: %.3f ms\n", CentroidsTime * 1000 / iter);
  printf("[Cost Time]: %.3f ms\n", CostTime * 1000 / iter);

  free(currCost);
  free(prevCost);
}
