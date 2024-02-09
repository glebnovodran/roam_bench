***
```
git clone --depth 1 https://github.com/glebnovodran/roam_bench.git
cd roam_bench
curl -o OGL_inc.tar.gz https://schaban.github.io/OGL_inc.tar.gz
tar -xzf OGL_inc.tar.gz
ALT_DEFS="-DDUMMY_GL" ALT_LIBS="" MACOS_SYSNAME="none" ./build.sh -O3 -flto
```
***