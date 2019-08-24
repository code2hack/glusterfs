# Development of Data Prefetching and Extensions for TIER translator #
## Creating/Modifying a translator to capture requests for forward analysis. ##
### Devoloper-guide of xlators ###
#### Setting the Stage ####
1. Loading translator(tag:dlsym), [xlator.c](./libglusterfs/src/xlator.c)

2. cbks
3. dumpops

#### init , fini , and private context ####
init  
* Checking the translator is being set up in the right kind of environment.
  * Some translators are "initial" translators that **inject** requests into the system from elsewhere like kernel, network etc. Thus they have **no parents**.
  * Some are "final" translators(from the perspective of the current process) that they have no children.
  * Some are "multiplex" translators that they have multiple children.
* Allocating private data area. (tag:GF_CALLOC)
fini  
free the private data.(tag:GF_FREE)

#### Building ####
#### Modifying volfile ####

### Source codes review ###
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






### The [_io-stats_](xlators/debug/io-stats/src/io-stats.c) Translator ###
#### Data Structure ####
![From TLPI](./img/FD-and-Inodes.jpg)
(tag:_ios\_conf_)  
io-stats xlator's private data.  
(tag:_ios\_fd_)  
* filename
* opened_at
(tag:\_fd)
* pid
* \_inode
* \_ctx:A dictionary referred to each xlator. xlator's pointer -- value?
(tag:\_inode)

[**\_inode**](./libglusterfs/src/inode.c)  

#### Functions ####
> fd\_
> ios\_bump\_read (xlator\_t \*this, fd\_t \*fd, size\_t len)

> inode_t * inode_link (inode_t *inode, inode_t *parent, const char* name, struct iatt *iatt)
>