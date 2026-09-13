/* Summarise the raw sample schema emitted by perf-compare.
   CPU clock ticks and GetSystemTimes can be coarser than a short compiler
   invocation.  A zero CPU value and a negative busy value are therefore
   unavailable observations, never measurements of zero work or zero load. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_GROUPS 128
#define MAX_SAMPLES 64

typedef struct {
  char name[160];
  int rows;
  int warmups;
  int failed;
  int wall_count;
  int cpu_count;
  int cpu_unavailable;
  int busy_count;
  int busy_unavailable;
  double wall[MAX_SAMPLES];
  double cpu[MAX_SAMPLES];
  double busy[MAX_SAMPLES];
} Group;

static void usage(void)
{
  printf("usage: perf-dispersion -Samples FILE\n");
}

static void copy_text(char *dst, size_t cap, const char *src)
{
  size_t i = 0;
  if (!cap) return;
  while (src && src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
  dst[i] = 0;
}

static int compare_double(const void *left, const void *right)
{
  double a = *(const double *)left;
  double b = *(const double *)right;
  return a < b ? -1 : a > b;
}

static double median(const double *values, int count)
{
  double sorted[MAX_SAMPLES];
  if (count <= 0) return -1.0;
  memcpy(sorted, values, (size_t)count * sizeof(double));
  qsort(sorted, (size_t)count, sizeof(double), compare_double);
  if (count & 1) return sorted[count / 2];
  return (sorted[count / 2 - 1] + sorted[count / 2]) / 2.0;
}

static void range(const double *values, int count, double *minimum, double *maximum)
{
  int i;
  *minimum = -1.0;
  *maximum = -1.0;
  for (i = 0; i < count; i++) {
    if (i == 0 || values[i] < *minimum) *minimum = values[i];
    if (i == 0 || values[i] > *maximum) *maximum = values[i];
  }
}

static int split(char *line, char **fields, int maximum)
{
  int count = 0;
  char *cursor = line;
  if (maximum <= 0) return 0;
  fields[count++] = cursor;
  while (*cursor && count < maximum) {
    if (*cursor == '\t') {
      *cursor = 0;
      fields[count++] = cursor + 1;
    }
    cursor++;
  }
  while (cursor > line && (cursor[-1] == '\r' || cursor[-1] == '\n')) *--cursor = 0;
  return count;
}

static Group *find_group(Group *groups, int *count, const char *case_name,
                         const char *compiler)
{
  char name[160];
  int i;
  snprintf(name, sizeof name, "%s/%s", case_name, compiler);
  for (i = 0; i < *count; i++)
    if (strcmp(groups[i].name, name) == 0) return &groups[i];
  if (*count == MAX_GROUPS) return 0;
  memset(&groups[*count], 0, sizeof groups[*count]);
  copy_text(groups[*count].name, sizeof groups[*count].name, name);
  return &groups[(*count)++];
}

int main(int argc, char **argv)
{
  const char *path = 0;
  FILE *input;
  char line[1024];
  Group groups[MAX_GROUPS];
  int group_count = 0;
  int i;

  if (argc == 3 && strcmp(argv[1], "-Samples") == 0) path = argv[2];
  if (!path) { usage(); return 2; }
  input = fopen(path, "rb");
  if (!input) { fprintf(stderr, "perf-dispersion: cannot read %s\n", path); return 2; }

  while (fgets(line, sizeof line, input)) {
    char *fields[9];
    int field_count;
    int warmup;
    int exit_code;
    int output;
    double wall;
    double cpu;
    double busy;
    Group *group;
    if (line[0] == '#') continue;
    field_count = split(line, fields, 9);
    if (field_count != 9) continue;
    warmup = atoi(fields[3]);
    wall = atof(fields[4]);
    cpu = atof(fields[5]);
    busy = atof(fields[6]);
    exit_code = atoi(fields[7]);
    output = atoi(fields[8]);
    group = find_group(groups, &group_count, fields[0], fields[1]);
    if (!group) { fclose(input); fprintf(stderr, "perf-dispersion: too many case/compiler rows\n"); return 2; }
    group->rows++;
    if (warmup) { group->warmups++; continue; }
    if (exit_code != 0 || !output) { group->failed++; continue; }
    if (group->wall_count < MAX_SAMPLES) group->wall[group->wall_count++] = wall;
    if (cpu > 0.0) {
      if (group->cpu_count < MAX_SAMPLES) group->cpu[group->cpu_count++] = cpu;
    } else group->cpu_unavailable++;
    if (busy >= 0.0) {
      if (group->busy_count < MAX_SAMPLES) group->busy[group->busy_count++] = busy;
    } else group->busy_unavailable++;
  }
  fclose(input);

  if (group_count == 0) { fprintf(stderr, "perf-dispersion: no sample rows in %s\n", path); return 2; }
  printf("raw sample dispersion: %s\n", path);
  printf("case/compiler                 measured  wall median [min,max]       cpu                 busy\n");
  for (i = 0; i < group_count; i++) {
    Group *group = &groups[i];
    double wall_min, wall_max;
    range(group->wall, group->wall_count, &wall_min, &wall_max);
    printf("%-29s %3d+%d    ", group->name, group->wall_count, group->warmups);
    if (group->wall_count)
      printf("%7.3f [%7.3f,%7.3f]  ", median(group->wall, group->wall_count), wall_min, wall_max);
    else printf("failed                      ");
    if (group->cpu_count)
      printf("%7.3f (%d; %d unavailable)  ", median(group->cpu, group->cpu_count),
             group->cpu_count, group->cpu_unavailable);
    else printf("unavailable (%d)         ", group->cpu_unavailable);
    if (group->busy_count)
      printf("%7.3f%% (%d; %d unavailable)", median(group->busy, group->busy_count),
             group->busy_count, group->busy_unavailable);
    else printf("unavailable (%d)", group->busy_unavailable);
    if (group->failed) printf("  failures %d", group->failed);
    printf("\n");
  }
  return 0;
}
