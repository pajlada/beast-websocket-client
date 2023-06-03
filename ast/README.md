## Environment variables

The following environment variables can be configured to change the behaviours of this project

 - `LLVM_PATH`  
   Will be used to change where to find the `clang++` executable file.  
   Example: `LLVM_PATH=/opt/llvm` ./get-builtin-include-dirs.py will use `/opt/llvm/bin/clang++`

 - `LIBCLANG_LIBRARY_FILE`  
   Will be used to change where clang cindex can find the dynamic library.  
   Must be an absolute path to the file.  
   Example: `LIBCLANG_LIBRARY_FILE=/opt/llvm/lib/libclang.so.15.0.7`

 - `LIBCLANG_LIBRARY_PATH`  
   Will be used to change where clang cindex can look for the dynamic library.  
   Must be an absolute path to the directory.  
   Example: `LIBCLANG_LIBRARY_PATH=/opt/llvm/lib`