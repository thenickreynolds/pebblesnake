//
// Implementation of Multiply with Carry for Pebble
// http://en.wikipedia.org/wiki/Multiply-with-carry
//

#define PHI 0x9e3779b9

static uint32_t Q[4096], c = 362436;

void init_rand(uint32_t x)
{
  int i;

  Q[0] = x;
  Q[1] = x + PHI;
  Q[2] = x + PHI + PHI;

  for (i = 3; i < 4096; i++) {
    Q[i] = Q[i - 3] ^ Q[i - 2] ^ PHI ^ i;
  }
}

uint32_t rand_cmwc(void) {
  uint64_t t, a = 18782LL;
  static uint32_t i = 4095;
  uint32_t x, r = 0xfffffffe;
  i = (i + 1) & 4095;
  t = a * Q[i] + c;
  c = (t >> 32);
  x = t + c;
  if (x < c) {
    x++;
    c++;
  }
  return (Q[i] = r - x);
}

void init_random_with_time() {
  PblTm time;
  get_time(&time);

  uint32_t time_seed = ((time.tm_hour * 60) + time.tm_min * 60) + time.tm_sec;
  init_rand(time_seed);
}