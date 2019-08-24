# Tips #
## Centos ##
## How to Build and Install Glusterfs ##
[Building-Glusterfs](https://docs.gluster.org/en/latest/Developer-guide/Building-GlusterFS/)
> yum install autoconf automake bison cmockery2-devel dos2unix flex fuse-devel glib2-devel libacl-devel libaio-devel libattr-devel libcurl-devel libibverbs-devel librdmacm-devel libtirpc-devel libtool libxml2-devel lvm2-devel make openssl-devel pkgconfig pyliblzma python-devel python-eventlet python-netifaces python-paste-deploy python-simplejson python-sphinx python-webob pyxattr readline-devel rpm-build sqlite-devel systemtap-sdt-devel tar userspace-rcu-devel

Enabling EPEL  
> wget https://dl.fedoraproject.org/pub/epel/7/x86_64/Packages/e/epel-release-7-11.noarch.rpm

Install liburcu  
> yum install userspace-rcu-devel

## Setting Servers ##
Create cluster 
> gluster peer probe <server>
> gluster peer status  

Partition and mount 

> mkfs.xfs -i size=512 /dev/sdb
> mkdir -p /export/sdb/brick
> vi /etc/fstab
> mount -a

Setting up Volumes 
> gluster volume create [stripe | replica | disperse] [transport tcp | rdma | tcp,rdma]  
Example:
> gluster volume create repica 3 s1:/export/sdb/brick s2:/export/sdb/brick s3:/export/sdb/brick s1:/export/sdc/brick s2:/export/sdc/brick s3:/export/sdc/brick transport tcp

## Administration Guide ##
Starting glusterd automatically.
> chkconfig glusterd on

### Managing Trusted Storage Pool ###
