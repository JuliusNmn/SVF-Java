## 0. Install libs required for build

```
sudo apt-get install zlib1g-dev unzip cmake gcc g++ libtinfo5 nodejs 
```

## 1.  get or build SVF 2.9 
```
npm install svf-lib@1.0.1838 --prefix ~/prefix
```
OR
```
 git clone --depth 1 --branch SVF-2.9 git@github.com:SVF-tools/SVF.git
 cd SVF
 ./build.sh
 
```

## 2. configure & build project 
cmake the project (`cmake -DCMAKE_BUILD_TYPE=Debug .` for debug build)
```
mkdir build
cd build
cmake .. -DSVF_DIR="$HOME/SVF" -DLLVM_DIR="$HOME/SVF/llvm-14.0.0.obj" -DZ3_DIR="$HOME/SVF/z3.obj"
```

to build the native target, run `make`.

to build the java target, run `make jar` (this will also build the native target)
## 3. Run the project

### Analyse file through native executable:
```
./bin/svf-ex libnative.bc
```
### Analyze a bc file using svf-java jar executable
```
java -jar target/svfjava.jar libnative.bc
```

## Building example file

Build example bitcode file:
```
cd xl_llvm
make
```
This requires setting `JAVA_HOME`, so `jni.h` can be found . Also, clang-14 must be in `$PATH`.
You can find it in `${HOME}/SVF/llvm-14.0.0.obj` .