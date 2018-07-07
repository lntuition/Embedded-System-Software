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

(board)
echo "7 6 1 7" > /proc/sys/kernel/printk
mknod /dev/dev_driver c 242 0
chmod 777 dev_driver.ko
insmod dev_driver.ko

Start Application "Let"

rmmod dev_driver.ko
rm -rf dev_driver.ko