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
int lim;
int *row;
int *col;
int *path;
bool *visited;
int N;
int **local_queues;
int *indices;
void processNode(int root,int dist);
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

void top_down_step(int *&frontier,int *&next,int &load,bool &check,int dist)
{
  
  check = false;
  int size = 0;
#pragma omp parallel proc_bind(spread)
  {
    int tid = omp_get_thread_num();
    indices[tid] = 0;
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
	      local_queues[tid][indices[tid]] = ind;
	      indices[tid]++;
	    }

	}
    }
  
  
#pragma omp critical
  {
    for(int j = 0;j<indices[tid];j++)
      {
	frontier[size] = local_queues[tid][j];
	size++;
      }
  }
  }
  
  
  check = load =size;// prefix_sum[lim-1];
  
  //int *temp = frontier;
  //frontier = next;
  //next = temp;
  //    cout << "**********************"<< endl;

  
}
void bottom_up_step(int *&frontier,int *&next,int &load,bool &check,int dist,boost::dynamic_bitset<>  &myset)
{

  check = false;
  //int size = 0;
  int * prefix_sum = new int[lim];
#pragma omp parallel proc_bind(spread)
  {
    int tid = omp_get_thread_num();
    indices[tid] = 0;
#pragma omp for schedule(guided)
  for(int i=0;i<N;i++)
    {
      int elem  = i;
      if(path[elem] == -1)
	{
	  int par = inFrontier(frontier,elem,load,myset);
	  
	  if(par != -1)
	    {
	      path[elem] = dist;
	      
	      local_queues[tid][indices[tid]] = elem;

	      indices[tid]++;
	    }
	  
	}
    }
  }

  prefix_sum[0] = indices[0];
  for(int t=1;t<lim;t++)
    {
	prefix_sum[t] = indices[t] + prefix_sum[t-1];
	//cout << prefix_sum[t] << "  "<< indices[t] << endl;
    }
    
#pragma omp parallel //proc_bind(spread)
  {
    int tid = omp_get_thread_num();
    int inc = indices[tid];
    int sz = prefix_sum[tid-1];

    for(int j=0;j < inc;j++)
      {
	frontier[sz + j] = local_queues[tid][j];
      }
  }
  load = prefix_sum[lim-1];
  check = load;
  //  cout << "load is " << load << endl;
  // int *temp = frontier;
  //  frontier = next;
  //next = temp;
  //    cout << "**********************"<< endl;

  
}


int main(int argc, const char **argv) {
  
  if(argc != 2) {
      cout << "wrong input " << endl;
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

    

    // cout << "Graph has " << N << " nodes and " << (1+symmetric*1)*edge << " edges and symmetric " << symmetric << endl;
    
    int n;
    bool based0 = false;
    while(file >> n) {
      if (n == 0) {
            based0 = true;
            break;
        }
    }
    file.close();
    
    
    

    /*    if(based0)
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

    //    int *levels = new int[N];
    
    //for(int i = 0; i < N; i++)
    //  path[i] =  INT_MAX;
    int * outdegrees = new int[N];
    int numedges = (1+symmetric*1)*edge;
    for(int i=0;i<N;i++)
      {
	int st = row[i];
	int en = row[i+1];
	outdegrees[i] = en-st;
	
      }           
    //cout << "Preprocessing complete " << endl;

    // algorithm starts here
    //*************************************************
    boost::dynamic_bitset<>  myset(N);
    double begintime, endtime;
    int root = 0;//rand() & (N - 1);//root init random
    for(int k = 1; k <= 16; k*=2)
      {
	lim = k;
	omp_set_num_threads(k);
	bool ch = true;
	int load = 1;
	int dist = 1;
	int *frontier = new int[N];
	int *next = new int[N];
	frontier[0] = root;

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
	int alpha = 14; // chosen based on paper
	int beta = N/24;
	bool top_down = true;
	omp_set_num_threads(k);
	begintime = omp_get_wtime();
	while(ch)
	  {
	  
	    if(symmetric)
	      {
		if(top_down)
		  {
		    int mf=0;
#pragma omp simd reduction(+:mf)
		    for(int i=0;i<load;i++)
		      {
			mf+=frontier[i];
		      }
		    
		    int c = (numedges-mf)/alpha;
		    if(mf > c)
		      {
			top_down  = false;
		      }
		  }
		
		if (!top_down){
		  if(load < beta)
		top_down = true;
		}
	      }
	    if(top_down)
	      {
		top_down_step(frontier,next,load,ch,dist);
	      }
	    else
	      {
		myset.reset();
		for(int i=0;i<load;i++)
		  myset[frontier[i]] = 1;
  		bottom_up_step(frontier,next,load,ch,dist,myset);
	      }
	    //	    cout << load << endl;
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
	/*	string filename = "res " + to_string(k);
	ofstream inp(filename);
	for(int i = 0; i < N; i++) // reset visited
	  {
	    inp << path[i] << endl;
	    //visited[i] = false;
	    path[i] = INT_MAX;
	    
	  }
	  inp.close();*/
	//	cout  << k << " threads  with source " << root << " takes " << endtime - begintime << " seconds." << "Is it correct ? " <<  check << endl;
	cout  << k <<","<< endtime - begintime <<",\n"<<endl;// << "Is it correct ? " <<  check << endl;
      }
    
    
    delete[] col;
    delete[] row;
    delete[] path;
    
    //    cout << "Graph deleted" << endl;
    
    return 0;
}

