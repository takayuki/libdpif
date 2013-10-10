#! /bin/sh

# sudo ip netns e ns1 ./hub -F2 -M -d dp0,addr=172.16.0.240/24,mac=02:00:00:00:01:00 -n veth1 -n veth2 -g tun0,src=10.0.0.1,dst=10.0.0.2 -v
sudo ip netns e ns1.2 ip route flush cache
sudo ip netns e ns1.2 ping 172.16.0.2 -s1472 -c2
sudo ip netns e ns1.2 ping 172.16.0.2 -s1473 -c2

# sudo ip netns e ns1 ./hub -F2 -M -d dp0,addr=172.16.0.240/24,mac=02:00:00:00:01:00 -n veth1 -n veth2 -g tun0,src=10.0.0.1,dst=10.0.0.2 -v
# sudo ip netns e ns2 ./hub -F2 -M -d dp0,addr=172.16.0.241/24,mac=02:00:00:00:01:01 -n veth1 -g tun0,src=10.0.0.2,dst=10.0.0.1 -v

sudo ip netns e ns1.2 ip route flush cache
sudo ip netns e ns1.2 ping 172.16.0.3 -s548 -c2
sudo ip netns e ns1.2 ping 172.16.0.3 -s549 -c2
sudo ip netns e ns1.2 ip route get 172.16.0.3

sudo ip netns e ns1.2 ip route flush cache
sudo ip netns e ns1.2 ping 172.16.0.3 -s1472 -c2
