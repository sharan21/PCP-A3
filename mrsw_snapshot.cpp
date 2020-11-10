#include <iostream>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <vector>
#include <atomic>
#include <unistd.h>

using namespace std;

template <typename T> // https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
struct atomwrapper
{
  std::atomic<T> val;

  atomwrapper()
      : val()
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
    // return nullptr;
  }
};

class snap_value
{

public:
  int value;
  int label;
  int reg_no;
  int snap[10];

  snap_value(int n) //construction for dummy snap value
  {
    value = 0;
    label = 0;

    for (int i = 0; i < n; i++)
    {
      snap.push_back(0);
    }
  }

  snap_value(vector<int> new_snap, int new_value, int new_label) //constructor for actual snap value update
  {
    value = new_value;
    label = new_label;
    snap = new_snap;
  }

  snap_value(const snap_value &s) //copy constructor
  {
    value = s.value;
    label = s.label;
    snap = s.snap;
  }
};

class mrsw_snapshot_obj
{
public:
  vector<atomwrapper<int>> s_table_atomic_values;
  vector<snap_value> s_table_snap_values;

  int n_threads;

  mrsw_snapshot_obj(int n, float u_1, float u_2)
  {

    n_threads = n;
    atomwrapper<int> dummy_val(0);

    for (int i = 0; i < n; i++)
    {
      s_table_atomic_values.push_back(dummy_val);
    }
  }

  void update(int me, int new_value, vector<int> clean_snap, fstream &times_file) // this is invoked after scan()
  {

    // fetch the old snap value stored in register
    snap_value old_snap = s_table_snap_values[me];

    //create and store new snap 
    snap_value new_snap(clean_snap, new_value, old_snap.label+1);
    s_table_snap_values.push_back(new_snap);

    //update the new value of the atomic register
    s_table_atomic_values[me].val.store(new_value);
    
    // cout << "Thread: " << me << " wrote value: " << value << " at: " << endl;

    #pragma omp critical
    {
      times_file << "Thread: " << me << " wrote value: " << new_value << " at: " << endl;
    }
  }

  vector<atomwrapper<int> > collect(){
    return s_table_atomic_values;
  }

  void scan(int me){
    vector<

    
  }

  void display()
  {
    for (int i = 0; i < n_threads; i++)
    {
      cout << "Register: " << i << " has value: " << s_table_atomic_values[i].val.load() << endl;
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

  // cout << ss.

  // start timer
  start_time = omp_get_wtime();
  omp_set_num_threads(n + 1);

  // int count = 0;

#pragma omp parallel
  {
    int id, i = 0;
    id = omp_get_thread_num();

    if (id == 0)
    { //collect snapshot

      // vector
    }

    else //writing threads
    {

      while (i < 10)
      {
        sc


        i++;
        int r = rand() % 100;
        ss.update(id - 1, r, times_file);
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
