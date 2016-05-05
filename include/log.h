#ifndef 	LOG_H_
#define 	LOG_H_

#include <unordered_map>
#include <vector>
#include <fuzzy_log.h>

using namespace std;

struct FuzzyLog {
        uint32_t 	_blah;
};

class chain_entry {
 private:        
        uint32_t  		_entry_id;
        uint16_t 		_ndeps;
        ChainAndEntry 		*_dep_ids;
        uint16_t 		_multi_sz;
        ChainAndEntry 		*_multi_ids;
        uint16_t 		_sz;
        uint8_t 		*_payload;

 public:
        chain_entry(uint32_t entry_id, uint16_t ndeps, 
                    const ChainAndEntry *dep_ids, 
                    uint16_t multi_sz,
                    const ChainAndEntry *multi_ids,
                    uint16_t sz,
                    const uint8_t *payload);        
        
        uint32_t get_entry_id();
        void get_deps(uint16_t *ndeps, ChainAndEntry **dep_ids);
        void get_multis(uint16_t *multi_sz, ChainAndEntry **multi_ids);
        void get_payload(uint16_t *sz, uint8_t **payload);
};

class log {
 private:
        log();

        static log 					*_log;
        unordered_map<uint32_t, vector<chain_entry*>* >	_map;
        unordered_map<uint32_t, uint32_t>		_multi_map;

 public:
        static log* create_log();

        ChainAndEntry append(uint32_t chain, const uint8_t *val, 
                             uint16_t len, 
                             const ChainAndEntry *multi_ids,
                             uint16_t multi_sz,
                             const ChainAndEntry *deps, 
                             uint16_t num_deps);
        
        void multi_append(uint32_t *chain, uint16_t num_chains, 
                          const uint8_t *val, 
                          uint16_t len, 
                          const ChainAndEntry *deps, uint16_t num_deps);

        vector<chain_entry*>* get_chain(uint32_t chain);
};

class playback {
 private:
        playback();
        playback(log *lg, fuzzy_log_callback callback);
        
        log 					*_log;        
        fuzzy_log_callback 			_callback;        
        unordered_map<uint32_t, uint32_t> 	_cursor;        
        static playback 			*_playback;
        
        void play_forward(ChainAndEntry addr);
        bool already_processed(ChainAndEntry addr);

 public:
        static void create_playback(fuzzy_log_callback callback);
        static playback* get_playback();
        ChainAndEntry play_chain(uint32_t chain);        
};

#endif 		// LOG_H_
