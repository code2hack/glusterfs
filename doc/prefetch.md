# Development of Data Prefetching and Extensions for TIER translator #
## Creating/Modifying a translator to capture requests for forward analysis. ##
### Main thoughts about translators ###
1. Macros
  * STACK_WIND
2. Data structure
  * xl          : pointer to the _in-memory_ xlator
  * xlator_fops : defined in [xlator.h](https://github.com/code2hack/glusterfs/blob/doc/libglusterfs/src/xlator.h)
    * call_frame:
    * call_stack: [wiki:call stack](https://en.wikipedia.org/wiki/Call_stack) 
      * client:
      * state:pointer to **request state**
      * ctime:timestamp.
  * xlator_cbks :
  * 
