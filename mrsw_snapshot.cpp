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
    :val()
  {}

  atomwrapper(const std::atomic<T> &val) :val(val.load())
  {}

  atomwrapper(const atomwrapper &other) :val(other.val.load())
  {}

  atomwrapper &operator=(const atomwrapper &other) //copy constructor
  {
    val.store(other.val.load());
    // return nullptr;
  }
};


struct snap_value{

  int value;
  int label;
  vector<int> snap;

};




class mrsw_snapshot{ 
    public:

        // vector< atomwrapper<snap_value> > s_table;
        vector< atomwrapper<int> > s_table;
        int n_threads;
        
  
        mrsw_snapshot(int n, float u_1, float u_2){
            
          atomic_int32_t temp(0);
          // atomwrapper<snap_value> empty_value;
          // empty_value.lo = 0;
          // empty_value.label = 0;
          n_threads = n;
          
          
          for (int i = 0; i < n; i++){
              s_table.push_back(temp);
          }
        }

        void update(int me, int value, fstream& times_file){
            s_table[me].val = value;
            // cout << "Thread: " << me << " wrote value: " << value << " at: " << endl;
            
            #pragma omp critical
            {
              times_file << "Thread: " << me << " wrote value: " << value << " at: " << endl;
            }
            

        }

        void display(){

            for(auto reg: s_table){
                cout << reg.val << endl;
            }
        }

        
};


vector<int> collect(mrsw_snapshot m){

  vector<int> state;

  for (int i = 0; i < m.n_threads; i++){ 
    state.push_back(m.s_table[i].val);  
  }

  return state;
  
}



int main(){

    
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

    mrsw_snapshot ss(n, u_1, u_2);


    // start timer
    start_time = omp_get_wtime();
    omp_set_num_threads(n+1);

    // int count = 0;

    #pragma omp parallel
    {
        int id, i = 0;
        id = omp_get_thread_num();
        
        if(id == 0){ //collect snapshot

          // vector

        }


        else{

          while(i < 10){
            i++;
            int r = rand()%100;
            ss.update(id-1, r, times_file);
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

    
    

