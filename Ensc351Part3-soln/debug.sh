echo $7
cd $7
echo PWD is now $PWD
#printenv
taskset -c 0 chrt 98 gdbserver $@
