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
  
  return 0;
}