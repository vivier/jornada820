#!/bin/sh
# See http://udhcp.busybox.net/README.udhcpc
# Invoked by: udhcpc -H mildendo -n -f -q -s /etc/udhcpc.sh -i eth0

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

case "$action" in
  bound|renew)
    ifconfig $interface $ip netmask $subnet
    route add default gw $router
    echo "nameserver $namesvr" > /etc/resolv.conf
    hostname $hostname
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
