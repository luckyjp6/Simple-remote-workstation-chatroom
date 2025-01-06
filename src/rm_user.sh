#!/bin/bash
# Program:
#	Inputs user_name to remove an exsisting user and remove the corresponding home directory.

read -p "User name: " user_name
sudo rm -r ./user_space/home/${user_name}
sudo userdel ${user_name}