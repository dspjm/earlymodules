./unload_umdm.sh
./load_umdm.sh
echo '4-1:1.0' > /sys/bus/usb/drivers/usbhid/unbind
echo '4-1:1.0' > /sys/bus/usb/drivers/umdm_driver/bind
