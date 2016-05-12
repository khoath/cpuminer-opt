#
# Dockerfile for cpuminer
# usage: docker run hmage/cpuminer-opt --url xxxx --user xxxx --pass xxxx
# ex: docker run hmage/cpuminer-opt -a lyra2 -o stratum+tcp://lyra2re.eu.nicehash.com:3342 -O 1HMageKbRBu12FkkFbMEcskAtH59TVrS2G.${HOSTNAME//-/}:x
#

FROM		debian:jessie
MAINTAINER	Eugene Bujak <hmage@hmage.net>

RUN		echo 'APT::Install-Recommends "false";' > /etc/apt/apt.conf.d/zz-local-tame
RUN		apt-get update
RUN		apt-get upgrade -y
RUN		apt-get install -y git ca-certificates                                       # for cloning from git
RUN		apt-get install -y build-essential autoconf automake                         # compiler and tools
RUN		apt-get install -y libssl-dev libcurl4-openssl-dev libjansson-dev libgmp-dev # library dependencies

RUN		git clone https://github.com/hmage/cpuminer-opt

WORKDIR		/cpuminer-opt

RUN		autoreconf -f -i -v
RUN		CFLAGS="-O3 -maes -mssse3 -mtune=intel -DUSE_ASM" CXXFLAGS="$CFLAGS -std=gnu++11" ./configure --with-crypto --with-curl
RUN		make -j8

ENTRYPOINT	["./cpuminer"]
