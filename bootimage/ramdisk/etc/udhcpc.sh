#!/bin/sh
# See http://udhcp.busybox.net/README.udhcpc
# Invoked by: udhcpc -H mildendo -n -f -q -s /etc/udhcpc.sh -i eth0
# $Id: udhcpc.sh,v 1.3 2004/05/08 12:39:23 fare Exp $

action=$1

print_vars () {
  pvspace=
  for i ; do
    eval "echo -n \"$pvspace$i=\$$i\""
    pvspace=" "
  done
  echo
}
print_all_vars () {
  print_vars \
	action interface ip siaddr sname \
	boot subnet timezone router timesvr namesvr dns logsvr \
	cookiesvr lprsvr hostname bootsize domain swapsvr \
	rootpath ipttl mtu broadcast ntpsrv wins lease dhcptype \
	serverid message tftp bootfile
}
print_few_vars () {
  print_vars action interface
}

make_resolv_conf () {
  if [ -n "$domain" ] ; then
    echo "search $domain"
  fi
  for i in $dns ; do
    echo "nameserver $i"
  done
}
try_rdate () {
  if [ -n "$timesvr" ] ; then
    rdate -s $timesvr
  fi
}

case "$action" in
  bound|renew)
    ifconfig $interface $ip netmask $subnet
    route add default gw $router
    make_resolv_conf > /etc/resolv.conf
    hostname $hostname
    try_rdate
    print_all_vars
    ;;
  nak)
    print_few_vars
    ;;
  deconfig|nak|*)
    ifconfig $interface 0.0.0.0 up
    print_few_vars
    ;;
esac
