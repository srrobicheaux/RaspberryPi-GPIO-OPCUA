# RaspberryPi-GPIO-OPCUA
An OPCUA Client and Server with a simple PHP page tied to the client to control GPIO on the server.

I installed the Raspian imager on a computer from:
https://www.raspberrypi.org/blog/raspberry-pi-imager-imaging-utility/

I downloaded the new Ubuntu IoT Image file from:
https://ubuntu.com/download/raspberry-pi

I used the Raspian Imager selecting other image and then browsing to the Ubuntu IoT Image file I had down loaded.
I then wrote the image onto (over) a USB/sd card and then used this as my OS on a Raspberry Pi 3b+.
sudo apt-get update
sudo apt-get upgrade


I installed the preprequisites for open62541.
 sudo apt-get install git build-essential gcc pkg-config cmake python


I then cloned open62541 into the pi user directory.

git clone https://github.com/open62541/open62541.git
mkdir build
cd build
cmake ..
ccmake . (selected options for amalgamation and encryption)
sudo make install
gcc -c open62541.c -o open62541.o
gcc open62541.o RaspOPCUA.c -o RaspOPC -lmbedtls -lmbedx509 -lmbedcrypto
sudo ./RaspOPC

installed as a service to start at boot
sudo vi /etc/systemd/system/opcua.service
added the following content:

[Unit]
Description=OPCUA Service for GPIO devices
Requires=systemd-networkd.socket

[Service]
ExecStart=/sbin/RaspOPC
Type=idle
RestartSec=5

[Install]
WantedBy=multi-user.target
Alias=opcua.service

sudo chmod 664 /etc/systemd/system/opcua.service
systemctl is-active opcua.service
active

rebooted
sudo init 6
systemctl is-active opcua.service
active

Now OPCUA Server is running.

install apache2
sudo apt-get install apache2
test web server
install php
add php files to /var/www/html
load php file
change web folder permissions
sudo chmod -R 400 /var/www/html/*
 sudo usermod -a -G www-data pi

compile toggle.c similar to RaspOPC above.
test toggle
./toggle -node_str 85 -namespace 0 opc.tcp://gpio.process.pw:4840

sudo vi ../apache2.conf 
configure https
sudo vi 000-default-le-ssl.conf 
repoint http to https
sudo vi 000-default.conf 

request domain name from google
install letsencrypt
run certbot
restart apache
sudo apachectl restart

setup password for website via htpasswd

I installed Visual Code via Headmelted
https://github.com/headmelted/codebuilds

I installed the gpiod libraries to develop against them.
 2122  sudo apt-get install gpiod
 2123  sudo apt-get install libgpiod-dev


