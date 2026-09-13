// PERF_NAME: c.struct.fields
/* Aggregate layout and member addressing: struct/union declarations, nested
   member chains, arrays of records and pointer chasing over a linked list. */

union payload {
  int as_int;
  unsigned as_unsigned;
  char as_bytes[4];
};

struct node {
  int key;
  int flags;
  double weight;
  union payload value;
  struct node *next;
};

static struct node storage[128];

static int sum_list(struct node *head)
{
  int total = 0;
  struct node *cursor = head;
  while (cursor) {
    total += cursor->key;
    total += cursor->flags;
    total += (int)cursor->weight;
    total += cursor->value.as_int;
    total += cursor->value.as_bytes[0];
    cursor = cursor->next;
  }
  return total;
}

int main(void)
{
  int i;
  int total;
  for (i = 0; i < 128; i++) {
    storage[i].key = i * 3;
    storage[i].flags = i ^ 0x55;
    storage[i].weight = (double)i * 1.5;
    storage[i].value.as_int = i - 64;
    storage[i].next = (i + 1 < 128) ? &storage[i + 1] : 0;
  }
  total = sum_list(&storage[0]);
  total += sum_list(&storage[64]);
  return total == 0 ? 1 : 0;
}
