# RDMA programming example

[![BSD license](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Transmit a file from client to server

## System required:

RDMA capable NIC, Linux kernel > 3.10, libverbs and librdmacm.

## How to use:

Git clone this repository at both client and server

    # git clone https://github.com/w180112/RDMA_DPDK.git

Set iptables to make UDP port 4791 open

On client side,

    # cd rdma_client
    # make
    # ./client -l 0-3 -n 2 <server ip address> <filename>
e.g.

    # ./client -l 0-3 -n 2 192.168.10.155 test.dat

On server side,

    # cd rdma_server
    # make
    # ./server -l 0-3 -n 2

The -l and -n options refer to DPDK EAL options

## Test environment

1. Mellanox Connectx-4 Lx with SRIOV enable
2. AMD R7-2700 + 64GB RAM

## To do

1. support multiple thread transmission
2. support complex DPDK EAL options
3. needs to add comments to source code
