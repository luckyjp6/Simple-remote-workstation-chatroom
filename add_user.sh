#!/bin/bash
# Program:
#	Inputs user_name to creat a new_user and the corresponding home directory.

read -p "User name: " user_name
sudo useradd -M ${user_name}
mkdir ./user_space/home/${user_name}
sudo chown -R ${user_name}:${user_name} ./user_space/home/${user_name}
sudo chmod 700 ./user_space/home/${user_name}