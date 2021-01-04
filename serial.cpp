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

struct node { // neighbour nodes
    int id;
    node *next;

    node() {id = -1; next = nullptr;}; // default constructor
    node(int i=-1, node *n=nullptr): id(i), next(n) {};

};

struct adj { // adjacency list node
    bool visited;
    node *head;
    node *tail;

    adj() {visited = false; head = nullptr; tail = nullptr;}; // default constructor
};

//#############################################
// edit for kerem boi
void processNeg(vector<vector<int>>& nv, int *nrow, int *ncol, int N) {
  nrow[0] = 0;
  int index = 0;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < nv[i].size(); j++) {
      ncol[index] = nv[i][j];
      index++;
    }
    nrow[i+1] = index;
  }
}
//###############################################

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

    int N, M, edge;
    file >> N >> M >> edge;



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
    //#####################################
    vector<vector<int>> negv(N, vector<int>(0)); // additional edit
    //#####################################
    //for(int k = 0; k < edge; k++) {
    int e = 0;
    while(!file.eof()) {
        e++;
        file >> i >> j;
        if(!based0) { // make 0 based;
            i--;
            j--;
        }

        v[i].push_back(j);
	//##################
	negv[j].push_back(i); // for directed graph only
	//#################
        if(symmetric)
            v[j].push_back(i);

    }
    file.close();

    edge = e;
    edge *= 1 + (int)symmetric;
    // check the graph statistics
    int zerodeg = 0, onedeg = 0, mindeg = INT_MAX, maxdeg = -1;
    double avgdeg = 0.0;
    for(int i = 0; i < N; i++) {
        int size = v[i].size();
        if(size == 0)
            zerodeg++;
        else if(size == 1)
            onedeg++;
        if(size < mindeg)
            mindeg = size;
        if(size > maxdeg)
            maxdeg = size;
        avgdeg += size;
    }
    avgdeg /= N;

    cout << "Graph has " << N << " nodes and " << edge << " edges and symmetric " << symmetric << endl;
    cout << "zero degree vertices: " << zerodeg << endl;
    cout << "one degree vertices: " << onedeg << endl;
    cout << "min degree: " << mindeg << endl;
    cout << "max degree: " << maxdeg << endl;
    cout << "avg degree: " << avgdeg << endl;


    int *row = new int[N+1];
    int *col = new int[edge];
    int *nrow = new int[N+1];
    int *ncol = new int[edge];
    //######################
    processNeg(negv,nrow,ncol,N); // for kerrem boi
    //######################
    row[0] = 0;
    int index = 0;
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < v[i].size(); j++) {
            col[index] = v[i][j];
            index++;
        }
        row[i+1] = index;

    }



    int *path = new int[N];
    for(int i = 0; i < N; i++)
        path[i] =  INT_MAX;


    int * outdegrees = new int[N];
    for(int i=0;i<N;i++)
      {
	int st = row[i];
	int en = row[i+1];
	outdegrees[i] = en-st;

      }
    cout << "Preprocessing complete " << endl;

    // algorithm starts here
    //*************************************************

    std::queue<int> q;
    double begin, end;

    for(int k = 0; k < 10; k++) {
        begin = omp_get_wtime();

        int seed = rand() & (N - 1);

        q.push(seed); // starting node
        path[seed] = 0;

        int start, stop;
        while (!q.empty()) {
	  int n =q.front();
	  q.pop();
	  start = row[n];
	  stop = row[n + 1];
	  int step = path[n];
	  for (int i = start; i < stop; i++) {
	    if (path[col[i]] == INT_MAX) {
	      path[col[i]] = step+1;
                    q.push(col[i]);
	    }
	  }
        }
	
        end = omp_get_wtime();
	
	
	
        bool check = true;
        for (int i = 0; i < N; i++) {
            if (path[i] == INT_MAX) {
                check = false;
                break;
            }
        }



        for(int i = 0; i < N; i++) // reset visited
            path[i] = INT_MAX;

        /*
        if (check)
            cout << "Graph complete" << endl;
        else
            cout << "Graph not complete" << endl; */

        cout << "Trial " << k << " with source " << seed << " takes " << end - begin << " seconds." << check << endl;

    }



    delete[] col;
    delete[] row;
    delete[] path;

    cout << "Graph deleted" << endl;

    return 0;
}
