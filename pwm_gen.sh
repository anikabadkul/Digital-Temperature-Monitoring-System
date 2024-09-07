#!/bin/sh
config-pin -q P9_16
config-pin P9_16 pwm
cd /sys/class/pwm/pwmchip4/pwm-4\:1
echo 100000000 > period
echo 50000000 > duty_cycle
echo 1 > enable
echo 'PWM Configured & Started'

config-pin -q P8_08
config-pin P8_08 gpio
cd /sys/class/gpio/gpio67
echo in > direction
echo "rising" > /sys/class/gpio/gpio67/edge
cat direction
echo 'GPIO Configured as Input with Rising Edge Interrupt'