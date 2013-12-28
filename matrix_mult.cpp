/*
*MPI program to mulitply a 2D array with a column vector
 INPUTS: The master process takes rows, cols, vector
 OUTPUT: Vector

 We first determine the number of rows each slave must receive
 	= rows / processes

If rows is not divisible by processes
	plus_one_count (extra rows) = rows % processes

If processes > rows , the extra are not used

	Then we scatterv the rows 
	Compute
	Gather and display the results 


*/

#include<mpi.h>
#include<iostream>
using namespace std;


int *ary; // the 2D array 
int *vect; // the vector
int buff_len; // 

int *send_conf_ary; //element count array used by scatterv
int *send_displ_ary; // displacement array used by scatterv

int *slavesend_conf_ary; // element count array for gatherv
int *slavesend_displ_ary; // displacement array for gatherv

int *rec_buf; // slaves recieves the stripped rows here
int *result_buf;// the partial results stored in slaves
int *final_result; // the result array in the master
int col;int row; 
int rank;

int division_size;    // the size(length) of each of the rows
int division_count;   // number of rows each slave will receive
int process_count;    // number of processes
int plus_one_count;   // number of processs that will receive an extra row, in case
		      // the number of rows is not an integral multiple of number of processes

int getArraySize()
{
	cout<<"Enter M N: ";
	cin>>row>>col;
}

int getArrayMem()
{
	ary = new int[row*col];
}

int getVectMem()
{
	vect = new int[col];
}

int getAuxMem()
{
	send_conf_ary = new int[process_count];
	send_displ_ary = new int[process_count];
	slavesend_conf_ary = new int[process_count];
	slavesend_displ_ary = new int[process_count];
}

void populateArray()
{
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
			ary[i*col+j]=col*i+j;
	}
}

void populateVect()
{
	for(int i=0;i<col;i++)
		vect[i]=1;     // for testing convinience we have initialized vector to 1's
	/*
	   cout<<"V["<<i<<"]:";
	   cin>>vect[i];
	   */
}

void populateArrayUser()
{
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
		cin>>ary[i*col+j]; // array input
	}
}

void printArray()
{
	for(int i=0;i<row;i++)
	{
		for(int j=0;j<col;j++)
			cout<<ary[i*col+j]<<" ";

		cout<<"\n";
	}
}


void makeSendArray()
	//Configures for sending, we used scatter'V', so we are creating the element count arrays
       //and the displacements of the elements from the root element.	
{
	int i=0;
	int dispacc=0;
	
	for(;i<plus_one_count;i++)
	{
		send_conf_ary[i] = (division_count + 1)*division_size;
		send_displ_ary[i] = dispacc;
		dispacc += (division_count + 1)*division_size;
	}

	for(;i<process_count;i++)
	{
		send_conf_ary[i] = division_count * division_size;
		send_displ_ary[i] = dispacc;
		dispacc +=division_count * division_size;
	}

		rec_buf = new int[send_conf_ary[rank]];buff_len = send_conf_ary[rank];
}

void makeRecvArray()
	//configuration for receiving variable number of elements
{
	int i=0;
	int dispacc = 0 ;
	
	for(;i<process_count;i++)
	{
		slavesend_conf_ary[i] = send_conf_ary[i]/division_size;
		slavesend_displ_ary[i] = dispacc;
		dispacc += slavesend_conf_ary[i];
	}

}

void compute()
{
	// the actual multiplication 
	result_buf = new int[send_conf_ary[rank]];
	cout<<"\n";
	for(int times =0; times<send_conf_ary[rank]/col; times++)
	{
	int temp=0;
	for(int i=0;i<col;i++)
	temp+=rec_buf[times*col+i]*vect[i];
	result_buf[times]=temp;
	}
}

int sendComputeRecv()
{
	// Master will send the elements , compute it and receive the results
	MPI_Scatterv(ary,send_conf_ary,send_displ_ary,MPI_INT,rec_buf,buff_len,MPI_INT,0,MPI_COMM_WORLD);
		compute();
	final_result = new int[process_count];
	for(int i=0;i<process_count;i++)final_result[i] = -1;
	
	MPI_Gatherv(result_buf,slavesend_conf_ary[rank],MPI_INT,final_result,slavesend_conf_ary,slavesend_displ_ary,MPI_INT,0,MPI_COMM_WORLD);
}


int main()
{

	MPI_Init(NULL,NULL);
	MPI_Comm_size(MPI_COMM_WORLD,&process_count);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	int st;

	if(rank == 0)
	{
	st=MPI_Wtime();
	getArraySize();
	getArrayMem();
	populateArrayUser();
	}
	MPI_Bcast(&row,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&col,1,MPI_INT,0,MPI_COMM_WORLD);
	getVectMem();
	populateVect();

	getAuxMem();
	division_size = col; // the number of elements in each row
	division_count = row / process_count;
	plus_one_count = row % process_count ;

	if(rank==0)
	{
	printArray();
	cout<<"\nDivision count "<<division_count;
	cout<<"\nPlus-one count "<<plus_one_count;
	}

	makeSendArray();
	makeRecvArray();
	sendComputeRecv();

if(rank==0) //OUTPUT
{
	for(int i=0;i<row;i++)
		cout<<"\n"<<final_result[i];
	cout<<"\n\nTIME : "<<(MPI_Wtime()-st);
}
	MPI_Finalize();
}
