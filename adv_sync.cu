#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <limits.h>
#include <vector>
#include <queue>          // std::queue
#include <chrono>

using namespace std;
int *row;
int *col;
int *path;
bool *visited;
int N;


__global__ void top_down_step(int *load,int *path,int dist,int *row,int *col) {
  unsigned int ind = threadIdx.x + blockIdx.x * blockDim.x;
  *load = 0;
  if(path[i] == dist){
  int start = row[ind];
  int end = row[ind+1];
  for(int I = start; I < end; I++){
      if(path[col[I]] == dist)
        {
          path[col[I]] = dist+1;
          *load = 1;
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

  vector<vector<int> > v(N, vector<int>(0));

  for(int k = 0; k < edge; k++) {
    file >> i >> j;
    if(!based0) { // make 0 based;
      i--;
      j--;
    }

    v[i].push_back(j);
    if(symmetric)
      v[j].push_back(i);

  }
  file.close();

  edge *= 1 + (int)symmetric;


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

  path = new int[N];
  visited = new bool[N];
  //    int *levels = new int[N];
  for(int i = 0; i < N; i++)
    path[i] =  INT_MAX;
 

 
 cout << "Preprocessing complete " << endl;


  // algorithm starts here
  //*************************************************
  cudaSetDevice(0);
  int root = rand() % (N-1);
  root = 0;
  path[root] = 0;

  int root_begin = row[root];
  int root_end = row[root+1];
  
  for(int I = root_begin; I < root_end; I++){
	path[col[I]] = 1;
}
  // copy graph to device
  int *d_row;
  int *d_col;
  int *d_path;
  int *d_load;
  int size_row = (N+1)*sizeof(int);
  int size_col = edge*sizeof(int);
  int size_path = N*sizeof(int);
  int load = 1;

  // for the row check up
  cudaMalloc((void **)&d_row,size_row);
  cudaMalloc((void **)&d_col,size_col);
  // path array
  cudaMalloc((void **)&d_path,size_path);
  // integer for check operation
  cudaMalloc((void **)d_load,sizeof(int)); 
  
  cout << "Memory allocated" <<endl;
  // move memory
  cudaMemcpy(d_row,row,size_row,cudaMemcpyHostToDevice);
  cudaMemcpy(d_col,col,size_col,cudaMemcpyHostToDevice);
  cudaMemcpy(d_path,path,size_path,cudaMemcpyHostToDevice);
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
  auto start = omp_get_wtime();

  while(load != 0) {
    top_down_step<<<BlockDIM,threads>>>(d_load,path,level,d_row,d_col);
    cudaDeviceSynchronize();
    cudaMemcpy(&load,d_load,sizeof(int),cudaMemcpyDeviceToHost);
    cout << "LOAD: " << load << endl;
    cudaDeviceSynchronize();
    level++;
  }
  auto finish = omp_get_wtime();


  cudaMemcpy(path,d_path,size_path,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  int count = 0;
  for(int i = 0; i < N; i++) {
    if(path[i] == INT_MAX) {
      count++;
      //cout << i << endl;
    }
  }

  if(count == 0)
    cout << "BFS Complete" << endl;
  else
    cout << count << " unvisited nodes" << endl;

  auto elapsed = finish - start;
  cout << "Time: " << elapsed << endl;

  cudaFree(d_row);
  cudaFree(d_col);
  cudaFree(d_path);

  delete[] col;
  delete[] row;
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
