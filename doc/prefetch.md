# Development of Data Prefetching and Extensions for TIER translator #
## Creating/Modifying a translator to capture requests for forward analysis. ##
### Devoloper-guide about xlators ###
#### Setting the Stage ####
1. Loading translator(tag:dlsym), [xlator.c](https://github.com/code2hack/glusterfs/blob/doc/libglusterfs/src/xlator.c)
[README](glusterfs/doc/README.md)

2. cbks
3. dumpops

#### init , fini , and private context ####
1. init
* Checking the translator is being set up in the right kind of environment.
  * Some translators are "initial" translators that **inject** requests into the system from elsewhere like kernel, network etc. Thus they have **no parents**.
  * Some are "final" translators(from the perspective of the current process) that they have no children.
  * Some are "multiplex" translators that they have multiple children.
* Allocating private data area. (tag:GF_CALLOC)
2. fini
free the private data.(tag:GF_FREE)

### Main thoughts about translators ###
1. Backgrounds  
  * [Call Stack](https://en.wikipedia.org/wiki/Call_stack)  
  ![ ](https://upload.wikimedia.org/wikipedia/commons/d/d3/Call_stack_layout.svg)  
2. Macros
  * STACK\_WIND\_COMMON(frame, rfn,has_cookie,cky, obj, fn, params ...)
    * frame:current frame?
    * rfn:callback unction
    * has_cookie:
    * cky:cookie:
    * obj:
    * fn:function to invoke
3. Data structure
  * xl          : pointer to the _in-memory_ xlator
  * xlator_fops : defined in [xlator.h](https://github.com/code2hack/glusterfs/blob/doc/libglusterfs/src/xlator.h)
    * call_frame:
      * call_stack:
        * pool:
          * [ ] stak\_mem\_pool
          * frame\_mem\_pool
        * client :
        * state  : pointer to **request state**
        * ctime  : timestamp,when to create.
      * timespec : **begin and end**
      * WIND and UNWIND, from and to.
  * xlator_cbks :
  * 
