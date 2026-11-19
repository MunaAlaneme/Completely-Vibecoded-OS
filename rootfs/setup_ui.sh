#!/bin/sh
apk update
apk add icewm xorg-server xf86-video-fbdev ttf-dejavu

# Configure IceWM Windows 95 look
echo 'Theme="win95"' > /root/.icewm/theme
echo "icewm-session" > /root/.xinitrc
chmod +x /root/.xinitrc
