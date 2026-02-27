#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

// https://stackoverflow.com/questions/9596945/how-to-get-appropriate-timestamp-in-c-for-logs
char *timestamp()
{
  time_t ltime; /* calendar time */
  ltime = time(NULL); /* get current cal time */
  return asctime(localtime(&ltime));
}

#define DEFAULT_CONF_GB 1
#define DEFAULT_CONF_DELAY 30

typedef struct
{
  int gb;
  int delay;
  bool force;
} run_config;

bool parse_args(int argc, char** argv, run_config* conf)
{
  for (int i = 0; i < argc;)
  {
    if (0 == strcmp("-g", argv[i]))
    {
      if (i + 1 >= argc)
      {
        return false;
      }

      conf->gb = strtol(argv[i + 1], NULL, 10);
      if (conf->gb < 1 || conf->gb > 1024 * 1024 /* 1 petabyte */)
      {
        return false;
      }
      i += 2;
    }
    else if (0 == strcmp("-d", argv[i]))
    {
      if (i + 1 >= argc)
      {
        return false;
      }

      conf->delay = strtol(argv[i + 1], NULL, 10);
      if (conf->delay < 1 || conf->delay > 365 * 86400 /* 1 year */)
      {
        return false;
      }
      i += 2;
    }
    else if (0 == strcmp("-f", argv[i]))
    {
      conf->force = true;
      i++;
    }
    else
    {
      // Unknown option
      return false;
    }
  }

  return true;
}

int main(int argc, char** argv) {
  run_config conf = { .gb = DEFAULT_CONF_GB, .delay = DEFAULT_CONF_DELAY, .force = false };
  if (!parse_args(argc - 1, argv + 1, &conf))
  {
    printf("%s usage:\n", argv[0]);
    printf("\t-g <gigabytes>: memory allocation size\n");
    printf("\t-d <seconds>: delay interval between checks\n");
    printf("\t-f: force acceptance of questionable parameters\n");
    return 1;
  }

  if (!conf.force && (conf.delay < 5 || conf.gb > 8))
  {
    printf("Requested more than 8 GB allocation or less than 5 seconds delay!\n");
    printf("Use -f to force if you are really sure this is what you want!\n");
    return 1;
  }

  size_t bytes = conf.gb * 1073741824UL;
  unsigned int tests = 0;
  unsigned int total = 0;

  printf("=== Bitflipped ===\n");
  printf("==================\n");
  printf("Allocating %d GB...\n", conf.gb);
  unsigned char *buffer = (unsigned char *)calloc(bytes, 1);

  // Ensure the buffer is actually resident in RAM
  mlock(buffer, bytes);
  memset(buffer, 0, bytes);

  printf("Run started: %s\n", timestamp());

  fflush(stdout);
  while (total == 0) {
    // We aren't going to miss a bitflip by being slow
    sleep(conf.delay);

    // Naively walk through and tally all zero bytes
    for (size_t i = 0; i < bytes; ++i) {
      total += buffer[i];
    }

    // Keep the user sane that it isn't frozen :)
    fprintf(stderr, "\rTest run #%d (every %ds)", tests, conf.delay);
    ++tests;
  }

  printf("--- !!! ---");
  printf("Error detected: %s\n", timestamp());
  printf("Result should be 0 but is %d\n", total);
  printf("Total tests run: %d\n", tests);
  fflush(stdout);
}
