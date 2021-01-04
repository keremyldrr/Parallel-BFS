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
void processNode(int root,int dist);
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
  int root = rand() & (N - 1);//root init random
  for(int k = 1; k <= 16; k*=2)
    {

      omp_set_num_threads(k);


      path[root] = 0;
      int dist = 0;

      int start = row[root];
      int end = row[root+1];

      if(end > start){
#pragma omp parallel for schedule(static) proc_bind(spread)
	for(int i=start;i<end;i++)
	  {
	    path[col[i]] = 1;
	  }
      }
      else
	{
	  path[col[start]] = 1;
	}

      dist++;
      bool ch = false;
      begintime = omp_get_wtime();
      while(!ch)
	{
	  ch = true;
#pragma omp parallel for schedule(static) proc_bind(spread)
	  for(int q=0;q<N;q++)
	    {

	      int elem = path[q];
	      if(elem == dist)
		{
		  int st = row[q];
		  int en = row[q+1];
		  //cout << st << " " << en << endl;
		  for(int p = st;p<en;p++)
		    {
		      if(path[col[p]] == INT_MAX)
			{

			  ch = false;
			  path[col[p]] = dist+1;
			}
		    }
		}

	    }
	  dist++;

	}
      //#pragma omp barrier
      endtime = omp_get_wtime();
      bool check = true;
      for (int i = 0; i < N; i++) {
	if (path[i] == INT_MAX) {
	  check = false;
	  break;
	}
      }

      for(int i = 0; i < N; i++) // reset visited
	{
	  visited[i] = false;
	  path[i] = INT_MAX;

	}
      cout  << k << " threads  with source " << root << " takes " << endtime - begintime << " seconds." << "Is it correct ? " <<  check << endl;
    }



  delete[] col;
  delete[] row;
  delete[] path;

  cout << "Graph deleted" << endl;

  return 0;
}
