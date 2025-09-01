#! /bin/bash
# DEBIAN_FRONTEND=noninteractive  
# apt-get -y install sudo # 如果没有 sudo 的话，得在 root 权限下 安装一下
sudo apt-get -y update
sudo apt-get -y install wget
# 清华源 如果需要可以打开
# wget https://gitee.com/lin-xi-269/tools/raw/master/os/QHubuntu20.04 && bash QHubuntu20.04
sudo apt-get install build-essential -y
sudo apt-get install gcc-multilib -y
sudo apt-get -y install git

mkdir csapplab
cd csapplab
labs=( "datalab" "bomblab" "attacklab" "archlab" "cachelab" "shlab" "malloclab" "proxylab" )
i=1
for dir in ${labs[*]} ;do
    lab="lab"$i"$dir"
    wget https://gitee.com/lin-xi-269/csapplab/raw/origin/$lab/install.sh -O install$lab.sh&& bash install$lab.sh && rm install$lab.sh
    i=$((i+1))
done
# lab8 需要
sudo sudo apt-get install python2 -y
sudo ln /usr/bin/python2 /usr/bin/python
# lab4 方便测试
cd csapplab/archlab/archlab-handout/
tar -xvf sim.tar
cd sim/pipe/
wget https://gitee.com/lin-xi-269/csapplab/raw/master/lab4archlab/archlab-handout/sim/pipe/run.sh
chmod +x run.sh

# lab4 环境配置
wget https://gitee.com/lin-xi-269/csapplab/raw/master/lab4archlab/archlab-handout/installTclTk.sh && bash installTclTk.sh

# lab8 proxy
sudo apt-get install net-tools
# lab2 如果 需要 cgdb 工具 的话，可以打开注释
# wget https://gitee.com/lin-xi-269/csapplab/raw/master/lab2bomb/installCgdb.sh
# bash installCgdb.sh

