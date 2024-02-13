Collect performance info for all roam program types:

`./time_all.sh -sjnt_off:1 -ik_off:1 -smap:0 -draw_2d_disable:1`

***
```
git clone --depth 1 https://github.com/glebnovodran/roam_bench.git
cd roam_bench
./build_all.sh dummygl
cp ./src/cmd/time_all.sh .
# apt install time
# echo performance > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
./time_all.sh -sjnt_off:1 -ik_off:1 -smap:0 -draw_2d_disable:1
```
***

<br>

***
|CPU/prog| native | lua | wrench | qjs | pint | minion |
|--------|--------|-----|--------|-----|------|--------|
| Cortex-A35, 1.2GHz | 11.52 | 12.37 | 13.14 | 13.93 | 45.39 | 12.80 |
| Cortex-A53, 1.0GHz | 09.78 | 10.22 | 11.06 | 11.46 | 40.35 | - |
| Cortex-A15, 1.5GHz | 06.63 | 06.91 | 07.22 | 07.54 | 24.79 | - |
| Cortex-A72, 1.8GHz | 03.52 | 03.69 | 03.94 | 04.25 | 13.21 | - |
| RISC-V SiFive U74-MC, 1.5GHz | 10.15 | 10.78 | 11.61 | 11.82 | 34.83 | - |
| i7-3770 @ 1.6GHz   | 03.22 | 03.41 | 03.72 | 03.98 | 11.53 | - |
| AMD Ryzen 7 4700U @ 3.6GHz | 01.46 | 01.49 | 01.55 | 01.57 | 4.34 | 01.53 |
***
