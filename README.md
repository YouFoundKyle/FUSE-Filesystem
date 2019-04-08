##Implemented a basic filesystem
######a FUSE filesystem driver that mounts a 1MB disk image (data file) as a filesystem.


Provides functionality for the following features:

*   Create, delete, and rename files.
*   List the files in the filesystem root directory

*   Read and write from files larger than one block. For example, you should be able to support one hundred 1k files or five 100k files.

*   Create directories and nested directories. Directory depth should only be limited by disk space (and possibly the POSIX API).

*   Remove directories.

*   Hard links.

*   Symlinks.

*   Supports metadata (permissions and timestamps) for files and directories.

