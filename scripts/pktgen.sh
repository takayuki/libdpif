#! /bin/sh
echo 'rem_device_all' >/proc/net/pktgen/kpktgend_0
echo 'add_device eth0' >/proc/net/pktgen/kpktgend_0
echo 'min_pkt_size 1500' >/proc/net/pktgen/eth0
echo 'max_pkt_size 1500' >/proc/net/pktgen/eth0
echo 'src_min 172.16.0.1' >/proc/net/pktgen/eth0
echo 'src_max 172.16.0.1' >/proc/net/pktgen/eth0
echo 'dst_min 172.16.0.240' >/proc/net/pktgen/eth0
echo 'dst_max 172.16.0.240' >/proc/net/pktgen/eth0
echo 'dst_mac 02:00:00:00:00:01' >/proc/net/pktgen/eth0
echo 'count 0' >/proc/net/pktgen/eth0
echo 'ratep 500000' >/proc/net/pktgen/eth0
#echo start >/proc/net/pktgen/pgctrl
