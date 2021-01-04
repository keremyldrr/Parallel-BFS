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


__global__ void top_down_step(int *load,int *path,int dist,int *row,int *col,int *frontier,int *next,int size)
{
unsigned int ind = threadIdx.x + blockIdx.x * blockDim.x; // index in frontier
int sum = 0;

 if(ind < size)
   {
     if ((threadIdx.x | threadIdx.y | threadIdx.z
	  | blockIdx.x | blockIdx.y | blockIdx.z) == 0) {
       // statements that are executed once per kernel invocation,
       // at an unspecified time
       *load = 0;     
     }
     
     __syncthreads();
     int elem = frontier[ind];
     int start = row[elem];
     int end = row[elem+1];
     
     for(int I = start; I < end; I++)
       {
	 int i = col[I];
	 //OPTIMIZATION
	 if(path[i] == -1)
	   {
	     //*load = 1222;  
	     path[i] = dist+1;
	     int t = atomicAdd(load,1);
	     next[t] = i;
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
  
  int *frontier = new int[N];
  int *next = new int[N];
  //    int *levels = new int[N];
  for(int i = 0; i < N; i++)
    {
      path[i] =  -1;//INT_MAX;
      frontier[i] = -1;
      next[i] = -1;
    }

 
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
  int *d_load;
  int *d_front;
  int *d_next;
  int size_row = (N+1)*sizeof(int);
  int size_col = edge*sizeof(int);
  int size_path = N*sizeof(int);

  
  int frontSize = size_path;

  // for the row check up
  cudaMalloc((void **)&d_row,size_row);
  cudaMalloc((void **)&d_col,size_col);
  // path array
  cudaMalloc((void **)&d_path,size_path);
  //frontier on gpu
  cudaMalloc((void **)&d_front,frontSize);
  cudaMalloc((void **)&d_next,frontSize);
  // integer for check operation
  cudaMalloc((void **)&d_load,sizeof(int)); // load is the frontier size 

  int load = 1;
  frontier[0] = root;
  cout << "Memory allocated" <<endl;
  // move memory
  cudaMemcpy(d_row,row,size_row,cudaMemcpyHostToDevice);
  cudaMemcpy(d_col,col,size_col,cudaMemcpyHostToDevice);
  cudaMemcpy(d_path,path,size_path,cudaMemcpyHostToDevice);
  cudaMemcpy(d_front,frontier,frontSize,cudaMemcpyHostToDevice);
  cudaMemcpy(d_next,next,frontSize,cudaMemcpyHostToDevice);
  cudaMemcpy(d_load,&load,sizeof(int),cudaMemcpyHostToDevice);
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

  int level = 0;
  cout << "Algo starts now... Hold on to your seats" << endl;
  
  double start = omp_get_wtime();
  int zeroman = 0;
  //BlockDIM = 1;
  cout << "init load " << load << endl;
  int size = load;

  //threads = load;
  int m = 0;
  
  while(load != 0)
    {
      //double a = omp_get_wtime();
      //    cudaMemcpy(d_load,&zeroman,sizeof(int),cudaMemcpyHostToDevice);
      //      fs+=omp_get_wtime()-a;
      //double b = omp_get_wtime();
      top_down_step<<<BlockDIM,1024>>>(d_load,d_path,level,d_row,d_col,d_front,d_next,size);
      //sc+=omp_get_wtime()-b;
      //cudaDeviceSynchronize();
      int *tmp = d_front;
      d_front = d_next;
      d_next = tmp;

      
      cudaMemcpy(&load,d_load,sizeof(int),cudaMemcpyDeviceToHost);
      //      cudaDeviceSynchronize();
      size = load;
      BlockDIM = int(ceil(double(size)/1024));
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
  //cout << "MEMCPY1 " << fs << " "; 
  if(count == 0)
    cout << "BFS Complete" << endl;
  else
    cout << count << " unvisited nodes" << endl;

  double elapsed = finish - start;
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
