#include "fanju.h"

#include <cstdio>
#include <time.h>

void print_data(float temp, uint8_t hum, bool bat_ok, bool tx_req, uint8_t chan)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    printf("%s,%.1f,%u,%u,%u,%u\n", asctime(timeinfo), temp, hum, bat_ok, tx_req, chan);
}

int main(int, char **)
{
    int ret = fanju::setup(print_data);
    while (ret == 0)
        fanju::loop();
    return ret;
}
