// EXPECT_EXIT: 0

template<typename T>
struct M
{
  T v[16];

  M();
  M(const T &m0, const T &m1, const T &m2, const T &m3,
    const T &m4, const T &m5, const T &m6, const T &m7,
    const T &m8, const T &m9, const T &m10, const T &m11,
    const T &m12, const T &m13, const T &m14, const T &m15);
};

template<typename T>
M<T>::M()
{
  v[0] = 0;
}

template<typename T>
M<T>::M(const T &m0, const T &m1, const T &m2, const T &m3,
        const T &m4, const T &m5, const T &m6, const T &m7,
        const T &m8, const T &m9, const T &m10, const T &m11,
        const T &m12, const T &m13, const T &m14, const T &m15)
{
  v[0] = m0; v[1] = m1; v[2] = m2; v[3] = m3;
  v[4] = m4; v[5] = m5; v[6] = m6; v[7] = m7;
  v[8] = m8; v[9] = m9; v[10] = m10; v[11] = m11;
  v[12] = m12; v[13] = m13; v[14] = m14; v[15] = m15;
}

int main()
{
  M<float> m(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16);
  return m.v[15] == 16 ? 0 : 1;
}
