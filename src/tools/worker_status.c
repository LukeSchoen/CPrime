/* Report progress toward the retained-GCC + first-party test target and keep a
   durable, machine-readable sample log for the worker loop.

   The log is append-only, tab separated, one sample per completed work cycle.
   It lives in the repository so `git add -A` commits each sample together with
   the work that produced it; a restart therefore needs no special handling.

     # cprime worker progress log v1
     # utc  epoch  cycle  event  gcc_left  fp_left  total_left  head

   `head` is the commit the sample was measured against; the cycle's own work
   lands in the next commit, which also carries this row.

   Usage: worker-status [options]
     --root DIR         repository root (default ".")
     --log PATH         progress log (default ROOT/Tests/progress/log.tsv)
     --corpus PATH      retained manifest (default ROOT/Tests/pedantic/gcc/corpus.json)
     --first-party PATH outstanding list (default ROOT/Tests/progress/first-party-failures.txt)
     --cycle N          cycle number for the sample (default: last recorded)
     --event NAME       sample event tag (default "cycle")
     --head REV         short commit hash to record (default "-")
     --now EPOCH        override the current time (default: system clock)
     --append           append a sample for the current state
     --last-cycle       print the last recorded cycle number and exit
     --brief            show backlog and last sample without rates or ETA
     --help             print this text

   Exit: 0 ok, 2 usage or input error, 10 target reached (nothing left). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SAMPLES 65536
#define MAX_LINE 4096

typedef struct {
    long long epoch;
    int cycle;
    int gcc;
    int fp;
    int total;
    char utc[32];
    char event[24];
    char head[64];
} Sample;

static Sample samples[MAX_SAMPLES];
static int sample_count;

static int is_space(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long size;
    size_t got;
    char *text;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    size = ftell(file);
    if (size < 0 || size > 64L * 1024 * 1024) { fclose(file); return 0; }
    if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return 0; }
    text = (char *)malloc((size_t)size + 1);
    if (!text) { fclose(file); return 0; }
    got = fread(text, 1, (size_t)size, file);
    fclose(file);
    text[got] = 0;
    return text;
}

/* Count `"role": "case"` entries in the retained corpus manifest. The manifest
   must carry a `"cases"` key so a truncated or foreign file cannot read as an
   empty (complete) corpus. */
static int count_cases(const char *text, int *ok)
{
    const char *cursor = text;
    int count = 0;
    *ok = strstr(text, "\"cases\"") != NULL;
    while ((cursor = strstr(cursor, "\"role\"")) != NULL) {
        const char *value = cursor + 6;
        cursor = value;
        while (is_space(*value)) value++;
        if (*value != ':') continue;
        value++;
        while (is_space(*value)) value++;
        if (*value != '"') continue;
        value++;
        if (strncmp(value, "case\"", 5) == 0) count++;
    }
    return count;
}

/* Count outstanding first-party entries: one repo-relative path per line, `#`
   comments and blank lines ignored. */
static int count_list(const char *text)
{
    const char *cursor = text;
    int count = 0;
    while (*cursor) {
        const char *end = strchr(cursor, '\n');
        size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
        size_t index = 0;
        while (index < length && is_space((unsigned char)cursor[index])) index++;
        if (index < length && cursor[index] != '#') count++;
        if (!end) break;
        cursor = end + 1;
    }
    return count;
}

static void copy_field(char *out, size_t size, const char *value)
{
    size_t index = 0;
    while (value[index] && index + 1 < size) { out[index] = value[index]; index++; }
    out[index] = 0;
}

static int read_log(const char *path)
{
    char *text = read_file(path);
    const char *cursor;
    if (!text) return 0;
    cursor = text;
    while (*cursor) {
        char line[MAX_LINE];
        char *fields[8];
        char *token;
        const char *end = strchr(cursor, '\n');
        size_t length = end ? (size_t)(end - cursor) : strlen(cursor);
        int count = 0;
        if (length >= MAX_LINE) length = MAX_LINE - 1;
        memcpy(line, cursor, length);
        line[length] = 0;
        cursor = end ? end + 1 : cursor + length;
        token = line;
        while (is_space((unsigned char)*token)) token++;
        if (!*token || *token == '#') continue;
        while (count < 8) {
            char *tab;
            fields[count++] = token;
            tab = strchr(token, '\t');
            if (!tab) break;
            *tab = 0;
            token = tab + 1;
        }
        if (count < 8 || sample_count >= MAX_SAMPLES) continue;
        {
            char *tail = fields[7] + strlen(fields[7]);
            Sample *row = &samples[sample_count];
            while (tail > fields[7] && is_space((unsigned char)tail[-1])) *--tail = 0;
            memset(row, 0, sizeof *row);
            copy_field(row->utc, sizeof row->utc, fields[0]);
            row->epoch = atoll(fields[1]);
            row->cycle = atoi(fields[2]);
            copy_field(row->event, sizeof row->event, fields[3]);
            row->gcc = atoi(fields[4]);
            row->fp = atoi(fields[5]);
            row->total = atoi(fields[6]);
            copy_field(row->head, sizeof row->head, fields[7]);
            sample_count++;
        }
    }
    free(text);
    return 1;
}

static int append_sample(const char *path, long long epoch, int cycle, const char *event,
                         int gcc, int fp, const char *head)
{
    /* Binary append keeps the log byte-stable: every row ends in a bare newline
       and .gitattributes marks the file -text, so no EOL rewriting happens. */
    FILE *file = fopen(path, "ab");
    struct tm *utc;
    time_t stamp = (time_t)epoch;
    char text[32];
    if (!file) return 0;
    utc = gmtime(&stamp);
    if (!utc || strftime(text, sizeof text, "%Y-%m-%dT%H:%M:%SZ", utc) == 0) {
        fclose(file);
        return 0;
    }
    fprintf(file, "%s\t%lld\t%d\t%s\t%d\t%d\t%d\t%s\n",
            text, epoch, cycle, event, gcc, fp, gcc + fp, head);
    if (fclose(file) != 0) return 0;
    return 1;
}

static void format_span(double hours, char *out, size_t size)
{
    if (hours < 1.0) snprintf(out, size, "%.0f min", hours * 60.0);
    else if (hours < 48.0) snprintf(out, size, "%.2f hours", hours);
    else snprintf(out, size, "%.2f days", hours / 24.0);
}

static void format_eta(double hours, char *out, size_t size)
{
    if (hours <= 0.0 || hours > 24.0 * 3650.0) { copy_field(out, size, "--"); return; }
    if (hours < 1.0) snprintf(out, size, "%.0f min", hours * 60.0);
    else if (hours < 48.0) snprintf(out, size, "%.1f hours", hours);
    else snprintf(out, size, "%.1f days", hours / 24.0);
}

/* Name a window rate, stating the span it really covers when the samples do
   not bracket the window, so a short or sparse history cannot look complete. */
static void format_window_rate(double per_day, double span_hours, double window_hours,
                               char *out, size_t size)
{
    if (span_hours < window_hours * 0.95 || span_hours > window_hours * 1.05) {
        char span[64];
        format_span(span_hours, span, sizeof span);
        snprintf(out, size, "%.2f/day over %s", per_day, span);
    } else {
        snprintf(out, size, "%.2f/day", per_day);
    }
}

/* Rate over the newest `window_hours` of wall clock. When the log does not
   reach back that far the earliest sample is used and the caller is told the
   span actually covered, so a short history never inflates the rate. */
static double window_rate(long long now, int total, double window_hours, double *span_hours)
{
    long long cutoff = now - (long long)(window_hours * 3600.0);
    int index = 0;
    int base = 0;
    int retired;
    while (index < sample_count && samples[index].epoch <= cutoff) base = index++;
    if (sample_count == 0) {
        *span_hours = 0.0;
        return 0.0;
    }
    retired = samples[base].total - total;
    *span_hours = (double)(now - samples[base].epoch) / 3600.0;
    if (*span_hours < 1.0 / 3600.0) *span_hours = 1.0 / 3600.0;
    return 24.0 * (double)retired / *span_hours;
}

static void usage(void)
{
    printf("usage: worker-status [--root DIR] [--log PATH] [--corpus PATH] [--first-party PATH]\n"
           "                     [--cycle N] [--event NAME] [--head REV] [--now EPOCH] [--append] [--last-cycle] [--brief]\n");
}

int main(int argc, char **argv)
{
    const char *root = ".";
    const char *log_path = 0;
    const char *corpus_path = 0;
    const char *list_path = 0;
    const char *event = "cycle";
    const char *head = "-";
    char log_buffer[1024];
    char corpus_buffer[1024];
    char list_buffer[1024];
    char *text;
    int cycle = -1;
    long long now_override = 0;
    int append = 0;
    int last_cycle = 0;
    int brief = 0;
    int gcc_left = 0;
    int fp_left = 0;
    int valid = 0;
    int index;
    long long now;
    int target;

    for (index = 1; index < argc; index++) {
        const char *key = argv[index];
        const char *value = index + 1 < argc ? argv[index + 1] : 0;
        if (strcmp(key, "--root") == 0 && value) { root = value; index++; }
        else if (strcmp(key, "--log") == 0 && value) { log_path = value; index++; }
        else if (strcmp(key, "--corpus") == 0 && value) { corpus_path = value; index++; }
        else if (strcmp(key, "--first-party") == 0 && value) { list_path = value; index++; }
        else if (strcmp(key, "--cycle") == 0 && value) { cycle = atoi(value); index++; }
        else if (strcmp(key, "--event") == 0 && value) { event = value; index++; }
        else if (strcmp(key, "--head") == 0 && value) { head = value; index++; }
        else if (strcmp(key, "--now") == 0 && value) { now_override = atoll(value); index++; }
        else if (strcmp(key, "--append") == 0) append = 1;
        else if (strcmp(key, "--last-cycle") == 0) last_cycle = 1;
        else if (strcmp(key, "--brief") == 0) brief = 1;
        else if (strcmp(key, "--help") == 0 || strcmp(key, "-h") == 0) { usage(); return 0; }
        else { fprintf(stderr, "worker-status: unknown or incomplete argument: %s\n", key); usage(); return 2; }
    }
    if (!log_path) {
        snprintf(log_buffer, sizeof log_buffer, "%s/Tests/progress/log.tsv", root);
        log_path = log_buffer;
    }
    if (!corpus_path) {
        snprintf(corpus_buffer, sizeof corpus_buffer, "%s/Tests/pedantic/gcc/corpus.json", root);
        corpus_path = corpus_buffer;
    }
    if (!list_path) {
        snprintf(list_buffer, sizeof list_buffer, "%s/Tests/progress/first-party-failures.txt", root);
        list_path = list_buffer;
    }
    /* A missing log is the first-ever run, not an error. */
    read_log(log_path);
    if (last_cycle) {
        printf("%d\n", sample_count ? samples[sample_count - 1].cycle : 0);
        return 0;
    }
    text = read_file(corpus_path);
    if (!text) {
        fprintf(stderr, "worker-status: cannot read corpus manifest: %s\n", corpus_path);
        return 2;
    }
    gcc_left = count_cases(text, &valid);
    free(text);
    if (!valid) {
        fprintf(stderr, "worker-status: no \"cases\" key in corpus manifest: %s\n", corpus_path);
        return 2;
    }
    text = read_file(list_path);
    if (!text) {
        fprintf(stderr, "worker-status: cannot read first-party list: %s\n", list_path);
        return 2;
    }
    fp_left = count_list(text);
    free(text);
    now = now_override > 0 ? now_override : (long long)time(0);
    if (append) {
        int label = cycle >= 0 ? cycle : (sample_count ? samples[sample_count - 1].cycle : 0);
        /* Keep the log monotonic: a stale counter from a worker that has not yet
           re-read the log must not write a cycle number that is already there. */
        if (sample_count && label <= samples[sample_count - 1].cycle) {
            label = samples[sample_count - 1].cycle + 1;
        }
        if (!append_sample(log_path, now, label, event, gcc_left, fp_left, head)) {
            fprintf(stderr, "worker-status: cannot append to progress log: %s\n", log_path);
            return 2;
        }
    }

    if (brief) {
        printf("Backlog at %s: GCC %d; first-party %d; total %d\n",
               head, gcc_left, fp_left, gcc_left + fp_left);
        if (sample_count) {
            const Sample *previous = &samples[sample_count - 1];
            printf("  Since recorded cycle %d (%s): GCC %+d, first-party %+d\n",
                   previous->cycle, previous->utc,
                   gcc_left - previous->gcc, fp_left - previous->fp);
        }
        printf("  Counts are inventory, not test results; scope removals are not repairs.\n");
        printf("  Acceptance: task.md; speed work: Performance/task.md\n");
        if (gcc_left + fp_left == 0) {
            printf("BACKLOG EMPTY: verify language, native gates, test speed and self-build before done.x\n");
            return 10;
        }
        return 0;
    }

    target = gcc_left + fp_left;
    for (index = 0; index < sample_count; index++) {
        if (samples[index].total > target) target = samples[index].total;
    }
    {
        int left = gcc_left + fp_left;
        int retired = target - left;
        double percent = target ? 100.0 * (double)retired / (double)target : 100.0;
        double span_hours = sample_count ? (double)(now - samples[0].epoch) / 3600.0 : 0.0;
        int overall_retired = sample_count ? samples[0].total - left : 0;
        double overall_day = span_hours > 0.0 ? 24.0 * (double)overall_retired / span_hours : 0.0;
        double day_span, hour_span, day_day, hour_day;
        char elapsed[64], eta_overall[64], eta_recent[64], recent[64];
        char day_text[96], six_hour_text[96];
        day_day = window_rate(now, left, 24.0, &day_span);
        hour_day = window_rate(now, left, 6.0, &hour_span);
        printf("CPrime test completion (%d sample%s in %s)\n",
               sample_count, sample_count == 1 ? "" : "s", log_path);
        printf("  left     %d of %d cases (GCC %d + first-party %d); completed %d (%.1f%%)\n",
               left, target, gcc_left, fp_left, retired, percent);
        if (!sample_count) {
            printf("  elapsed  no samples yet; baseline is this state\n");
            printf("  rate     --\n");
        } else {
            format_span(span_hours, elapsed, sizeof elapsed);
            printf("  elapsed  %s since baseline %s (%s)\n",
                   elapsed, samples[0].utc, samples[0].head);
            format_window_rate(day_day, day_span, 24.0, day_text, sizeof day_text);
            format_window_rate(hour_day, hour_span, 6.0, six_hour_text, sizeof six_hour_text);
            printf("  rate     overall %.2f/day (%.2f/hour) | 24h %s | 6h %s\n",
                   overall_day, overall_day / 24.0, day_text, six_hour_text);
        }
        if (sample_count > 1) {
            const Sample *previous = &samples[sample_count - 1];
            double gap_hours = (double)(now - previous->epoch) / 3600.0;
            int delta = previous->total - left;
            if (gap_hours < 1.0 / 60.0) {
                snprintf(recent, sizeof recent, "%d in under a minute", delta);
            } else {
                format_span(gap_hours, elapsed, sizeof elapsed);
                snprintf(recent, sizeof recent, "%d in %s (%.1f/day)",
                         delta, elapsed, 24.0 * (double)delta / gap_hours);
            }
            printf("  last     retired %s\n", recent);
        }
        {
            int trend = 0; /* -1 slipping, 0 steady, 1 improving */
            if (overall_day > 0.0) {
                if (hour_day > overall_day * 1.15) trend = 1;
                else if (hour_day < overall_day * 0.85) trend = -1;
            } else if (hour_day > 0.0) {
                trend = 1;
            }
            printf("  trend    %s (6h %.2fx overall)\n",
                   trend > 0 ? "improving" : (trend < 0 ? "slipping" : "steady"),
                   overall_day > 0.0 ? hour_day / overall_day : 0.0);
        }
        if (overall_day > 0.0) format_eta((double)left / overall_day * 24.0, eta_overall, sizeof eta_overall);
        else copy_field(eta_overall, sizeof eta_overall, "--");
        if (hour_day > 0.0) format_eta((double)left / hour_day * 24.0, eta_recent, sizeof eta_recent);
        else copy_field(eta_recent, sizeof eta_recent, "--");
        printf("  eta      %s at overall rate; %s at 6h rate\n", eta_overall, eta_recent);
        if (left == 0) {
            printf("TARGET REACHED: no retained GCC rows or first-party failures remain\n");
            return 10;
        }
    }
    return 0;
}
