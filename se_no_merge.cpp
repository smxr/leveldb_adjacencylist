#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <memory>
#include <omp.h>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "geometry.h"
#include "config.h"

using namespace std;

#define HASH_SIZE 100000
#define OBJECTS_COUNT 10000000
#define ULL_MAX (unsigned long long)1<<62

#if defined(OS_WIN)
#include <Windows.h>
#else
#include <unistd.h>  // sysconf() - get CPU count
#endif


#if defined(OS_WIN)
#include <Windows.h>
#else

#include <unistd.h>  // sysconf() - get CPU count

#endif

typedef struct meeting_unit{
  size_t key;
  unsigned short start;
  unsigned short end;
  //Point midpoint;            //2023.7.17
  box mbr;                     //7.24 7.26
  bool isEmpty(){
    return key == ULL_MAX;
  }
  void reset(){
    key = ULL_MAX;
  }
  uint get_pid1(){
    return ::InverseCantorPairing1(key).first;
  }
  uint get_pid2(){
    return ::InverseCantorPairing1(key).second;
  }
}meeting_unit;

void stringsplit(string str, const char split, vector <string> &raw) {
  istringstream iss(str);
  string token;
  while (getline(iss, token, split)) {
    raw.push_back(token);
  }
}

void string_merge(string &old, string this_second) { //(1,2)(3,4)|0-5,6-11    merge   (1,6)(7,4)|12,20
  float low0, low1, high0, high1;
  int start_old, end_old, start_new, end_new;
  box mbr_old, mbr_new;

  vector <string> first_split, latter_split;
  stringsplit(old, '|', first_split);
  sscanf(first_split[0].data(), "(%f,%f)(%f,%f)", &low0, &low1, &high0, &high1);   //first_split[0] is old mbr
  stringsplit(first_split[1], ',',
              latter_split);                                       //first_split[1] is old meetings
  //  vector<string>::iterator it;
  //  it = latter_split.end()-1;
  //  sscanf(it->data(),"%d-%d",&start_old,&end_old);             //only the last one is useful
  sscanf(latter_split[latter_split.size() - 1].data(), "%d-%d", &start_old, &end_old);
  mbr_old = box(low0, low1, high0, high1);
  sscanf(this_second.data(), "(%f,%f)(%f,%f)|%d-%d", &low0, &low1, &high0, &high1, &start_new, &end_new);
  mbr_new = box(low0, low1, high0, high1);
  mbr_old.update(mbr_new);
  char buf[1024];
  sprintf(buf, "(%f,%f)(%f,%f)", mbr_old.low[0], mbr_old.low[1], mbr_old.high[0], mbr_old.high[1]);
  string str_mbr = buf;

  string str_meetings;
  if (start_old == start_new && end_old == end_new - 1) {
    str_meetings = to_string(start_new) + '-' + to_string(end_new);     //continue
  } else {
    str_meetings = first_split[1] + ',' + to_string(start_new) + '-' +
                   to_string(end_new);                          //append
  }
  old = str_mbr + '|' + str_meetings;

}

void update_mbr(float &low0, float &low1, float &high0, float &high1, float p_x, float p_y) {
  if (low0 > p_x) {
    low0 = p_x;
  }
  if (high0 < p_x) {
    high0 = p_x;
  }

  if (low1 > p_y) {
    low1 = p_y;
  }
  if (high1 < p_y) {
    high1 = p_y;
  }
}

//void insert(size_t key, string value, meeting_unit *meeting_buckets) {
//    size_t slot = key % HASH_SIZE;
//    while (true) {
//        if (meeting_buckets[slot].key == key) {
//            //      string_merge(meeting_buckets[slot].value, value);               //no need
//            //      break;
//        } else if (meeting_buckets[slot].key == ULL_MAX) {
//            meeting_buckets[slot].key = key;
//            meeting_buckets[slot].value = value;
//            break;
//        }
//        slot = (slot + 1) % HASH_SIZE;
//    }
//}

void printTime(float ms) {
  int h = ms / (1000 * 3600);
  int m = (((int) ms) / (1000 * 60)) % 60;
  int s = (((int) ms) / 1000) % 60;
  int intMS = ms;
  intMS %= 1000;

  fprintf(stderr, "Time Taken (Serial) = %dh %dm %ds %dms\n", h, m, s, intMS);
  fprintf(stderr, "Time Taken in milliseconds : %d\n", (int) ms);
}

template<typename T>
std::string Pack(const T *data) {
  std::string d(sizeof(T), L'\0');
  memcpy(&d[0], data, d.size());
  return d;
}

template<typename T>
std::unique_ptr <T> Unpack(const std::string &data) {
  if (data.size() != sizeof(T))
    return nullptr;
  auto d = std::make_unique<T>();
  memcpy(d.get(), data.data(), data.size());
  return d;
}

typedef struct adjacency_node {      //4 + 4*8 + 4 + 4 = 44
  uint target;
  uint start = 0;
  unsigned short duration = 0;
  box mbr;

  void print() {
    cout << "target: " << target << " low0: " << mbr.low[0] << " start: " << start << " duration: " << duration;
  }
} adjacency_node;

int main(int argc, char **argv) {
  configuration config = get_parameters(argc,argv);
  if(config.reconstruction){
    string kRemoveDirCommand = "rm -rf ";
    string rm_cmd = kRemoveDirCommand + config.kDBPath;
    int ret = system(rm_cmd.c_str());
    if (ret != 0) {
      fprintf(stderr, "Error deleting %s, code: %d\n", config.kDBPath.c_str(), ret);           //这个代码结尾竟然没有delete
    }
  }

  // open DB
  leveldb::Options options;
  options.create_if_missing = true;

  leveldb::DB *db;
  leveldb::Status s = leveldb::DB::Open(options, config.kDBPath, &db);
  if (s.ok()) {
    cout << "OK opening" << endl;
  }
  if (!s.ok()){
    cerr << s.ToString() << endl;
    assert(s.ok());
  }

  //adjacency_node *adjacency_node_buckets = new adjacency_node[HASH_SIZE];

  ofstream p;
  p.open(config.output_filename, ios::out | ios::trunc);
  p << "t" << "," << "meetings" << "," << "time(s)" << endl;


  omp_lock_t *lock = new omp_lock_t[OBJECTS_COUNT + 1];
  for (int i = 0; i < OBJECTS_COUNT + 1; i++) {
    omp_init_lock(&lock[i]);
  }

  //while(!inFile.eof())
  //while(inFile.open())
  int t;
  for (t = config.begin_second; t < config.seconds_duration; t++) {
    short *neighbor_count_pers = new short[OBJECTS_COUNT + 1]();
    //ifstream inFile(config.input_filename, ios::in | ios::binary);
    string file_name = config.meeting_source+"meetings_"+to_string(t)+".in";
    ifstream inFile(file_name, ios::in | ios::binary);
    if(!inFile.is_open()){
      cout<<"There is no file: "<<file_name<<endl;
      break;
    }
    //    if(t==config.seconds_duration-1){
    //      cout<<"all the seconds_duration are put into the db"<<endl;
    //    }
    uint curtime=0;
    inFile.read((char *) &curtime, sizeof(curtime));      //curtime = t = .end
    cout<<"curtime:"<<curtime<<endl;

    double timestart = omp_get_wtime();
    uint this_s_count = 0;
    inFile.read((char *) &this_s_count, sizeof(this_s_count));
    fprintf(stderr, "t=%d meetings=%u\n", t, this_s_count);
    p << t << "," << this_s_count << ",";

    meeting_unit meetings[this_s_count];
    for (int i = 0; i < this_s_count; i++) {
      inFile.read((char *) &meetings[i], sizeof(meeting_unit));
    }
    inFile.close();
    omp_set_num_threads(128);
#pragma omp parallel for
    for (int i = 0; i < this_s_count; i++) {                          //insert new edges
      adjacency_node *temp = new adjacency_node;
      temp->mbr = meetings[i].mbr;
      temp->start = meetings[i].start;
      temp->duration = meetings[i].end - meetings[i].start;
      for (int k = 0; k < 2; k++) {
        uint pid;
        if (k == 0) {
          pid = meetings[i].get_pid1();
          temp->target = meetings[i].get_pid2();
        } else {         //exchange
          pid = meetings[i].get_pid2();
          temp->target = meetings[i].get_pid1();
        }
        string str_key = to_string(pid)+"_"+to_string(t)+"_"+to_string(neighbor_count_pers[pid]);
        string temp_string = Pack(temp);
        omp_set_lock(&lock[pid]);
        leveldb::Status s2 = db->Put(leveldb::WriteOptions(), str_key, temp_string);
        if (!s.ok()){
          cerr << s.ToString() << endl;
          assert(s2.ok());
        }
        neighbor_count_pers[pid]++;
        omp_unset_lock(&lock[pid]);
      }
      delete temp;
    }
    double timeend = omp_get_wtime( );
    double time_taken1 = timeend - timestart;
    fprintf(stderr, "time_taken1: %lf\n", time_taken1);

    // get the system time
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    // 打印时间
    cerr << ctime(&time) << endl;

    p << time_taken1 << endl;
    delete []neighbor_count_pers;
  }
  p.close();

  for (int i = 0; i < OBJECTS_COUNT + 1; i++) {
    omp_destroy_lock(&lock[i]);
  }
  delete []lock;

  //  int not_zero_count = 0;
  //  int sum = 0;
  //  for(int i=0;i<OBJECTS_COUNT + 1;i++){
  //    if(neighbor_count[i]>0){
  //      not_zero_count++;
  //      sum += neighbor_count[i];
  //    }
  //  }
  //  cout<<"total key in db: "<<not_zero_count<<endl;
  //  cout<<"average neighbor for per node: "<<sum/not_zero_count<<endl;

  //  Iterator* it = db->NewIterator(ReadOptions());
  //  for (it->SeekToFirst(); it->Valid(); it->Next()) {
  //    //int key = atoi(it->key().ToString().c_str());
  //    cout << it->key().ToString() << ": ";
  //
  //    string string_node = it->value().ToString();       //48 ,pack doesn't change .size
  //    std::unique_ptr<adjacency_node> this_node = Unpack<adjacency_node>(string_node);
  //    this_node->print();
  //    cout<<" ; ";
  //
  //    cout<<endl;
  //  }
  //  assert(it->status().ok()); // Check for any errors found during the scan
  //  delete it;



  return 0;
}

