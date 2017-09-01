#!/bin/sh
# scross@chumby.com

# Turn on the backlight
echo "100" > /sys/devices/platform/stmp3xxx-bl/backlight/stmp3xxx-bl/brightness

# We are a robot.  Clear the screen and say so.
dd if=/dev/zero of=/dev/fb0 bs=640 count=240
echo -n "Entering robot mode" | fbwrite --color=255,255,255 --pos=1,2 -


# Bring up udev, which will handle most (if not all) drivers.
# This needs to happen after /psp/ is mounted, because udev has
# some config files that live in /psp/.
mount /dev
mknod /dev/null c 1 3
mkdir /dev/oprofile
mkdir /dev/pts
mount /dev/pts
udevd --daemon


# Run ntpd to keep time in sync even without the network (using the driftfile)
#if [ ! -e /psp/ntp.drift ]
#then
#    echo -140.000 > /psp/ntp.drift
#fi
#ntpd -g -x -f /psp/ntp.drift


# Now, issue udev triggers for all devices that have already been created.
# This'll get us devices like /dev/null.
udevadm trigger
# Wait for all that noise to die down.
udevadm settle


# Insert the USB module, and just for safety, reset the USB system.  We're
# trying to prevent ETIMEDOUT messages on the rt73, which seems to not like
# it if you try to talk to it too soon.
reset_usb.sh
modprobe ehci_hcd
# Wait for things to quiet down after the USB reset
udevadm settle


# Fire off syslog.  This quiets down the boot screen a little.
/sbin/syslogd -C200


# Mount the USB partition, which should be available now that USB is loaded.
mount /proc/bus/usb


# Load the i2c interface.  This will enable the dcid query program to talk to
# the dcid eeprom.
modprobe i2c-dev
modprobe i2c-stmp378x


# Bring audio up
enable_audio.sh


# Set up the rotary coder, which is present on some units.
modprobe stmp3xxx_rotdec


# Start the accelerometer daemon.                                                                          
acceld &                                                                                                   
                                                                                                           
                                                                                                           
# Assign the hostname.  Generate a default one if one doesn't exist.
if [ -e /psp/hostname ]
then
    cat /psp/hostname > /proc/sys/kernel/hostname
    HOSTNAME="$(hostname)"
fi
if [ ! -e /psp/hostname -o "${HOSTNAME}" = "chumby" -o "${HOSTNAME}" = "(none)" ]
then
    HOSTNAME=chumby-$(ifconfig wlan0 | grep HWaddr | awk '{print $5}' | cut -d':' -f4,5,6 | tr ':' '-' | tr A-Z a-z )
    echo "${HOSTNAME}" > /psp/hostname
    echo "Creating default hostname: ${HOSTNAME}"
fi
cat /psp/hostname > /proc/sys/kernel/hostname


# Bring up the "local" network interface.  Other network interfaces will
# be brought up as their devices are discovered via udev events.
ifconfig lo 127.0.0.1 &


# Start the network if we have a network config script.
if [ -e /psp/network_config ]
then
    (start_network &&
     /usr/sbin/httpd -h /mnt/storage/www &&
     ifconfig wlan0 # | grep "inet addr" | cut -d' ' -f11-12 | fbwrite --color=255,255,255 --pos=1,3 -
    ) &
else
     echo -n "No Network.  Chumby mode on next boot." | fbwrite --color=255,255,255 --pos=1,3 -
     rm /mnt/storage/be_a_robot &
fi


# Run crond by uncommenting the following line
#/usr/sbin/crond -c /etc/cron/crontabs -S


# Start the Demand Peripherals user-space daemon
if [ -e /mnt/storage/dp/DPCore.bin  -a -c /dev/ttyUSB0 ]
then
    sh /mnt/storage/dp/start_baseboard.sh &
fi

# Run Jul's stuff
/mnt/storage/jul/clock &
sleep 5
btplayd &
amixer sset DAC 210
# /mnt/storage/jul/dimmer /dev/input/by-id/soc-noserial-event-joystick /sys/devices/platform/stmp3xxx-bl/backlight/stmp3xxx-bl/brightness &
/mnt/storage/jul/input_handler /dev/input/by-id/soc-noserial-event-kbd /dev/input/by-id/soc-noserial-event-joystick /sys/devices/platform/stmp3xxx-bl/backlight/stmp3xxx-bl/brightness &
sleep 60
/mnt/storage/jul/netchecker.sh &
