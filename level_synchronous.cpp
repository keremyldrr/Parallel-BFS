#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <limits.h>
#include <vector>
#include <queue>          // std::queue

#include <string.h> 


using namespace std;
int *row;
int *col;
int *path;
bool *visited;
int N;
void processNode(int root,int dist);

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
    visited = new bool[N];
    //    int *levels = new int[N];
    
    for(int i = 0; i < N; i++)
        path[i] =  INT_MAX;
    cout << "Preprocessing complete " << endl;

    // algorithm starts here
    //*************************************************

    double begintime, endtime;
    int root = 0;//rand() & (N - 1);//root init random
    for(int k = 1; k <= 16; k*=2)
      {

	omp_set_num_threads(k);

	for(int i = 0; i < N; i++)
	  path[i] =  INT_MAX;
	path[root] = 0;
	int dist = 1;
	int  **parents = new int* [k];
	int  **children = new int*[k];
	int  *indexes = new int[k];
	for(int i=0;i<k;i++)
	  {
	    indexes[i] = 0;
	    parents[i] = new int[N];
	    children[i] = new int[N];
	  }
	int start = row[root];
	int end = row[root+1];
#pragma omp parallel
	{
	  int tid = omp_get_thread_num();
#pragma omp for schedule(static,1)
	  for(int i=start;i<end;i++)
	    {
	      path[col[i]] = 1;
	      parents[tid][indexes[tid]]  = col[i];
	      indexes[tid]++;			    
	    }
	}
	begintime = omp_get_wtime();
	bool ch = true;

	while(ch)
	  {
	    
	    ch = false;
	    omp_set_num_threads(k);
	    int flag = 0;
	    int f_size = 0; //frontier size
#pragma omp parallel //proc_bind(spread)
	    {
	      int tid = omp_get_thread_num();
	      int lim  = indexes[tid];

	      indexes[tid] = 0;
#pragma omp atomic
	      f_size+=lim;
		
	      
	      for(int i=0;i<lim;i++)
		{
		  int elem = parents[tid][i];	
		  int st = row[elem];
		  int en = row[elem+1];
		  if(f_size > 10) //this is the check
		    {
		      flag = 1;

		      #pragma omp cancel parallel
		    }

#pragma omp cancellation point parallel
		  for(int kp=st;kp<en;kp++)
		    {
		    if(path[col[kp]] == INT_MAX)
{
			ch = true;
			path[col[kp]] = dist+1;
			children[tid][indexes[tid]] = col[kp];
			indexes[tid]++;			    
		      }
		    }
		  //
		}
	      
	      int sz = indexes[tid];
	      /*	      if(sz == 0)
			      zeroChecker++;*/
	      //	      #pragma omp simd
	      for(int a = 0;a<sz;a++)
		{
		  parents[tid][a] = children[tid][a];
		}
	      /*if(sz == 0)
		{
		  //
		  }*/
	    }
	    //	    cout << flag << endl;
	    if(flag == 1)
	      {
		//		cout <<"in" << endl;
		flag = 0;
#pragma omp parallel
		{

		  int tid  = omp_get_thread_num();
		  indexes[tid] = 0;
#pragma omp for schedule(static)
		for(int i=0;i<N;i++)
		  {
		    if(path[i] == dist)
		      {

			int st = row[i];
			int ed = row[i+1];

			for(int j = st;j<ed;j++)
			  {
			    int ind = col[j];
			    //			    printf("INACCEPTABLE %d \n",path[ind]);
			    if(path[ind] ==  INT_MAX)
			      {

				path[ind] = dist+1;
				parents[tid][indexes[tid]] = ind;
				indexes[tid]++;

				ch = true;
			      }
			  }

		      }


		  }
		}

	      }
	    //delete []children;
	    dist++;
	  }

	//	delete [] parents;
	//	delete []children;
	endtime = omp_get_wtime();

	bool check = true;
	for (int i = 0; i < N; i++) {
	  if (path[i] == INT_MAX) {
	    check = false;
	    //	    cout << "ERROR" << endl;
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
	cout  << k << " threads  with source " << root << " takes " << endtime - begintime << " seconds." << "Is it correct ? " <<  check << endl;
      }
    
    
    delete[] col;
    delete[] row;
    delete[] path;
    
    cout << "Graph deleted" << endl;
    
    return 0;
}

