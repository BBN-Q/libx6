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

  double obufvar[4];
  accumlator.snapshot_variance(obufvar);
  assert(obufvar[0] == 2);
  assert(obufvar[1] == 50);
  assert(obufvar[2] == 2);
  assert(obufvar[3] == 5000);

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
  assert(obufvar[0] == 4);
  assert(obufvar[1] == 100);
  assert(obufvar[2] == 4);
  assert(obufvar[3] == 10000);

  cout << "snapshot variance: " << endl;
  cout << "obufvar[0]: " << obufvar[0] << " (goal 4)" << endl;
  cout << "obufvar[1]: " << obufvar[1] << " (goal 100)" << endl;
  cout << "obufvar[2]: " << obufvar[2] << " (goal 4)" << endl;
  cout << "obufvar[3]: " << obufvar[3] << " (goal 10000)" << endl;

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
  int sid1 = 273;
  int sid2 = 289;
  Correlator correlator({ch1, ch2}, 2, 1);

  ibuf[0] = 0 * scale; ibuf[1] = 10 * scale; // segment 1, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 1 * scale; ibuf[1] = 20 * scale; // segment 1, ch2
  correlator.accumulate(sid2, ibuf);
  ibuf[0] = 2 * scale; ibuf[1] = 100 * scale; // segment 2, ch1
  correlator.accumulate(sid1, ibuf);
  ibuf[0] = 3 * scale; ibuf[1] = 200 * scale; // segment 2, ch2
  correlator.accumulate(sid2, ibuf);

  correlator.snapshot(obuf);
  cout << "snapshot correlator: " << endl;
  cout << "obuf[0]: " << obuf[0] << endl;
  cout << "obuf[1]: " << obuf[1] << endl;
  cout << "obuf[2]: " << obuf[2] << endl;
  cout << "obuf[3]: " << obuf[3] << endl;
  assert(obuf[0] == 0);
  assert(obuf[1] == 200);
  assert(obuf[2] == 6);
  assert(obuf[3] == 20000);

  return 0;
}