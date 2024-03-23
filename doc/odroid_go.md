This benchmark can be compiled on the device itself.


First connect to your Odroid device through SSH. You can find out the IP address of your Odroid device in Settings/Network Info. The default password for 'odroid' user is 'odroid'.

`ssh odroid@<ip>`

At this point it makes sense to run the following command to maximize the device performance.

`/usr/local/bin/perfmax`

Next, install build-essential and libdrm-dev.

`sudo apt install build-essential libdrm-dev`

Compile with

`ALT_DEFS="-DEGL_API_FB -D__DRM__ -DDRM_ES -I/usr/include/libdrm" ALT_LIBS="-lgbm -ldrm -lEGL -lGLESv2" ./build_all.sh`

Run with

`./run.sh -glsl_echo:1 -smap:256 -vl:1 -viewrot:1 ...`

`-viewrot:1` is to set the landscape mode. The default is the portrait mode (`-viewrot:0`). The portrait mode is somewhat faster.