# The track xlator #

## Data Structure ##
### track_conf ###

> eh_t    *history;

This struct which resides in struct xlator as _history_ maintains a buffer for event histories.

## Thread Management ##
The _dump\_thread_ in _track\_conf_ is the handle of dump thread;  
The 

## Memory management ##

A new type of memory should be registered in _enum gf\_common\_mem\_types_.