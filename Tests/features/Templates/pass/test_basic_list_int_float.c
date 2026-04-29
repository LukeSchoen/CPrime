// EXPECT_EXIT: 0

template<typename T>
int list_push_i(T *buf, int *len, int v)
{
  buf[*len] = (T)v;
  *len = *len + 1;
  return *len;
}

template<typename T>
T list_get(T *buf, int idx)
{
  return buf[idx];
}

int main(void)
{
  int li[4];
  int len_i = 0;
  float lf[4];
  int len_f = 0;

  list_push_i(int)(li, &len_i, 10);
  list_push_i(int)(li, &len_i, 20);

  list_push_i(float)(lf, &len_f, 1);
  list_push_i(float)(lf, &len_f, 2);

  if (len_i != 2 || len_f != 2)
    return 1;
  if (list_get(int)(li, 0) != 10 || list_get(int)(li, 1) != 20)
    return 2;
  if (lf[0] != 1.0f || lf[1] != 2.0f)
    return 3;

  return 0;
}
