#include <iostream>
#include <cmath>
#include <omp.h>
#include <fstream>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <array>
#include <random>
#include "helpers.cpp"

using namespace std;

#define MAX_THREADS 100

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

  mrsw_snapshot_obj(int n)
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
    scan(clean_snap);

    snap_value old_snap_value = s_table_snap_values[me].load();
    snap_value new_sv(n_threads, clean_snap, new_value, old_snap_value.label + 1);

    s_table_snap_values[me].store(new_sv);
  }

  void scan(int clean_scan[])
  {

    int new_copy[n_threads], old_copy[n_threads];
    bool moved[n_threads];
    collect(old_copy);

    for (int i = 0; i < n_threads; i++)
    {
      moved[i] = false;
    }

    while (true)
    {
      while (true)
      {
        //get new collect
        collect(new_copy);

        for (int i = 0; i < n_threads; i++)
        {
          if (old_copy[i] != new_copy[i])
          {
            if (moved[i])
            { //second move, get snap  of this register
              cout << "found second move !" << endl;
              copy_b_to_a(clean_scan, s_table_snap_values[i].load().snap, n_threads);

              return;
            }
            else
            {
              moved[i] = true;
              // cout << "someone moved!" << endl;
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

  void collect(int collected[])
  { //returns the value states of s_table

    for (int i = 0; i < n_threads; i++)
    {
      collected[i] = s_table_snap_values[i].load().value;
    }

    return;
  }

  void display()
  {
    cout << endl
         << "------------------printing state of object-----------------" << endl;
    cout << "No of MRSW registers/threads: " << n_threads << endl;
    cout << endl
         << "-------------------------------------------" << endl;

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
      cout << endl
           << "-------------------------------------------" << endl;
    }
  }
};

int main()
{

  int m, n, k;
  float u_s, u_w;
  bool stop_writing = false;

  ifstream input_file;
  fstream experiments_file;
  fstream snapshots_file;

  input_file.open("inp-params-mrsw.txt", ios::in);
  experiments_file.open("experiments_mrsw.txt", ios::app);
  snapshots_file.open("snapshots_file_mrsw.txt");

  input_file >> n;
  input_file >> u_s;
  input_file >> u_w;
  input_file >> k;

  snapshots_file << "n: " << n << endl;
  snapshots_file << "k: " << k << endl;
  snapshots_file << "u_s: " << u_s << endl;
  snapshots_file << "u_w: " << u_w << endl;

  experiments_file << "n: " << n << endl;
  experiments_file << "k: " << k << endl;
  experiments_file << "u_s: " << u_s << endl;
  experiments_file << "u_w: " << u_w << endl;
  experiments_file << "u_s/u_w: " << u_s/u_w << endl;

  //init snapshot object
  mrsw_snapshot_obj ss(n);

  //init random generators
  default_random_engine generator;
  exponential_distribution<double> snapshot_delay(u_s);
  exponential_distribution<double> writer_delay(u_w);
  srand(time(NULL)); //set random seed using time for future random numbers generated

  omp_set_num_threads(n + 1); // n MRSW threads and 1 snapshot thread

#pragma omp parallel
  {
    int id, no_of_snapshots = 0, rand_val;
    double time_now, rand_time, start_time, tot_time, avg_time, worst_time = 0;

    id = omp_get_thread_num();

    if (id == n) //snapshot collecting thread executes here
    {
      int clean_snap[n];

      while (no_of_snapshots < k)
      {
        start_time = preprocess_timestamp(omp_get_wtime());

        //scan
        ss.scan(clean_snap);
        time_now = preprocess_timestamp(omp_get_wtime());
        write_to_file(snapshots_file, n, -1, clean_snap, time_now, -1, no_of_snapshots, true);
        no_of_snapshots++;

        //update avg and worst times
        tot_time = preprocess_timestamp(omp_get_wtime()) - start_time;
        avg_time += tot_time;

        if (tot_time > worst_time)
          worst_time = tot_time;

        //wait for rand time
        rand_time = snapshot_delay(generator);
        usleep(rand_time);
      }

      stop_writing = true; //make other threads stop writing
      experiments_file << "avg time: " << avg_time / k << ", worst time: " << worst_time << endl;
    }

    else //writing thread executes her
    {
      while (!stop_writing)
      {
        
        //update
        rand_val = rand() % 100;
        ss.update(id, rand_val);
        time_now = preprocess_timestamp(omp_get_wtime());
        write_to_file(snapshots_file, n, id, nullptr, time_now, rand_val, -1, false);

        //wait for rand time
        rand_time = writer_delay(generator);
        usleep(rand_time);
      }
    }
  }

  ss.display();

  // close files
  input_file.close();
  experiments_file.close();
  snapshots_file.close();
}
