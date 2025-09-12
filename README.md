# qfuse

**qfuse** is a virtual file system for KDB/Q that unifies multiple HDBs into a single VDB. It actively 
maintains a mapping of all files in source historical databases (HDBs) and collates their content into 
a single mounted directory simulating a virtual database (VDB). This utilizes FUSE which enables non-root 
applications to interact with POSIX disk commands on a mounted directory.

Detailed write can be found here:
https://jparmstrong.com/vdbs-with-vfs/

## Build and Run

qfuse has been tested with Debian Linux 12 on an AMD64 workstation. 

Only dependency is libfuse3 library and standard C.

```sh
sudo apt-get install libfuse3-dev
```

Update config.csv to suit your needs.

```sh
# build
make clean build

# run using "vdb" folder mount
./build/qfuse config.csv -f vdb


# to unmount 
fusermount -u vdb
```
