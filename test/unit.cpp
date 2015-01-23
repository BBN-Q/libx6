#include <iostream>
#include <iomanip>
#include <vector>

#include "X6_1000.h"
#include <Buffer_Mb.h>
#include <BufferDatagrams_Mb.h>

using namespace std;

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out) {
  return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

template <class T>
bool vec_equal(vector<T> a, vector<T> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}


int main ()
{
  cout << "BBN X6-1000 Library Unit Tests" << endl;

  // Accumulator tests
  Innovative::Buffer buf( Innovative::Holding<int>(2) );
  Innovative::IntegerDG ibuf(buf);

  Channel ch(1,1,1);
  Accumulator accumlator(ch, 1024, 2, 1);
  int scale = 1 << 14;

  ibuf[0] = 0 * scale; ibuf[1] = 10 * scale; // segment 1
  accumlator.accumulate(ibuf);
  ibuf[0] = 1 * scale; ibuf[1] = 100 * scale; // segment 2
  accumlator.accumulate(ibuf);
  ibuf[0] = 2 * scale; ibuf[1] = 20 * scale; // segment 1
  accumlator.accumulate(ibuf);
  ibuf[0] = 3 * scale; ibuf[1] = 200 * scale; // segment 2
  accumlator.accumulate(ibuf);

  double obuf[4];
  accumlator.snapshot(obuf);
  assert(obuf[0] == 1);
  assert(obuf[1] == 15);
  assert(obuf[2] == 2);
  assert(obuf[3] == 150);

  double obufvar[6];
  accumlator.snapshot_variance(obufvar);
  cout << "snapshot variance: " << endl;
  cout << "obufvar[0]: " << obufvar[0] << " (goal 2)" << endl;
  cout << "obufvar[1]: " << obufvar[1] << " (goal 50)" << endl;
  cout << "obufvar[2]: " << obufvar[2] << " (goal 10)" << endl;
  cout << "obufvar[3]: " << obufvar[3] << " (goal 2)" << endl;
  cout << "obufvar[4]: " << obufvar[4] << " (goal 5000)" << endl;
  cout << "obufvar[5]: " << obufvar[5] << " (goal 100)" << endl;
  assert(obufvar[0] == 2);
  assert(obufvar[1] == 50);
  assert(obufvar[2] == 10);
  assert(obufvar[3] == 2);
  assert(obufvar[4] == 5000);
  assert(obufvar[5] == 100);

  ibuf[0] = 4 * scale; ibuf[1] = 30 * scale; // segment 1
  accumlator.accumulate(ibuf);
  ibuf[0] = 5 * scale; ibuf[1] = 300 * scale; // segment 2
  accumlator.accumulate(ibuf);

  accumlator.snapshot(obuf);
  assert(obuf[0] == 2);
  assert(obuf[1] == 20);
  assert(obuf[2] == 3);
  assert(obuf[3] == 200);

  accumlator.snapshot_variance(obufvar);
  cout << "snapshot variance: " << endl;
  cout << "obufvar[0]: " << obufvar[0] << " (goal 4)" << endl;
  cout << "obufvar[1]: " << obufvar[1] << " (goal 100)" << endl;
  cout << "obufvar[2]: " << obufvar[2] << " (goal 20)" << endl;
  cout << "obufvar[3]: " << obufvar[3] << " (goal 4)" << endl;
  cout << "obufvar[4]: " << obufvar[4] << " (goal 10000)" << endl;
  cout << "obufvar[5]: " << obufvar[5] << " (goal 200)" << endl;
  assert(obufvar[0] == 4);
  assert(obufvar[1] == 100);
  assert(obufvar[2] == 20);
  assert(obufvar[3] == 4);
  assert(obufvar[4] == 10000);
  assert(obufvar[5] == 200);

  // Combinations test
  vector<vector<int>> combos;
  combos = combinations(3, 2);
  assert(combos.size() == 3);
  assert(vec_equal(combos[0], {0,1}));
  assert(vec_equal(combos[1], {0,2}));
  assert(vec_equal(combos[2], {1,2}));

  combos = combinations(4,2);
  assert(combos.size() == 6);
  assert(vec_equal(combos[0], {0,1}));
  assert(vec_equal(combos[1], {0,2}));
  assert(vec_equal(combos[2], {0,3}));
  assert(vec_equal(combos[3], {1,2}));
  assert(vec_equal(combos[4], {1,3}));
  assert(vec_equal(combos[5], {2,3}));

  combos = combinations(4,3);
  assert(combos.size() == 4);
  assert(vec_equal(combos[0], {0,1,2}));
  assert(vec_equal(combos[1], {0,1,3}));
  assert(vec_equal(combos[2], {0,2,3}));
  assert(vec_equal(combos[3], {1,2,3}));
  cout << "combinations:" << endl;
  for (int i = 0; i < combos.size(); i++) {
    cout << "combos[" << i << "]: {";
    for (int j = 0; j < combos[i].size(); j++) {
      cout << combos[i][j] << ", ";
    }
    cout << "}" << endl;
  }

  // Correlators
  Channel ch1(1,1,1), ch2(1,2,1);
  int sid1 = ch1.streamID;
  int sid2 = ch2.streamID;
  Correlator correlator({ch1, ch2}, 2, 1);

  ibuf[0] = 0 * scale; ibuf[1] = 7 * scale; // segment 1, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 1 * scale; ibuf[1] = 6 * scale; // segment 1, ch2
  correlator.accumulate(sid2, ibuf);
  ibuf[0] = 2 * scale; ibuf[1] = 5 * scale; // segment 2, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 3 * scale; ibuf[1] = 4 * scale; // segment 2, ch2
  correlator.accumulate(sid2, ibuf);

  correlator.snapshot(obuf);
  cout << "snapshot correlator: " << endl;
  cout << "obuf[0]: " << obuf[0] << endl;
  cout << "obuf[1]: " << obuf[1] << endl;
  cout << "obuf[2]: " << obuf[2] << endl;
  cout << "obuf[3]: " << obuf[3] << endl;
  assert(obuf[0] == 0*1 - 7*6);
  assert(obuf[1] == 0*6 + 1*7);
  assert(obuf[2] == 2*3 - 5*4);
  assert(obuf[3] == 2*4 + 3*5);

  correlator.snapshot_variance(obufvar);
  assert(obufvar[0] == 0);
  assert(obufvar[1] == 0);
  assert(obufvar[2] == 0);
  assert(obufvar[3] == 0);
  assert(obufvar[4] == 0);
  assert(obufvar[5] == 0);

  ibuf[0] = 4 * scale; ibuf[1] = 3 * scale; // segment 1, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 5 * scale; ibuf[1] = 2 * scale; // segment 1, ch2
  correlator.accumulate(sid2, ibuf);
  ibuf[0] = 6 * scale; ibuf[1] = 1 * scale; // segment 2, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 7 * scale; ibuf[1] = 0 * scale; // segment 2, ch2
  correlator.accumulate(sid2, ibuf);

  correlator.snapshot_variance(obufvar);
  cout << "snapshot variance: " << endl;
  cout << "obufvar[0]: " << obufvar[0] << " (goal 128)" << endl;
  cout << "obufvar[1]: " << obufvar[1] << " (goal 1568)" << endl;
  cout << "obufvar[2]: " << obufvar[2] << " (goal 448)" << endl;
  cout << "obufvar[3]: " << obufvar[3] << " (goal 128)" << endl;
  cout << "obufvar[4]: " << obufvar[4] << " (goal 1568)" << endl;
  cout << "obufvar[5]: " << obufvar[5] << " (goal -448)" << endl;
  assert(obufvar[0] == 128);
  assert(obufvar[1] == 1568);
  assert(obufvar[2] == 448);
  assert(obufvar[3] == 128);
  assert(obufvar[4] == 1568);
  assert(obufvar[5] == -448);

  Channel ch3(2,1,1);
  int sid3 = ch3.streamID;
  Correlator correlator2({ch1, ch2, ch3}, 1, 1);

  ibuf[0] = 0 * scale; ibuf[1] = 10 * scale; // segment 1, ch1
  correlator2.accumulate(sid1, ibuf);
  ibuf[0] = 1 * scale; ibuf[1] = 20 * scale; // segment 1, ch2
  correlator2.accumulate(sid2, ibuf);
  ibuf[0] = 2 * scale; ibuf[1] = 30 * scale; // segment 1, ch2
  correlator2.accumulate(sid3, ibuf);

  correlator2.snapshot(obuf);
  cout << "snapshot correlator: " << endl;
  cout << "obuf[0]: " << obuf[0] << endl;
  cout << "obuf[1]: " << obuf[1] << endl;
  assert(obuf[0] == 0*1*2 - 0*20*30 - 10*1*30 - 10*20*2);
  assert(obuf[1] == -10*20*30 + 10*1*2 + 0*20*2 + 0*1*30);

  return 0;
}