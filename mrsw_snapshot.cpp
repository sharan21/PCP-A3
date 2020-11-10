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
int default_int_array[] = {-1};

void checkpoint(){

  cout << "here";
  cout.flush();
  
}

void copy_b_to_a(int a[], int b[], int size){
  for (int i = 0; i < size; i++)
  {
    a[i] = b[i];
  }
  
}


double preprocess_timestamp(double time){ 
    //truncates the leading 8 digits in the timestamp for clarity
    double base = (double)((int)time/1000)*1000; //truncates the 10s, 100s, 1000s place and floating points to 0
    return(time-base); 
}


class snap_value
{

public:
  int value;
  int label;
  int snap[MAX_THREADS];

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



class mrsw_snapshot_obj
{
public:
  
  int n_threads;
  atomic<snap_value> s_table_snap_values[MAX_THREADS];

  //other vars for diagnostics
  int collisions_faced[MAX_THREADS];

  mrsw_snapshot_obj(int n, float u_1, float u_2)
  {

    n_threads = n;
    snap_value dummy_sv(n_threads);

    for (int i = 0; i < n_threads; i++)
    {
      s_table_snap_values[i].store(dummy_sv);
      collisions_faced[i] = 0;

    }
  }

  void update(int me, int new_value) // this is invoked after scan()
  {
    
    // get clean snap
    int clean_snap[n_threads];
    scan(clean_snap);
    
    snap_value old_snap_value = s_table_snap_values[me].load();
    snap_value new_sv(n_threads, clean_snap, new_value, old_snap_value.label+1);

    s_table_snap_values[me].store(new_sv);

  }



  void scan(int clean_scan[], bool is_scanner=false){

    
    
    int new_copy[n_threads], old_copy[n_threads];
    bool moved[n_threads];
    collect(old_copy);
    
    for (int i = 0; i < n_threads; i++)
    {
      moved[i] = false;
    }

    while(true){

      while(true){

      //get new collect
      collect(new_copy);

      for (int i = 0; i < n_threads; i++)
      {
        if(old_copy[i] != new_copy[i]){
          
          if(moved[i]){ //second move, get snap  of this register
            cout << "found second move !" << endl;
            copy_b_to_a(clean_scan, s_table_snap_values[i].load().snap, n_threads);
            
            return;
          }
          else{
            moved[i] = true;
            cout << "someone moved!" << endl;
            copy_b_to_a(old_copy, new_copy, n_threads); // old_copy = new_copy
            
            break;
          }
        }
      }
      copy_b_to_a(clean_scan, new_copy, n_threads);
      return;
      }
    }
    
  }

  void collect(int collected[]){ //returns the value states of s_table

    for (int i = 0; i < n_threads; i++)
    {
      collected[i] = s_table_snap_values[i].load().value;
    }
    usleep(1000);
    return;
  }


  void display()
  {
    cout << endl << "------------------printing state of object-----------------" << endl;

    cout << "No of MRSW registers/threads: " << n_threads << endl;

    cout << endl << "-------------------------------------------" << endl;

    for (int i = 0; i < n_threads; i++)
    {
      cout << "Register: " << i << endl;
      cout << "has value: " << s_table_snap_values[i].load().value << endl;
      cout << "has label: " << s_table_snap_values[i].load().label << endl;

      cout << "has snap: " << endl;
      for (int j = 0; j < n_threads; j++)
      {
        cout << s_table_snap_values[i].load().snap[j] << ", "; 
      }
      cout << endl << "-------------------------------------------" << endl;
      
    }
    
  }
};


void write_to_file(fstream &of, int n, int id = -1, int snapshot[] = default_int_array, double time_now = -1, int value = -1, int k = -1,  bool is_snapshot = false){

  #pragma omp critical
  {
    if(is_snapshot){
      of << endl << endl;
      of << "got " << k << "th snapshot at timestamp: "<< time_now << " as:" << endl;

      for (int i = 0; i < n; i++)
      {
       of << snapshot[i] << ", ";  
      }
      of << endl << endl;
      
    }
    else{
      of << "thread " << id << " wrote a value: "<< value << " at timestamp: " << time_now << endl;

    }
  }
  
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
  fstream writers_file;
  fstream snapshots_file;

  input_file.open("inp-params.txt", ios::in);
  writers_file.open("writers_file.txt");
  snapshots_file.open("snapshots_file.txt");

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
    int id, no_of_writes = 0, no_of_snapshots = 0, rand_val;
    double time_now;

    id = omp_get_thread_num();

    if (id == n) //snapshot collecting thread executes here
    { 
      
      int clean_snap[n];

      snapshots_file << "n: " << n << endl;
      snapshots_file << "k: " << k << endl;
      snapshots_file << "u_1: " << u_1 << endl;
      snapshots_file << "u_2: " << u_2 << endl;

      while(no_of_snapshots < k){

        ss.scan(clean_snap, true);
        time_now = preprocess_timestamp(omp_get_wtime());

        write_to_file(snapshots_file, n, -1, clean_snap, time_now, -1, no_of_snapshots, true);
        
        no_of_snapshots++;
      }
    }

    else //writing thread executes her
    {

      while (no_of_writes < 10)
      {

        no_of_writes++;
        rand_val = rand() % 100; 
        ss.update(id, rand_val);

        time_now = preprocess_timestamp(omp_get_wtime());

        write_to_file(snapshots_file, n, id, nullptr, time_now, rand_val, -1, false);
        usleep(100);

      }
    }
  }



  // ss.display();
  exit(0);

  // stop timer
  end_time = omp_get_wtime();

  // close files
  input_file.close();
  writers_file.close();
  snapshots_file.close();
}
