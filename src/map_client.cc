#include <stdint.h>
#include <unordered_map>
#include <deque>
#include <fuzzy_log.h>
#include <iostream>
#include <cassert>

// unordered_map<uint32_t, deque<uint32_t> > *global_map;
FuzzyLog *log_handle;

uint8_t callback(__attribute__((unused)) uint32_t nentries, 
                 __attribute__((unused)) ChainAndEntry *entry_list, 
                 const uint8_t *entry, 
                 __attribute__((unused)) uint16_t sz)
{
        uint32_t entry_val;

        entry_val = *((uint32_t*)entry);
        std::cout << entry_val << "\n";
        return 0;
}               

/* Use the chain API to insert something */
void insert(uint32_t chain, uint32_t value)
{
        fuzzy_log_append(log_handle, chain, (const uint8_t*)&value, sizeof(uint32_t), NULL, 0);
}

void insert_multi()
{
        uint32_t chains[2], val;
        
        val = 100;
        chains[0] = 0;
        chains[1] = 1;
        fuzzy_log_multiappend(log_handle, chains, 2, (const uint8_t*)&val, sizeof(uint32_t), NULL, 0);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv)
{
        uint32_t chains[2], i, nitems;
        
        /* Get a fuzzy log handle */
        chains[0] = 0;
        log_handle = fuzzy_log_new(0, 13265, chains, 1, callback);

        /* Insert a bunch of stuff */
        nitems = 100;
        for (i = 0; i < nitems; ++i) {
                if (i % 2 == 0)
                        insert(0, i);      
                else
                        insert(1, i);
        }

        insert_multi();

        /* Read a bunch of stuff */
        fuzzy_log_play_forward(log_handle, 0);
        fuzzy_log_play_forward(log_handle, 1);
}

