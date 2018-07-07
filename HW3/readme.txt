made by Jeon Sang hyun

driver name : stopwatch
major number : 242

usage :

(host)
source /root/.bashrc
cd module/
make
adb push stopwatch.ko /data/local/tmp
make clean
cd ..
cd app/
make
adb push app /data/local/tmp
make clean
cd ..

(board)
echo "7 6 1 7" > /proc/sys/kernel/printk
mknod /dev/stopwatch c 242 0
insmod stopwatch.ko
./app
rmmod stopwatch.ko

