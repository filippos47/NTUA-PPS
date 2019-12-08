#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"
#include "utils.h"


int converge(double ** u_previous, double ** u_current, int Xm, int Ym, int X, int Y) {
    int i,j;
    for (i=Xm;i<X;i++) {
        for (j=Ym;j<Y;j++) {
            if (fabs(u_current[i][j]-u_previous[i][j])>e) {
                return 0;
            }
        }
    }
    return 1;
}

void Jacobi(double ** u_previous, double ** u_current, int X_min, int X_max, int Y_min, int Y_max) {
    int i,j;
    for (i=X_min;i<X_max;i++)
        for (j=Y_min;j<Y_max;j++)
            u_current[i][j]=(u_previous[i-1][j]+u_previous[i+1][j]+u_previous[i][j-1]+u_previous[i][j+1])/4.0;
}


int main(int argc, char ** argv) {
    int rank,size;
    int global[2],local[2]; //global matrix dimensions and local matrix dimensions (2D-domain, 2D-subdomain)
    int global_padded[2];   //padded global matrix dimensions (if padding is not needed, global_padded=global)
    int grid[2];            //processor grid dimensions
    int i,j,t;
    int global_converged=0,converged=0; //flags for convergence, global and per process
    MPI_Datatype dummy;     //dummy datatype used to align user-defined datatypes in memory
    double omega;           //relaxation factor - useless for Jacobi


    struct timeval tts,ttf,tcs,tcf,tconvs,tconvf;   //Timers: total-> tts,ttf, computation -> tcs,tcf, convergence -> tconvs,tconvf
    double ttotal=0,tcomp=0,tconv=0,total_time,comp_time,conv_time;

    double ** U, ** u_current, ** u_previous, ** swap; //Global matrix, local current and previous matrices, pointer to swap between current and previous

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    //----Read 2D-domain dimensions and process grid dimensions from stdin----//

    if (argc!=5) {
        fprintf(stderr,"Usage: mpirun .... ./exec X Y Px Py");
        exit(-1);
    }
    else {
        global[0]=atoi(argv[1]);
        global[1]=atoi(argv[2]);
        grid[0]=atoi(argv[3]);
        grid[1]=atoi(argv[4]);
    }

    //----Create 2D-cartesian communicator----//
    //----Usage of the cartesian communicator is optional----//

    MPI_Comm CART_COMM;         //CART_COMM: the new 2D-cartesian communicator
    int periods[2]={0,0};       //periods={0,0}: the 2D-grid is non-periodic
    int rank_grid[2];           //rank_grid: the position of each process on the new communicator

    MPI_Cart_create(MPI_COMM_WORLD,2,grid,periods,0,&CART_COMM);    //communicator creation
    MPI_Cart_coords(CART_COMM,rank,2,rank_grid);                    //rank mapping on the new communicator

    //----Compute local 2D-subdomain dimensions----//
    //----Test if the 2D-domain can be equally distributed to all processes----//
    //----If not, pad 2D-domain----//

    for (i=0;i<2;i++) {
        if (global[i]%grid[i]==0) {
            local[i]=global[i]/grid[i];
            global_padded[i]=global[i];
        }
        else {
            local[i]=(global[i]/grid[i])+1;
            global_padded[i]=local[i]*grid[i];
        }
    }

    //Initialization of omega
    omega=2.0/(1+sin(3.14/global[0]));

    //----Allocate global 2D-domain and initialize boundary values----//
    //----Rank 0 holds the global 2D-domain----//
    if (rank==0) {
        U=allocate2d(global_padded[0],global_padded[1]);
        init2d(U,global[0],global[1]);
    }

    //----Allocate local 2D-subdomains u_current, u_previous----//
    //----Add a row/column on each size for ghost cells----//

    u_previous=allocate2d(local[0]+2,local[1]+2);
    u_current=allocate2d(local[0]+2,local[1]+2);

    //----Distribute global 2D-domain from rank 0 to all processes----//

    //----Appropriate datatypes are defined here----//
    /*****The usage of datatypes is optional*****/

    //----Datatype definition for the 2D-subdomain on the global matrix----//

    MPI_Datatype global_block;
    MPI_Type_vector(local[0],local[1],global_padded[1],MPI_DOUBLE,&dummy);
    MPI_Type_create_resized(dummy,0,sizeof(double),&global_block);
    MPI_Type_commit(&global_block);

    //----Datatype definition for the 2D-subdomain on the local matrix----//

    MPI_Datatype local_block;
    MPI_Type_vector(local[0],local[1],local[1]+2,MPI_DOUBLE,&dummy);
    MPI_Type_create_resized(dummy,0,sizeof(double),&local_block);
    MPI_Type_commit(&local_block);

    //----Rank 0 defines positions and counts of local blocks (2D-subdomains) on global matrix----//
    int * scatteroffset, * scattercounts;
    if (rank==0) {
        scatteroffset=(int*)malloc(size*sizeof(int));
        scattercounts=(int*)malloc(size*sizeof(int));
        for (i=0;i<grid[0];i++)
            for (j=0;j<grid[1];j++) {
                scattercounts[i*grid[1]+j]=1;
                scatteroffset[i*grid[1]+j]=(local[0]*local[1]*grid[1]*i+local[1]*j);
            }
    }


    //----Rank 0 scatters the global matrix----//

    /*Make sure u_current and u_previous are
        both initialized*/
    MPI_Status status;

    if(rank == 0) {
            for(i = 1; i < grid[0] * grid[1]; i++) { // scatter matrix to other processes
                MPI_Send(&(U[0][0]) + scatteroffset[i], 1, global_block, i, i, CART_COMM);
            }
            for(i = 0; i < local[0]; i++) { // and then init my u_current
                for(j = 0; j < local[1]; j++) {
                    u_current[i + 1][j + 1] = U[i][j];
                }
            }

    }
    else {
        MPI_Recv(&(u_current[1][1]), 1, local_block, 0, rank, CART_COMM, &status); // receive subdomain from root process
    }

    copy2d(u_current, u_previous, local[0] + 2, local[1] + 2);

    if (rank==0) {
        free2d(U);
    }

    //----Define datatypes or allocate buffers for message passing----//
    MPI_Datatype column, row;

    MPI_Type_vector(local[0] + 1, 1, local[1] + 2, MPI_DOUBLE, &column);
    MPI_Type_commit(&column);

    MPI_Type_contiguous(local[1] + 2, MPI_DOUBLE, &row);
    MPI_Type_commit(&row);

    //----Find the 4 neighbors with which a process exchanges messages----//

    /*Make sure you handle non-existing
        neighbors appropriately*/
    int north, south, east, west;
    MPI_Cart_shift(CART_COMM, 0, 1, &north, &south);
    MPI_Cart_shift(CART_COMM, 1, 1, &west, &east);

    //---Define the iteration ranges per process-----//
    int i_min,i_max,j_min,j_max;
    if(north > -1)
        i_min = 1;
    else
        i_min = 2;

    if(south > -1)
        i_max = local[0] +1;
    else
        i_max = local[0];

    if(west > -1)
        j_min = 1;
    else
        j_min = 2;

    if(east > -1)
        j_max = local[1] + 1;
    else
        j_max = local[1];

    /*Three types of ranges:
        -internal processes
        -boundary processes
        -boundary processes and padded global array
    */

    MPI_Request requests[8];
    int requests_cnt = 0;
    //----Computational core----//   
    gettimeofday(&tts, NULL);
    #ifdef TEST_CONV
    for (t=0;t<T && !global_converged;t++) {
    #endif
    #ifndef TEST_CONV
    #undef T
    #define T 256
    for (t=0;t<T;t++) {
    #endif
        /*Compute and Communicate*/
        /*Add appropriate timers for computation*/
        // exchange boundary rows and columns
        swap = u_previous;
        u_previous = u_current;
        u_current = swap;

        requests_cnt = 0;
        if(north > -1) {
            MPI_Irecv(&(u_previous[0][0]), 1, row, north, north * 10 + rank, CART_COMM, &requests[requests_cnt++]);
            MPI_Isend(&(u_previous[1][0]), 1, row, north, rank * 10 + north, CART_COMM, &requests[requests_cnt++]);
        }
        if(south > -1) {
            MPI_Irecv(&(u_previous[local[0] + 1][0]), 1, row, south, south * 10 + rank, CART_COMM, &requests[requests_cnt++]);
            MPI_Isend(&(u_previous[local[0]][0]), 1, row, south, rank * 10 + south, CART_COMM, &requests[requests_cnt++]);
        }
        if(west > -1) {
            MPI_Irecv(&(u_previous[0][0]), 1, column, west, west * 10 + rank, CART_COMM, &requests[requests_cnt++]);
            MPI_Isend(&(u_previous[0][1]), 1, column, west, rank * 10 + west, CART_COMM, &requests[requests_cnt++]);
        }
        if(east > -1) {
            MPI_Irecv(&(u_previous[0][local[1] + 1]), 1, column, east, east * 10 + rank, CART_COMM, &requests[requests_cnt++]);
            MPI_Isend(&(u_previous[0][local[1]]), 1, column, east, rank * 10 + east, CART_COMM, &requests[requests_cnt++]);
        }
        MPI_Waitall(requests_cnt, requests, MPI_STATUSES_IGNORE);

        gettimeofday(&tcs, NULL);

        Jacobi(u_previous, u_current, i_min, i_max, j_min, j_max);

        gettimeofday(&tcf, NULL);
        tcomp += (tcf.tv_sec - tcs.tv_sec)
            + (tcf.tv_usec - tcs.tv_usec) * 0.000001;

        gettimeofday(&tconvs, NULL);
        #ifdef TEST_CONV
        if (t%C==0) {
            /*Test convergence*/
            converged = converge(u_previous, u_current, i_min, j_min, i_max, j_max);
             MPI_Allreduce(&converged, &global_converged, 1, MPI_INT,
                MPI_LAND, CART_COMM);
        }
            #endif
        gettimeofday(&tconvf, NULL);
        tconv += (tconvf.tv_sec - tconvs.tv_sec) + (tconvf.tv_usec - tconvs.tv_usec) * 0.000001;



    }
    gettimeofday(&ttf,NULL);
    ttotal=(ttf.tv_sec-tts.tv_sec)+(ttf.tv_usec-tts.tv_usec)*0.000001;
    MPI_Reduce(&ttotal,&total_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    MPI_Reduce(&tcomp,&comp_time,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    MPI_Reduce(&tconv, &conv_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);


    //----Rank 0 gathers local matrices back to the global matrix----//

    if (rank==0) {
            U=allocate2d(global_padded[0],global_padded[1]);
    }

/*
    MPI_Gatherv(&(u_current[1][1]), 1, local_block, 
        &(U[0][0]), scattercounts, scatteroffset, global_block,
        0, CART_COMM);
*/
    if(rank == 0) {
        for(i = 1; i < grid[0] * grid[1]; i++) {
            MPI_Recv(&(U[0][0]) + scatteroffset[i], 1, global_block, i, i, CART_COMM, &status);
        }
        for(i = 0; i < local[0]; i++) {
            for(j = 0; j < local[1]; j++) {
                U[i][j] = u_current[i + 1][j + 1];
            }
        }
    }
    else {
        MPI_Send(&(u_current[1][1]), 1, local_block, 0, rank, CART_COMM);
    }

     //************************************//




    //----Printing results----//

    if (rank==0) {
        printf("Jacobi X %d Y %d Px %d Py %d Iter %d ComputationTime %lf Convergence Time %lf TotalTime %lf midpoint %lf processes %d\n",global[0],global[1],grid[0],grid[1],t,comp_time,conv_time,total_time,U[global[0]/2][global[1]/2], size);

        #ifdef PRINT_RESULTS
        char * s=malloc(50*sizeof(char));
        sprintf(s,"resJacobiMPI_%dx%d_%dx%d",global[0],global[1],grid[0],grid[1]);
        fprint2d(s,U,global[0],global[1]);
        free(s);
        #endif

    }
    MPI_Finalize();
    return 0;
}

