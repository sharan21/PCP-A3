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
#define MAX_MRMW_ARRAY_SIZE 100

array<int, MAX_MRMW_ARRAY_SIZE> default_std_arr;


class mrmw_entry
{

public:

    int value;
    int pid;
    int seq_no;
    

    mrmw_entry(int n) //construction for dummy snap value
    {
        value = -1;
        pid = -1;
        seq_no = -1;    
    }

    mrmw_entry(int new_sn, int new_value, int new_pid) //constructor for actual snap value update
    {
        value = new_value;
        seq_no = new_sn;
        pid = new_pid;

    }
};

class mrmw_snapshot_obj
{
public:
    int n_threads;
    int m_slots;
    atomic<mrmw_entry> s_table[MAX_MRMW_ARRAY_SIZE];
    atomic<array<int, MAX_MRMW_ARRAY_SIZE> > help_snaps[MAX_THREADS]; //array of n_thread atomic std::arrays

    mrmw_snapshot_obj(int n, int m)
    {
        n_threads = n;
        m_slots = m;
        mrmw_entry dummy_entry(n_threads);
        array<int,  MAX_MRMW_ARRAY_SIZE> dummy_helpsnap;

        


        // store dummy entries in s_table[m]
        for (int i = 0; i < m_slots; i++){   

            s_table[i].store(dummy_entry);
        }

        //create a dummy helpsnap
        for (int i = 0; i < n_threads; i++)
        {
            dummy_helpsnap[i] = -1;
        }
        
        //store dummy helpsnap in each atomic help_snaps[i]
        for (int i = 0; i < n_threads; i++){   

            help_snaps[i].store(dummy_helpsnap);
        }
    }

    void update(int me, int new_value, int loc) 
    {

        //first update s_table[m]
        mrmw_entry old_entry = s_table[loc].load();
        mrmw_entry new_entry(old_entry.seq_no+1, new_value, me);
        s_table[loc].store(new_entry);

        // then get clean snap
        array<int, MAX_MRMW_ARRAY_SIZE> clean_snap;
        scan(clean_snap);

        cout << "thread: " << me << "got clean snap:" <<endl;
        for (int i = 0; i < m_slots; i++)
        {
            cout << clean_snap[i] << ", ";
        }
        cout << endl;
        



        //then store help snap in help_snaps[n_threads]
        help_snaps[me].store(clean_snap);
    }

    void scan(array<int, MAX_MRMW_ARRAY_SIZE> &clean_scan, bool debug=false)
    {
    
        array<int, MAX_MRMW_ARRAY_SIZE> new_copy, old_copy;


        int thread_that_moved;
        bool moved[n_threads], try_again = false;
        collect_stdarray(old_copy);

    

        for (int i = 0; i < n_threads; i++){
            moved[i] = false;
        }
        
        
        while (true)
        {
            trythis:

                if(debug)
                    cout << "in trythis" << endl;

                while (true)
                {   
                    if(debug)
                        cout << "in the inner while" << endl;
                    //get new collect
                    if(debug)
                        usleep(100000);

                    collect_stdarray(new_copy);
                    
                    if(debug){

                        cout << "moved array is: " << endl;
                        for (int i = 0; i < m_slots; i++)
                        {
                            cout << moved[i] << ", ";
                        }
                        cout << endl;


                    }

                    for (int i = 0; i < m_slots; i++)
                    {
                        if (old_copy[i] != new_copy[i])
                        {
                            //slot i in s_table[m_slots] has changed, get the pid of thread that moved 
                            thread_that_moved = s_table[i].load().pid;
        
                            if(debug)    
                            {
                                cout << "thread that moved is:" << thread_that_moved << endl;
                                cout << "moved["  << thread_that_moved<<"] is "<<moved[thread_that_moved] << endl;

                            }
                                
                            if (moved[thread_that_moved]) //second move, get snap  of this register
                            { 
                                if(debug)
                                    cout << "found second move !" << endl;
                                auto help_snap_chosen = help_snaps[thread_that_moved].load();
                                clean_scan = help_snap_chosen;

                                for (int i = 0; i < m_slots; i++)
                                {
                                    cout << help_snap_chosen[i] << ",";
                                }
                                cout << endl;
                                // exit(0);
                                
                                // exit(0);
                                return;
                            }
                            else
                            {   
                                moved[thread_that_moved] = true;
                                if(debug)
                                {   
                                    cout << "thread " << thread_that_moved << " for the first time"  << endl;
                                    cout << moved[thread_that_moved] << endl;
                                }
                                    
                                old_copy = new_copy;
                                // try_again = true;
                                goto trythis;
                                // break;
                            }
                        }
                        if(debug)
                        cout << "new copy matches old copy! returning scan" << endl;
                    }
                    // if(debug && try_again){
                    //     cout << "trying again" << endl;
                    //     break;
                    // }
                        
                    if(debug)
                        checkpoint();

                    clean_scan = new_copy;
                    return;
                }
        }
    }


    void collect_stdarray(array<int, MAX_MRMW_ARRAY_SIZE> &collected)
    { //returns the value states of s_table

        for (int i = 0; i < m_slots; i++)
        {
            collected[i] = s_table[i].load().value;
        }
        usleep(10000);
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
            cout << "has value: " << s_table[i].load().value << endl;
            cout << "has seq_no: " << s_table[i].load().seq_no << endl;


            cout << endl
                 << "-------------------------------------------" << endl;
        }
    }
};

int main()
{

    int m, n, k;
    float u_1, u_2;
    bool stop_writing = false;

    ifstream input_file;
    fstream experiments_file;
    fstream snapshots_file;

    input_file.open("inp-params-mrmw.txt", ios::in);
    experiments_file.open("experiments_mrmw.txt", ios::app);
    snapshots_file.open("snapshots_file_mrmw.txt");

    input_file >> n;
    input_file >> m;
    input_file >> u_1;
    input_file >> u_2;
    input_file >> k;

    snapshots_file << "n: " << n << endl;
    snapshots_file << "m: " << m << endl;
    snapshots_file << "k: " << k << endl;
    snapshots_file << "u_1: " << u_1 << endl;
    snapshots_file << "u_2: " << u_2 << endl;

    experiments_file << "n: " << n << endl;
    experiments_file << "m: " << m << endl;
    experiments_file << "k: " << k << endl;
    experiments_file << "u_1: " << u_1 << endl;
    experiments_file << "u_2: " << u_2 << endl;

    //init snapshot object
    mrmw_snapshot_obj ss(n, m);

    //init random generators
    default_random_engine generator;
    exponential_distribution<double> writer_delay(u_1);
    exponential_distribution<double> snapshot_delay(u_2);
    srand(time(NULL)); //set random seed using time for future random numbers generated

    omp_set_num_threads(n + 1); // n MRSW threads and 1 snapshot thread

#pragma omp parallel
    {
        double start_time, tot_time, avg_time, worst_time=0;
        int id, no_of_snapshots = 0, rand_val, rand_loc;
        double time_now, rand_time;

        id = omp_get_thread_num();

        if (id == n) //snapshot collecting thread executes here
        {

            array<int, MAX_MRMW_ARRAY_SIZE> clean_snap;

            while (no_of_snapshots < k)
            {

                start_time = preprocess_timestamp(omp_get_wtime());
                
                //get a clean scan
                cout << "SCAN STARTTTT!!!!!" << endl;
                ss.scan(clean_snap, true);
                cout << "SCAN END" << endl;
                time_now = preprocess_timestamp(omp_get_wtime());
                write_to_file_2(snapshots_file, clean_snap, n, rand_loc, -1, time_now, -1, no_of_snapshots, true);
                no_of_snapshots++;

                //update avg and worst times
                tot_time = preprocess_timestamp(omp_get_wtime()) - start_time;
                avg_time += tot_time;

                if(tot_time > worst_time)
                    worst_time = tot_time;

                //wait for rand time
                rand_time = snapshot_delay(generator);
                usleep(100000);

            }

            //make other threads stop writing
            stop_writing = true; 

            //write to exp file
            experiments_file << "avg time: " << avg_time/k << ", worst time: "<< worst_time << endl;
        }

        else //writing thread executes her
        {
            while (!stop_writing)
            {
                //wait for rand time
                rand_time = writer_delay(generator);

                //update rand val at rand loc
                rand_val = rand()%100;          // 0 to 100
                rand_loc = std::rand()%n; //0 to n-1
                ss.update(id, rand_val, rand_loc);

                time_now = preprocess_timestamp(omp_get_wtime());
                
                write_to_file_2(snapshots_file, default_std_arr, n, rand_loc, id, time_now, rand_val, -1, false);

                //sleep
                rand_time = writer_delay(generator);
                usleep(10000);

            }
        }
        // cout << "thread: " << id << " out!" << endl;
        
    }

    ss.display();

    input_file.close();
    experiments_file.close();
    snapshots_file.close();
}
