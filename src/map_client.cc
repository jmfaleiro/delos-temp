#include <stdint.h>
#include <unordered_map>
#include <deque>
#include <fuzzy_log.h>
#include <iostream>
#include <cassert>

// unordered_map<uint32_t, deque<uint32_t> > *global_map;
FuzzyLog *log_handle;

uint8_t callback(const uint8_t *entry, uint16_t sz)
{
        uint32_t entry_val;

        entry_val = *((uint32_t*)entry);
        std::cout << entry_val << "\n";
        return 0;
}               

/* Use the chain API to insert something */
void insert(uint32_t chain, uint32_t value)
{
        assert(log_handle != NULL);
        fuzzy_log_append(log_handle, 0, (const uint8_t*)&value, sizeof(uint32_t), NULL, 0);
}

int main(int argc, char **argv)
{
        uint32_t chains[1], i, nitems;
        
        /* Get a fuzzy log handle */
        chains[0] = 0;
        log_handle = fuzzy_log_new(0, 13265, chains, 1, callback);

        /* Insert a bunch of stuff */
        nitems = 100;
        for (i = 0; i < nitems; ++i)
                insert(0, i);        

        /* Read a bunch of stuff */
        fuzzy_log_play_forward(log_handle, 0);
}

