#include <iostream>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <array>

using namespace std;

#define MAX_THREADS 100

template <typename T> // https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
struct atomwrapper
{
  std::atomic<T> val;

  atomwrapper(): val()
  {
  }

  atomwrapper(const std::atomic<T> &val) : val(val.load())
  {
  }

  atomwrapper(const atomwrapper &other) : val(other.val.load())
  {
  }

  atomwrapper &operator=(const atomwrapper &other) //copy constructor
  {
    val.store(other.val.load());
  }
};


class snap_value
{

public:
  int value;
  int label;
  int snap[];
  

  snap_value(int n) //construction for dummy snap value
  {
    value = 0;
    label = 0;

    for (int i = 0; i < n; i++)
    {
      snap[i] = 0;
    }
  }

  snap_value(int n_threads, int new_snap[], int new_value, int new_label) //constructor for actual snap value update
  {
    value = new_value;
    label = new_label;

    for (int i = 0; i < n_threads; i++)
    {
      snap[i] = new_snap[i];
    }
    
  }

  
};

void checkpoint(){

  cout << "here";
  cout.flush();
  
}

class mrsw_snapshot_obj
{
public:
  vector<atomwrapper<snap_value> > s_table_snap_values;

  int n_threads;

  mrsw_snapshot_obj(int n, float u_1, float u_2)
  {

    n_threads = n;
    
    snap_value dummy_sv(n_threads);
    atomwrapper<snap_value> dummy_sv_wrapped(dummy_sv);

    for (int i = 0; i < n_threads; i++)
    {
      s_table_snap_values.push_back(dummy_sv_wrapped);
    }
  }

  void update(int me, int new_value) // this is invoked after scan()
  {
    
    // fetch the old snap value stored in register
    int temp[MAX_THREADS];
    int *clean_snap = scan(me, temp);

    // cout << new_value << endl;

  

    for (int k = 0; k < n_threads; k++)
    {
      cout << clean_snap[k] << endl;
    }

    
    // int clean_snap[100];
    snap_value old_snap_value = s_table_snap_values[me].val.load();
    
    //create and store new snap 
    snap_value new_sv(n_threads, clean_snap, new_value, old_snap_value.label+1);

    cout << new_sv.snap[0] << endl;
    


    atomwrapper<snap_value> new_sv_wrapped(new_sv);
    // new_sv_wrapped.val.store(new_sv);


    cout << new_sv_wrapped.val.load().label << endl;
    exit(0);
    // s_table_snap_values.push_back(new_sv_wrapped);
    s_table_snap_values[me].val.store(new_sv);

  }



  int* scan(int me , int clean_scan[]){ //returns the value states of s_table

    for (int i = 0; i < n_threads; i++)
    {
      // clean_scan[i] = s_table_snap_values[i].val.load().value;
      clean_scan[i] = 66;
    }
    return clean_scan;
  }

  void display()
  {
    cout << "------------------printing state of object-----------------" << endl;

    cout << "No of MRSW registers/threads: " << n_threads << endl;

    cout << endl << "-------------------------------------------" << endl;

    for (int i = 0; i < n_threads; i++)
    {
      cout << "Register: " << i << endl;
      cout << "has value: " << s_table_snap_values[i].val.load().value << endl;
      cout << "has label: " << s_table_snap_values[i].val.load().label << endl;

      cout << "has snap: " << endl;
      for (int j = 0; j < n_threads; j++)
      {
        // cout << j << endl;
        cout << s_table_snap_values[i].val.load().snap[j] << ", "; 
      }
      cout << endl << "-------------------------------------------" << endl;
      
    }
    
  }
};

vector<int> collect(mrsw_snapshot_obj m)
{

  vector<int> state;

  for (int i = 0; i < m.n_threads; i++)
  {
    // state.push_back(m.s_table[i].val);
  }

  return state;
}

int main()
{

  srand(time(NULL)); //set random seed using time for future random numbers generated

  double N, count = 0;
  double start_time, end_time;
  int m, n, k;
  float u_1, u_2;

  // init input and output files, get input parameters
  ifstream input_file;
  fstream times_file;
  fstream output_file;
  input_file.open("inp-params.txt", ios::in);
  times_file.open("Times.txt", ios::app);
  output_file.open("Primes-SAM1.txt");

  input_file >> n; // no of writer threads
  input_file >> m;
  input_file >> u_1;
  input_file >> u_2;
  input_file >> k;


  mrsw_snapshot_obj ss(n, u_1, u_2);


  start_time = omp_get_wtime();
  omp_set_num_threads(n + 1);


#pragma omp parallel
  {
    int id, i = 0;
    id = omp_get_thread_num();

    if (id == n) //snapshot collecting thread executes here
    { 
      
      
    }

    else //writing thread executes her
    {

      while (i < 10)
      {
  
        i++;
        int rand_val = rand() % 100;
        // int rand_val = 66;
        ss.update(id, rand_val);

        #pragma omp critical
        {
          cout << "Thread: " << id << " wrote value: " << rand_val << " at: " << endl;
        }
        usleep(100000);

      }
    }
  }

  ss.display();
  exit(0);

  // stop timer
  end_time = omp_get_wtime();

  // close files
  input_file.close();
  times_file.close();
  output_file.close();
}
