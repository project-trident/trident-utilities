#!/bin/bash
########
# Simple script to create a user's home directory as a ZFS dataset with user ownership of it
########
if [ ! -x "/usr/bin/zpool" ] || [ ! -x "/usr/bin/zfs" ] ; then
  #Could not find ZFS utilities - cannot run script
  exit 1
fi

findZpool(){
  #This sets the "_zpool" variable, and verifies the input "zpool" variable
  if [ -n "${_zpool}" ] ; then
    #Got an input zpool - see if this one is valid
    if [ 0 -eq `zpool list | grep -q -E "^${_zpool} "` ] ; then
      #Good pool name - go ahead and use it
      return
    fi
  fi
  _zpool=`zfs list | grep -E ".* ${homedir}\$" | cut -d / -f 1`
  if [ -z "${_zpool}" ] ; then
    #Could not find parent dataset, just grab the first zpool instead (in case a UFS root filesystem?)
    _zpool=`zpool list -H | grep ONLINE | cut -f 1`
  fi
}

createUser(){
  #Create the dataset
  
  zfs create -o "mountpoint=${homedir}/${user}" -o setuid=off -o compression=on -o atime=off -o canmount=on "${_zpool}/${homedir}/${user}"

  # Create the user
  useradd -M -s "${usershell}" -d "${homedir}/${user}" -c"${usercomment}" -G "wheel,users,audio,video,input,cdrom,bluetooth" "${user}"
  echo "${user}:${userpass}" |  chpasswd -c SHA512

  # Setup ownership of the dataset
  # Allow the user to create/destroy child datasets and snapshots on their home dir
  zfs allow "${user}" mount,create,destroy,rollback,snapshot "${_zpool}/${homedir}/${user}"

}

#Check inputs
userfile=$1
_zpool=$2
homedir="/home"

if [ -z "$1" ] || [ ! -e "${userfile}" ] ; then
  echo "Usage: $0 <users.json file> [zpool=autodetect]] "
  echo ' users.json format:
This file is an array of JSON objects, with one user account per object. This file will get deleted as soon as the script finishes.

Example:
[
 { "name" : "username", "comment" : "Visible Name", "shell" : "/bin/bash", "password" : "mypassword" }
]

'
  exit 1
fi

#Now verify/find the current zpool
findZpool
if [ -z "${_zpool}" ] ; then
  # Could not find a valid zpool to create the dataset on
  exit 1
fi

numusers=`jq -r '. | length' ${userfile}`
num=0
while [ ${num} -lt ${numusers} ]
do
  user=`jq -r '.['${num}'].name' "${userfile}"`
  usercomment=`jq -r '.['${num}'].comment' "${userfile}"`
  usershell=`jq -r '.['${num}'].shell' "${userfile}"`
  userpass=`jq -r '.['${num}'].password' "${userfile}"`
  createUser
  echo "User Created: ${user}"
  num=`expr ${num} + 1`
done
