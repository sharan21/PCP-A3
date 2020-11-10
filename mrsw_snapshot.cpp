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


  mrsw_snapshot_obj(int n, float u_1, float u_2)
  {

    n_threads = n;
    
    snap_value dummy_sv(n_threads);

    for (int i = 0; i < n_threads; i++)
    {
      s_table_snap_values[i].store(dummy_sv);
    }
  }

  void update(int me, int new_value) // this is invoked after scan()
  {
    
    // get clean snap
    int clean_snap[n_threads];
    scan(me, clean_snap);
    
    snap_value old_snap_value = s_table_snap_values[me].load();
    snap_value new_sv(n_threads, clean_snap, new_value, old_snap_value.label+1);

    s_table_snap_values[me].store(new_sv);

  }



  void scan(int me ,int clean_scan[]){

    
    int new_copy[n_threads], old_copy[n_threads];

    //get old collect
    collect(old_copy);

    bool moved[n_threads];
    
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
            
            copy_b_to_a(clean_scan, s_table_snap_values[i].load().snap, n_threads);
            return;
          }
          else{
            moved[i] = true;
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
    return;
  }


  void display()
  {
    cout << "------------------printing state of object-----------------" << endl;

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
        // cout << j << endl;
        cout << s_table_snap_values[i].load().snap[j] << ", "; 
      }
      cout << endl << "-------------------------------------------" << endl;
      
    }
    
  }
};



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
