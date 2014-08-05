host_dir=`echo ~`
proc_name="sails"
file_name="/sails/dist/cron.log"
pid=0
proc_num(){
    num=`ps -ef | grep $proc_name | grep -v grep | wc -l`
    return $num
}
proc_id(){
    pid=`ps -ef | grep $proc_name | grep -v grep | awk '{print $2}'`
}
proc_num
number=$?
if [ $number -eq 0 ]
then
    echo "test" >> $host_dir$file_name
    cd $host_dir/sails/dist/; ./sails
    proc_id
    echo ${pid}, `date` >> $host_dir$file_name
fi
