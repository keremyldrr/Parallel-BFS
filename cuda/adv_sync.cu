#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <limits.h>
#include <vector>
#include <queue>          // std::queue


using namespace std;
int *row;
int *col;
int *path;
bool *visited;
int N;


__global__ void top_down_step(int *load,int *path,int dist,int *row,int *col)
{
  unsigned int ind = threadIdx.x + blockIdx.x * blockDim.x;

    if(path[ind] == dist)
      {
      int start = row[ind];
      int end = row[ind+1];
      for(int I = start; I < end; I++)
	{
	  int i = col[I];
	  if(path[i] == -1)
	    {
	      path[i] = dist+1;
	      //*load = 1;
	      atomicAdd(load, 1);
	    }
	}
      }
}

__global__ void bottom_up_step(int *load,int *path,int dist,int *row,int *col)
{
  unsigned int ind = threadIdx.x + blockIdx.x * blockDim.x;
  if(path[ind] == -1)
      {
      int start = row[ind];
      int end = row[ind+1];
      for(int I = start; I < end; I++)
	{
	  int i = col[I];
	  if(path[i] == dist)
	    {
	      path[ind] = dist+1;
	      atomicAdd(load, 1);
	      break;
	    }
	  
	}
      }
}


int main(int argc, const char **argv) {

  if(argc != 2) {
    cout << "wrong input " << endl;
    return 0;
  }


  srand (time(NULL));

  // tested on coPapersDBLP.mtx
  string filename = argv[1]; //"coPapersDBLP.mtx"; // edit
  //string filepath = "/home/cema/Documents/CS406/project/data/" + filename;
  ifstream file(filename.c_str());

  if(file.fail()) {
    cout << "Path not found" << endl;
    return 0;
  }

  string line;
  getline(file,line);

  stringstream s(line);
  bool symmetric = false;
  string word;
  while(s >> word) // check for symmetric
    if(word == "symmetric")
      symmetric = true;

  while(file.peek() == '%')//dirty boi
    file.ignore(2048, '\n');

  int  M, edge;
  file >> N >> M >> edge;



  cout << "Graph has " << N << " nodes and " << (1+symmetric*1)*edge << " edges and symmetric " << symmetric << endl;

  int n;
  bool based0 = false;
  while(file >> n) {
    if (n == 0) {
      based0 = true;
      break;
    }
  }
  file.close();




  if(based0)
    cout << "Graph is 0 based" << endl;
  else
    cout << "Graph is 1 based and is being turned in to 0 base." << endl;

  file.open(filename.c_str());
  while(file.peek() == '%')
    file.ignore(2048, '\n');
  file >> n >> n >> n; //tmp


  int i,j;
  vector<vector<int> > v_inv(N, vector<int>(0));
  vector<vector<int> > v(N, vector<int>(0));

  for(int k = 0; k < edge; k++) {
    file >> i >> j;
    if(!based0) { // make 0 based;
      i--;
      j--;
    }

    v[i].push_back(j);
    v_inv[j].push_back(i); // inverse graph
    if(symmetric)
      v[j].push_back(i);

  }
  file.close();

  edge *= 1 + (int)symmetric;

  int *row_inv = new int[N+1];
  int *col_inv = new int[edge];
  
  row = new int[N+1];
  col = new int[edge];

  row[0] = 0;
  int index = 0;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < v[i].size(); j++) {
      col[index] = v[i][j];
      index++;
    }
    row[i+1] = index;
  }

  //inv graph porcessing
  row_inv[0] = 0;
  index = 0;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < v_inv[i].size(); j++) {
      col_inv[index] = v_inv[i][j];
      index++;
    }
    row_inv[i+1] = index;
  }

  path = new int[N];
  visited = new bool[N];
  //    int *levels = new int[N];
  for(int i = 0; i < N; i++)
    path[i] =  -1;//INT_MAX;
 

 
 cout << "Preprocessing complete " << endl;


  // algorithm starts here
  //*************************************************
  cudaSetDevice(0);
  int root = rand() % (N-1);
  root = 0;
  path[root] = 0;

  int root_begin = row[root];
  int root_end = row[root+1];
  int root_begin_inv = row_inv[root];
  int root_end_inv = row_inv[root+1];
  
  for(int I = root_begin; I < root_end; I++){
	path[col[I]] = 1;
  }
  // copy graph to device
  int *d_row;
  int *d_col;
  int *d_row_inv;
  int *d_col_inv;
  int *d_path;
  int *d_load;
  int size_row = (N+1)*sizeof(int);
  int size_col = edge*sizeof(int);
  int size_path = N*sizeof(int);
  int load = 0;
  

  // for the row check up
  cudaMalloc((void **)&d_row,size_row);
  cudaMalloc((void **)&d_col,size_col);
  cudaMalloc((void **)&d_row_inv,size_row);
  cudaMalloc((void **)&d_col_inv,size_col);
  // path array
  cudaMalloc((void **)&d_path,size_path);
  // integer for check operation
  cudaMalloc((void **)&d_load,sizeof(int)); 
  
  cout << "Memory allocated" <<endl;
  // move memory
  cudaMemcpy(d_row,row,size_row,cudaMemcpyHostToDevice);
  cudaMemcpy(d_col,col,size_col,cudaMemcpyHostToDevice);
  cudaMemcpy(d_path,path,size_path,cudaMemcpyHostToDevice);
  cudaMemcpy(d_load,&load,sizeof(int),cudaMemcpyHostToDevice);
  cudaMemcpy(d_row_inv,row_inv,size_row,cudaMemcpyHostToDevice);
  cudaMemcpy(d_col_inv,col_inv,size_col,cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();

  cout << "Graph copied to device" << endl;

  cudaDeviceProp prop;
  cudaGetDeviceProperties(&prop, 0);
  unsigned int threads = prop.maxThreadsPerBlock;
  unsigned int threadDIM = prop.maxThreadsDim[3];

  cout << "Here are the specs\n";
  cout << "The threads: " << threads << "\n";
  cout << "Thread DIM:  " << threadDIM << "\n";
  int BlockDIM = int(ceil(double(N)/threads));  // BlockDIM ->> number of blocks necessary

  int level = 1;
  cout << "Algo starts now... Hold on to your seats" << endl;
  load = 1;
  double start = omp_get_wtime();
  int zeroman = 0;
  while(load != 0)
    {

      cudaMemcpy(d_load,&zeroman,sizeof(int),cudaMemcpyHostToDevice);
      if(double(load)/N <0.1)
	{
	  //cout << "topdown " << load << endl;
      top_down_step<<<BlockDIM,threads>>>(d_load,d_path,level,d_row,d_col);
	}
      else
	{
	  //cout << "bottomup " << load << endl;
	  if(symmetric)
	    bottom_up_step<<<BlockDIM,threads>>>(d_load,d_path,level,d_row,d_col);
	  else
	    bottom_up_step<<<BlockDIM,threads>>>(d_load,d_path,level,d_row_inv,d_col_inv);
	}
      cudaMemcpy(&load,d_load,sizeof(int),cudaMemcpyDeviceToHost);
      level++;
    }
  
  double finish = omp_get_wtime();
  

  cudaMemcpy(path,d_path,size_path,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  int count = 0;
  for(int i = 0; i < N; i++) {
    if(path[i] == -1) {
      count++;
    }
  }

  if(count == 0)
    cout << "BFS Complete" << endl;
  else
    cout << count << " unvisited nodes" << endl;

  double elapsed = finish - start;
  cout << "Time: " << elapsed << endl;

  cudaFree(d_row);
  cudaFree(d_col);
  cudaFree(d_path);
  cudaFree(d_row_inv);
  cudaFree(d_col_inv);

  delete[] col;
  delete[] row;
  delete[] col_inv;
  delete[] row_inv;
  //delete[] path;

  cout << "Graph deleted" << endl;

  ofstream ofile;
  ofile.open("results.txt");
  for(int i = 0; i < N; i++)
    ofile << path[i] << "\n";
  ofile.close();

  delete[] path;
  return 0;
}
