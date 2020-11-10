#include <iostream>
#include <fstream>

using namespace std;

int default_int_array[] = {-1}; 

void checkpoint(){

  cout << "here";
  cout.flush();
  
}

void copy_b_to_a(int a[], int b[], int size){
  for (int i = 0; i < size; i++){
    a[i] = b[i];
  }
  
}

double preprocess_timestamp(double time){ 
    //truncates the leading 8 digits in the timestamp for clarity
    double base = (double)((int)time/1000)*1000; //truncates the 10s, 100s, 1000s place and floating points to 0
    return(time-base); 
}

void write_to_file(fstream &of, int n, int id = -1, int snapshot[] = default_int_array, double time_now = -1, int value = -1, int k = -1,  bool is_snapshot = false){

  #pragma omp critical
  {
    if(is_snapshot){
      of << endl << endl;
      of << "got " << k << "th snapshot at timestamp: "<< time_now << " as:" << endl;

      for (int i = 0; i < n; i++){
       of << snapshot[i] << ", ";  
      }
      of << endl << endl;
      
    }
    else{
      of << "thread " << id << " wrote a value: "<< value << " at timestamp: " << time_now << "seconds" <<  endl;

    }
  }
}

void write_to_file_2(fstream &of, int n, int loc = -1, int id = -1, int snapshot[] = default_int_array, double time_now = -1, int value = -1, int k = -1,  bool is_snapshot = false){

  #pragma omp critical
  {
    if(is_snapshot){
      
        of << endl << endl;
        of << "got " << k << "th snapshot at timestamp: "<< time_now << " as:" << endl;

        for (int i = 0; i < n; i++){
            of << snapshot[i] << ", ";  
        }
            of << endl << endl;
        
    }
    else{
      of << "thread " << id << " wrote a value: "<< value << " at location:" << loc << " at timestamp: " << time_now << "seconds" << endl;

    }
  }
}



