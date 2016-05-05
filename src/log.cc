#include <log.h>
#include <string.h>
#include <cassert>
#include <fuzzy_log.h>
#include <iostream>

log* log::_log = NULL;
playback* playback::_playback = NULL;

FuzzyLog *fuzzy_log_new(__attribute__((unused)) uint32_t server_ip_addr, 
                        __attribute__((unused)) uint16_t server_port,
                        __attribute__((unused)) const uint32_t *relevent_chains,
                        __attribute__((unused)) uint16_t num_relevent_chains, 
                        fuzzy_log_callback callback)
{
        playback::create_playback(callback);
        return NULL;
}

ChainAndEntry fuzzy_log_append(__attribute__((unused)) FuzzyLog *fz, 
                               uint32_t chain,
                               const uint8_t *val, 
                               uint16_t len,
                               const ChainAndEntry* deps, 
                               uint16_t num_deps)
{
        log *log_ref;
        
        log_ref = log::create_log();
        return log_ref->append(chain, val, len, NULL, 0, deps, num_deps);
}

void fuzzy_log_multiappend(__attribute__((unused)) FuzzyLog *fz, 
                           uint32_t *chain, 
                           uint16_t num_chains,
                           const uint8_t *val, 
                           uint16_t len,
                           const ChainAndEntry* deps, 
                           uint16_t num_deps)
{
        log *log_ref;
        
        log_ref = log::create_log();
        log_ref->multi_append(chain, num_chains, val, len, deps, num_deps);
}

ChainAndEntry fuzzy_log_play_forward(__attribute__((unused)) FuzzyLog *log, 
                                     uint32_t chain)
{
        playback *pb;
        
        pb = playback::get_playback();
        return pb->play_chain(chain);
}


/*
 * Constructor for a single chain entry. Includes the following data + meta-data
 * 
 * 1) Causal dependencies -- ndeps + dep_ids
 * 2) Multi-put meta-data -- multi_sz + multi_ids
 * 3) Payload -- sz + payload
 */
chain_entry::chain_entry(uint32_t entry_id, uint16_t ndeps, 
                         const ChainAndEntry *dep_ids, 
                         uint16_t multi_sz,
                         const ChainAndEntry *multi_ids,
                         uint16_t sz,
                         const uint8_t *payload)
{
        _entry_id = entry_id;

        _ndeps = ndeps;
        _dep_ids = (ChainAndEntry*)malloc(sizeof(ChainAndEntry)*ndeps);
        memcpy(_dep_ids, dep_ids, sizeof(ChainAndEntry)*ndeps);
        
        _multi_sz = multi_sz;
        _multi_ids = (ChainAndEntry*)malloc(sizeof(ChainAndEntry)*multi_sz);
        memcpy(_multi_ids, multi_ids, sizeof(ChainAndEntry)*multi_sz);
        
        _sz = sz;
        _payload = (uint8_t*)malloc(sizeof(uint8_t)*sz);
        memcpy(_payload, payload, sizeof(uint8_t)*sz);
}

/* Get causal dependencies */
void chain_entry::get_deps(uint16_t *ndeps, ChainAndEntry **dep_ids)
{
        *ndeps = _ndeps;
        *dep_ids = _dep_ids;
}

/* Get multi-put meta-data */
void chain_entry::get_multis(uint16_t *multi_sz, ChainAndEntry **multi_ids)
{
        *multi_sz = _multi_sz;
        *multi_ids = _multi_ids;
}

/* Get payload */
void chain_entry::get_payload(uint16_t *sz, uint8_t **payload)
{
        *sz = _sz;
        *payload = _payload;
}

uint32_t chain_entry::get_entry_id()
{
        return _entry_id;
}

log::log()
{
        _map.clear();
        _multi_map.clear();
}

log* log::create_log() 
{
        if (_log == NULL) 
                _log = new log();
        return _log;
}

/* 
 * Appends an entry to a chain. Takes causal dependencies, multi-put meta-data, 
 * and payload. 
 */
ChainAndEntry log::append(uint32_t chain, const uint8_t *val, 
                          uint16_t len,
                          const ChainAndEntry *multi_ids,
                          uint16_t multi_sz,
                          const ChainAndEntry *deps, 
                          uint16_t num_deps)
{
        chain_entry *c_entry;
        vector<chain_entry*> *list;
        ChainAndEntry ret;

        if (_map.count(chain) == 0) {
                list = new vector<chain_entry*>();
                _map[chain] = list;
        } else {
                list = _map[chain];
        }

        c_entry = new chain_entry(list->size(), num_deps, deps, multi_sz, 
                                  multi_ids, 
                                  len, 
                                  val);
        list->push_back(c_entry);
        
        ret.chain = chain;
        ret.entry = c_entry->get_entry_id();
        return ret;        
}

/* Append multiple entries atomically to several chains */
void log::multi_append(uint32_t *chain, uint16_t num_chains, 
                       const uint8_t *val, 
                       uint16_t len,
                       const ChainAndEntry *deps, uint16_t num_deps)
{
        ChainAndEntry multi_entries[num_chains];
        uint32_t i;

        /* Create vector timestamp associated with multi-put */
        for (i = 0; i < num_chains; ++i) {
                multi_entries[i].chain = chain[i];
                if (_map.count(chain[i]) == 0) 
                        multi_entries[i].entry = 0;
                else 
                        multi_entries[i].entry = _map[chain[i]]->size();
        }
        
        for (i = 0; i < num_chains; ++i) 
                append(chain[i], val, len, multi_entries, num_chains, deps, 
                       num_deps);
}

/* Get an array corresponding to a single chain. */
vector<chain_entry*>* log::get_chain(uint32_t chain)
{
        if (_map.count(chain) == 0)
                return NULL;
        return _map[chain];
}

void playback::create_playback(fuzzy_log_callback callback)
{
        log *lg;
        
        lg = log::create_log();
        if (_playback == NULL)
                _playback = new playback(lg, callback);
}

playback* playback::get_playback() 
{
        assert(_playback != NULL);
        return _playback;
}

/* Used to playback fuzzy log on a single client */
playback::playback(log *lg, fuzzy_log_callback callback)
{
        _cursor.clear();
        _log = lg;
        _callback = callback;
}

bool playback::already_processed(ChainAndEntry addr)
{
        uint32_t cursor;
        
        if (_cursor.count(addr.chain) == 0)
                return false;
        
        cursor = _cursor[addr.chain];
        return cursor >= addr.entry;
}

void playback::play_forward(ChainAndEntry max_entry)
{
        uint8_t *payload;
        uint16_t payload_sz, num_causal, num_multi;
        uint32_t i;
        ChainAndEntry *causal, *multi, temp;
        vector<chain_entry*> *cur_chain;
        chain_entry *entry;

        if (already_processed(max_entry))
                return;        
        
        cur_chain = _log->get_chain(max_entry.chain);
        assert(cur_chain->size() > max_entry.entry);
        entry = (*cur_chain)[max_entry.entry];

        /* Ensure that preceding chain entries are played back */
        temp = max_entry;
        if (max_entry.entry > 0) {
                temp.entry -= 1;
                play_forward(temp);
        }

        /* Ensure that causal dependencies are satisfied */
        entry->get_deps(&num_causal, &causal);
        for (i = 0; i < num_causal; ++i) 
                play_forward(causal[i]);

        /* Ensure that multi-put dependencies are satisfied */
        entry->get_multis(&num_multi, &multi);
        for (i = 0; i < num_multi; ++i) {
                temp = multi[i];
                if (temp.entry > 0) {

                        /* entries before the multi-put must be played back */
                        temp.entry -= 1;
                        play_forward(temp);
                }
                _cursor[multi[i].chain] = multi[i].entry;
        }
        
        /* Play the entry */
        entry->get_payload(&payload_sz, &payload);
        _callback(0, NULL, payload, payload_sz);
        _cursor[max_entry.chain] = max_entry.entry;
}

/* 
 * Play forward a particular chain. Ensures that every entry that should 
 * precede the tail of the current chain is also played back. 
 */
ChainAndEntry playback::play_chain(uint32_t chain)
{
        vector<chain_entry*> *entries;
        ChainAndEntry addr;
        
        addr.chain = chain;
        entries = _log->get_chain(chain);
        if (entries == NULL) {
                addr.entry = 0;
                return addr;
        }

        addr.entry = entries->size() - 1;        
        play_forward(addr);
        addr.entry += 1;
        return addr;
}
