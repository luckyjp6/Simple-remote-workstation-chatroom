#!/bin/bash
# Program:
#	Inputs user_name to creat a new_user and the corresponding home directory.

# add group
if ! grep -q '^worm_server:' /etc/group; then
    sudo groupadd worm_server
fi

read -p "User name: " user_name
sudo useradd -M -g worm_server ${user_name}
# sudo usermod -G worm_server ${user_name}
mkdir ./user_space/home/${user_name}
sudo chown -R ${user_name}:worm_server ./user_space/home/${user_name}
sudo chmod 700 ./user_space/home/${user_name}