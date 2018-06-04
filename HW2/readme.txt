made by Jeon Sang hyun

driver name : dev_driver
major number : 242
usage :

(host)
source /root/.bashrc
cd module/
make
adb push dev_driver.ko /data/local/tmp
make clean
cd ..
cd app/
make
adb push app /data/local/tmp
make clean
cd ..

(board)
echo "7 6 1 7" > /proc/sys/kernel/printk
mknod /dev/dev_driver c 242 0
insmod dev_driver.ko
./app [interval] [count] [options]
rmmod dev_driver.ko
rm -rf dev_driver.ko
rm -rf app
