#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <limits.h>
#include <vector>
#include <boost/dynamic_bitset.hpp>


#include <string.h> 


using namespace std;
int *row;
int *col;
int *path;
int numThreads;
bool *visited;
int N;
int **local_queues;
int *indices;

int inFrontier(int *frontier,int elem,int size,	boost::dynamic_bitset<>  &myset)
{

  int ret = -1;
  int start = row[elem];
  int end = row [elem+1];

  for(int i=start;i<end;i++)
    {
      int neigh = col[i];
      if(myset[neigh])
	{

	  return neigh;
	}
      
    }

  return ret;

}
void top_down_step(int *&frontier,int &load,int *&next,bool &check,int dist)
{
  
  check = false;
  int size = 0;
#pragma omp parallel //proc_bind(spread)
  {
    int tid = omp_get_thread_num();
    int pos=  0;//indices[tid] = 0;
#pragma omp for schedule(guided)
  for(int i=0;i<load;i++)
    {
      int elem  = frontier[i];
      int start = row[elem];
      int end = row[elem+1];
      for(int j=start;j<end;j++)
	{
	  int ind = col[j];
	  //	  cout << ind <<endl;
	  if(path[ind] == -1)
	    {
	      path[ind] = dist;
	      //local_queues[tid][indices[tid]] = ind;
	      //indices[tid]++;
	      local_queues[tid][pos++] = ind;
	      //	      pos++;

	    }
	  //if(!check)
	  //check = true;

	}
    }
  indices[tid] = pos;
  int sz = pos;
#pragma omp critical
  {
    for(int j=0;j<sz;j++)
      {
	next[size] = local_queues[tid][j];
	size++;

      }
  }
  }

  load = size;//prefix_sum[numThreads-1];
  check = load;
  //  cout << size << endl;
  int *temp = frontier;
  frontier = next;
  next = temp;
  
}
void bottom_up_step(int *&frontier,int &load,int *&next,bool &check,int dist,boost::dynamic_bitset<>  &myset)
{

  check = 1;
  int size = 0;

#pragma omp parallel //proc_bind(spread)
  {
    int tid = omp_get_thread_num();
    int pos = 0;//    indices[tid] = 0;
    //    printf("%d %d \n",indices[tid],tid);
    //local_queues[tid] = new int [N];
#pragma omp for schedule(guided)
  for(int i=0;i<N;i++)
    {
      int elem  = i;
      if(path[elem] == -1)
	{
	  //	  int par = inFrontier(frontier,elem,load,myset);
	  //	  int elem = frontier[i];
	  int start = row[elem];
	  int end = row [elem+1];
	  int par = -1;
	  for(int i=start;i<end;i++)
	    {
	      int neigh = col[i];
	      if(myset[neigh])
		{
		  
		  par =  neigh;
		  break;
		}
	      
	    }
	  if(par != -1)
	    {
	      path[elem] = dist;
	      local_queues[tid][pos++] = elem;
	      //pos++;
	      
	    }
	  
	}
    }
  indices[tid] = pos;
  }
  double s = omp_get_wtime();
  int *prefix_sum = new int[numThreads];
  prefix_sum[0] = indices[0];
  for(int id=1;id<numThreads;id++)
    prefix_sum[id] = indices[id]+prefix_sum[id-1];
  
  // cout<<"Prefix sum took "<<omp_get_wtime()-s << " seconds" << endl;  

#pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int sz = indices[tid];
    int ps = prefix_sum[tid-1];
    for(int j=0;j<sz;j++)
      {
	next[ps+j] = local_queues[tid][j];
	//	size++;

      }
  }
  
  
  
  load = prefix_sum[numThreads-1];
  check = load;//  cout << size << endl;
  int *temp = frontier;
  frontier = next;
  next = temp;
  //    cout << "**********************"<< endl;

  
}


int main(int argc, const char **argv) {
  
  if(argc != 2) {
      cout << "wrong input enter alpha beta values also " << endl;
        return 0;
    }
    

    srand (time(NULL));
    
    // tested on coPapersDBLP.mtx
    string filename = argv[1]; //"coPapersDBLP.mtx"; // edit
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

    

    //cout << "Graph has " << N << " nodes and " << (1+symmetric*1)*edge << " edges and symmetric " << symmetric << endl;
    
    int n;
    bool based0 = false;
    while(file >> n) {
      if (n == 0) {
            based0 = true;
            break;
        }
    }
    file.close();
    
    
    

    /*if(based0)
      cout << "Graph is 0 based" << endl;
    else
      cout << "Graph is 1 based and is being turned in to 0 base." << endl;
    */
    file.open(filename);
    while(file.peek() == '%')
      file.ignore(2048, '\n');
    file >> n >> n >> n; //tmp
    
    
    int i,j;
    
    vector<vector<int>> v(N, vector<int>(0));

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



    //*************************************************
    boost::dynamic_bitset<>  myset(N);

    myset.reset();
    double begintime, endtime;
    int root = 0;//rand() & (N - 1);//root init random

    for(int k = 1; k <= 16; k*=2)
      {

	omp_set_num_threads(k);
	numThreads = k;
	bool ch = true;
	int load = 1;
	int dist = 1;
	int *frontier = new int[N];
	frontier[0] = root;
	int *next = new int[N];
	for(int i = 0; i < N; i++)
	  path[i] =  -1;
	path[root] = 0;

	indices = new int [k];
	local_queues = new int *[k];
	#pragma omp parallel
	{
	  int k = omp_get_thread_num();
	  indices[k] = 0;
	  local_queues[k] = new int[N];
	  
	}

	//	int mu = 0;

	bool top_down = true;
	omp_set_num_threads(k);
	begintime = omp_get_wtime();
	while(ch)
	  {
	    double ratio = double(load)/N;
	    if(1)//symmetric  && ratio < 0.05)
	      {
		top_down_step(frontier,load,next,ch,dist);

	      }
	    else
	      {
		myset.reset();
		
		if(ratio > 0.1)
		  {
		  
#pragma omp parallel for schedule(static) 
		    for(int i=0;i<load;i++)
		      myset[frontier[i]] = 1;
		  }
		else
		  {
		    for(int i=0;i<load;i++)
		      myset[frontier[i]] = 1;

		  }
		  

  		bottom_up_step(frontier,load,next,ch,dist,myset);

	      }

	    dist++;
	    //	    mf = 0 ;
	  }
	endtime = omp_get_wtime();

	bool check = true;
	for (int i = 0; i < N; i++) {
	  if (path[i] == -1) {
	    check = false;
	    	    cout << "ERROR" << endl;
	    break;
	  }
	}
	/*
	string filename = "res " + to_string(k);
	ofstream inp(filename);
	for(int i = 0; i < N; i++) // reset visited
	  {
	    inp << path[i] << endl;
	    //visited[i] = false;
	    path[i] = INT_MAX;
	    
	  }
	  inp.close();*/
	//	cout  << k << " threads  with source " << root << " takes " << endtime - begintime << " seconds." << "Is it correct ? " <<  check << endl;
	cout <<filename << " "<< k << " "<< endtime - begintime<<endl;
      }
    
    
    delete[] col;
    delete[] row;
    delete[] path;
    
    //    cout << "Graph deleted" << endl;
    
    return 0;
}

