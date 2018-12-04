# Development of Data Prefetching and Extensions for TIER translator #
## Creating/Modifying a translator to capture requests for forward analysis. ##
### Main thoughts about translators ###
1. Backgrounds  
  * [Call Stack](https://en.wikipedia.org/wiki/Call_stack)  
  ![ ](https://upload.wikimedia.org/wikipedia/commons/d/d3/Call_stack_layout.svg)  
2. Macros
  * STACK_WIND:Adding subroutine's entry to the call stack.  
3. Data structure
  * xl          : pointer to the _in-memory_ xlator
  * xlator_fops : defined in [xlator.h](https://github.com/code2hack/glusterfs/blob/doc/libglusterfs/src/xlator.h)
    * call_frame:
    * call_stack:   
      * client:
      * state:pointer to **request state**
      * ctime:timestamp.
  * xlator_cbks :
  * 
