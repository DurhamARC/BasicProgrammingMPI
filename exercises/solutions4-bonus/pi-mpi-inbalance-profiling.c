#include <stdio.h>

#include <time.h>
#include <sys/time.h>
#include <mpi.h>

#define f(A) (4.0/(1.0+A*A))

const int n = 10000000;

int main(int argc, char** argv)
{
  int my_rank, num_procs;
  int i, sub_n, sub_start;
  double w, x, sum, p_sum, pi;
  double wt1, wt2, wt3, wt4;
  double wt_numerics, wt_inbalance, wt_comm, wt_total;
  double wt_all_numerics, wt_all_inbalance, wt_all_comm, wt_all_total;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  // Calculating the number of elements of my subdomain: sub_n
  // Calculating the start index sub_start within 0..n-1 
  // or sub_start = -1 and sub_n = 0 if there is no element

  // The following algorithm divides 5 into 2 + 1 + 1 + 1
  sub_n = n / num_procs; // = rounding_off(n/num_procs)
  int num_larger_procs = n - num_procs*sub_n; // = #procs with sub_n+1 elements
  if (my_rank < num_larger_procs)
  { sub_n = sub_n + 1;
    sub_start = 0 + my_rank * sub_n;
  }else if (sub_n > 0)
  { sub_start = 0 + num_larger_procs + my_rank * sub_n;
  }else 
  { // this process has only zero elements
    sub_start = -1;
    sub_n = 0;
  }
  if (num_procs >= 3) // in principle, the following inbalance requires only at least 2 processes
                      // but "3" allows a balanced run with "mpirun -np 2"  :-)
  {
    if (my_rank == num_procs-2 && sub_start >= 0)
    { // taking all remaining iterations
      sub_n = n - sub_start;
    }
    if (my_rank == num_procs-1)
    { // taking zero remaining iterations
      sub_start = -1; sub_n = 0;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD); // for a nearly common start time
  wt1=MPI_Wtime(); // start time
 
/* calculate pi = integral [0..1] 4/(1+x**2) dx */
  w=1.0/n;
  p_sum=0.0;
  for (i=sub_start;i<sub_start+sub_n;i++)
  {
    x=w*((double)i+0.5);
    p_sum=p_sum+f(x);
  }

  wt2=MPI_Wtime(); // after numerics
  MPI_Barrier(MPI_COMM_WORLD); // to profile the idle time due to bad load balance
  wt3=MPI_Wtime(); // before communication
  MPI_Reduce (&p_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
  MPI_Barrier(MPI_COMM_WORLD); // for a nearly common end time
  wt4=MPI_Wtime(); // end time
  wt_total     = wt4 - wt1; MPI_Reduce (&wt_total,     &wt_all_total,     1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  wt_numerics  = wt2 - wt1; MPI_Reduce (&wt_numerics,  &wt_all_numerics,  1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  wt_inbalance = wt3 - wt2; MPI_Reduce (&wt_inbalance, &wt_all_inbalance, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  wt_comm      = wt4 - wt3; MPI_Reduce (&wt_comm,      &wt_all_comm,      1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  printf ("PE%i/%i: sub_n= %7i, %12.4g numerics +  %12.4g inbalance +  %12.4g comm = %12.4g sec in total\n",
           my_rank, num_procs, sub_n, wt_numerics,    wt_inbalance,       wt_comm,   wt_total); 

  MPI_Barrier(MPI_COMM_WORLD); // to have some chance to separate output above and below in common stdout
  if (my_rank == 0) 
  { pi=w*sum;
    printf ("PE%i/%i: computed pi = %24.16g \n", my_rank, num_procs, pi);
    printf ("average over all %7i processes: sub_n= %i, %12.4g numerics +  %12.4g inbalance +  %12.4g comm = %12.4g sec in total\n",
                      num_procs, n/num_procs, wt_all_numerics/num_procs, wt_all_inbalance/num_procs, wt_all_comm/num_procs, wt_all_total/num_procs); 
    printf ("Parallel efficiency   = time in numeric  / total time = %6.2f \%\n", wt_all_numerics /wt_all_total * 100); 
    printf ("Loss by inbalance     = time in inbalance/ total time = %6.2f \%\n", wt_all_inbalance/wt_all_total * 100); 
    printf ("Loss by communication = time in comm     / total time = %6.2f \%\n", wt_all_comm     /wt_all_total * 100); 
  }

  MPI_Finalize();
  return 0;
}
