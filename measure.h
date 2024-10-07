#ifndef MEASURE_H
#define MEASURE_H

#define TXT(x) XTXT(x)
#define XTXT(x) #x

#define DEFAULT_POINTS 1000

int measure(size_t argc, char *argv[]);
int show_data(size_t argc, char *argv[]);

#endif //MEASURE_H
