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


__global__ void processLevel(int *row, int *col, int *path, int level, int N, int *check, unsigned int tnum, int chunk) {
 unsigned int id = threadIdx.x + blockIdx.x * blockDim.x;
  if(id == 0)
    *check = 0; // reset value
  // __syncthreads();  
  for(unsigned int i = id; i < N; i += tnum) {
    if(path[i] == level) {
      int start = row[i];
      int stop = row[i+1];
      int w;
      for(int j = start; j < stop; j++) {
	w = col[j];
	if(path[w] > level)
	  path[w] = level + 1;
      }
    }
  }
  __syncthreads();
  for(unsigned int i = id; i < N; i += tnum) {
    if(path[i] == INT_MAX) {
      atomicOr(check,1);
      break;
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
  // copy graph to device
  int *d_row;
  int *d_col;
  int *d_path;
  int *d_check;
  int size_row = (N+1)*sizeof(int);
  int size_col = edge*sizeof(int);
  int size_path = N*sizeof(int);
   
  cudaMalloc((void **)&d_row,size_row);
  cudaMalloc((void **)&d_col,size_col);
  cudaMalloc((void **)&d_path,size_path);
  cudaMalloc((void **)&d_check,sizeof(int));
  
  cout << "Memory allocated" <<endl;
  
  cudaMemcpy(d_row,row,size_row,cudaMemcpyHostToDevice);
  cudaMemcpy(d_col,col,size_col,cudaMemcpyHostToDevice);
  cudaMemcpy(d_path,path,size_path,cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();
  
  cout << "Graph copied to device" << endl;
  unsigned int numthread = 1024;
  unsigned int numblock = 2*4096;
  unsigned int tnum = numblock*numthread;
  int chunk = N/tnum;
  if(chunk*tnum < N)
    chunk += 1;
  if(tnum >= N) {
    chunk = 1;
    numblock = ceil((double)N/numthread);
  }
  int check = 1;
  int level = 0;
  cout << "starting" << endl;
  auto start = chrono::high_resolution_clock::now();
  while(check == 1) {
    processLevel<<<numblock,numthread>>>(d_row,d_col,d_path,level,N,d_check,tnum,chunk);
    cudaDeviceSynchronize();
    cudaMemcpy(&check,d_check,sizeof(int),cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    level++;
  }
  auto finish = chrono::high_resolution_clock::now();
  

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

  chrono::duration<double> elapsed = finish - start;
  cout << "Time: " << elapsed.count() << endl; 

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
