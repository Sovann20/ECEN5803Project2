#!/bin/bash

# Authors: Michael Starks and Sovann Chak
# ECEN 5803 - 002
# Project 2 Module 3


# Displays some information on the
# current time/date and relevant
# HW and SW information
console_display() {
        current_time_date=$(date +%T)
        current_time_date+=" "
        current_time_date+=$(date +%D)
        os_version=$(cat /etc/os-release | sed -n 's/^PRETTY_NAME="\(.*\)"/\1/p')
        kernel_version=$(uname -r)
        ipv4_address=$(ip -f inet addr show $(ip addr | awk '/state UP/ {print $2}' | sed 's/.$//') | grep -Po 'inet \K[\d.]+')
        default_gateway=$(ip -f inet addr show $(ip addr | awk '/state UP/ {print $2}' | sed 's/.$//') | grep -Po 'brd \K[\d.]+')
        mac_address=$(ifconfig  | grep -A1 eth0 | grep ether | awk '{print $2}')
        echo "The current time and date is: $current_time_date"
        echo "OS Verions: $os_version"
        echo "Kernel Version: $kernel_version"
        echo "IPV4 Address: $ipv4_address"
        echo "MAC Address: $mac_address"
        echo "Default Gateway: $default_gateway"
        echo ""
}

# Updates the system and accumulates the counts
# to display a total count
update(){

        echo "Updating packages..."
        # silence output from update, I don't want it
        sudo apt-get update > /dev/null 2>&1
        upgrades=$(sudo apt-get upgrade -y)

        echo "Cleaning unnecessary packages..."
        # silence output from clean, I don't want it
        sudo apt-get autoclean > /dev/null 2>&1
        removes=$(sudo apt-get autoremove -y)


        # Collect the counts from each so that
        # we an add them up as a single print
        # message
        upgrade_counts=$(echo $upgrades | sed 's/[^0-9]\+/ /g')
        remove_counts=$(echo $removes | sed 's/[^0-9]\+/ /g')

        upgrade_count0=$(echo $upgrade_counts | awk '{print $1}')
        upgrade_count1=$(echo $upgrade_counts | awk '{print $2}')
        upgrade_count2=$(echo $upgrade_counts | awk '{print $3}')
        upgrade_count3=$(echo $upgrade_counts | awk '{print $4}')

        remove_count0=$(echo $remove_counts | awk '{print $1}')
        remove_count1=$(echo $remove_counts | awk '{print $2}')
        remove_count2=$(echo $remove_counts | awk '{print $3}')
        remove_count3=$(echo $remove_counts | awk '{print $4}')

        count0=$((upgrade_count0 + remove_count0))
        echo "Upgraded: $((upgrade_count1 + remove_count1))"
        echo "Newly Installed: $((upgrade_count2 + remove_count2))"
        echo "To Remove: $((upgrade_count2 + remove_count2))"
        echo "Not Upgraded: $((upgrade_count3 + remove_count3))"
        echo ""
}

# Monitors the WiFi status and
# if the WiFi is connected to a
# network then the green LED will
# stay lit
monitor_wifi(){
#!/bin/bash

echo "Monitoring WiFi using the green LED..."

# Assuming these are static
INTERFACE="wlan0"
STATE_FILE="/tmp/wifi_state"

state_string="off"
# Initializes the LED and STATE_FILE by
# checking if the interface is up via ip command and
# by using grep, if grep returns something then we
# know the WiFi interface is up and can enable the green LED
ip link show wlan0 | grep 'state UP' > /dev/null && (echo "up" > /tmp/wifi_state && sudo echo default-on > /sys/class/leds/led0/trigger && echo "WiFi is on") \
                                                 || (echo "down" > /tmp/wifi_state && sudo echo none > /sys/class/leds/led0/trigger && echo "WiFi is off")

# Start the monitoring loop
while true; do

        ip link show $INTERFACE | grep 'state UP' > /dev/null
        if [ $? -eq 0 ]; then
                curr_state=1
                trigger=default-on
		state=on
        else
                curr_state=0
                trigger=none
		state=off
        fi

        # Read previous state from state file
        prev_state=$(cat $STATE_FILE)

        # Compare the current state with the previous state
        if [ "$curr_state" != "$prev_state" ]; then
                echo $curr_state > $STATE_FILE
                echo $trigger > /sys/class/leds/led0/trigger
		echo "WiFi is currently $state"
        fi
        # Wait, don't want to each too much processing
        sleep 1
done
}

# Raspbian Linux is greedy when it comes
# to control, so we must usurp control
chmod 666 /sys/class/leds/led0/trigger

# Calling the functions
console_display
update
monitor_wifi

# end script
