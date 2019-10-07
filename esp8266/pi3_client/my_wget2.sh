#set -x
while true; do
    # rpush to add / lpop to retreive
    PARAMS=$(redis-cli lpop mylist)
    if [ -z "$PARAMS" ]
    then
        # PARAMS is empty, nothing to do
        sleep 3s
    else
    
        RSSI=$(grep wlan0 /proc/net/wireless | awk '{print $4}' | tr -d '.')
        CHAN=$(iwlist wlan0 channel | grep Current | rev | tr -d ')' | awk '{print $1}' | rev)
        #URL="http://10.155.249.179/cgi-bin/insert.py?n=WIFI_PI&fw=0.1.1&rssi=${RSSI}&channel=${CHAN}"
        URL="http://10.155.249.179/cgi-bin/insert.py${PARAMS}&rssi=${RSSI}&channel=${CHAN}"
        
        wget -qO /dev/null "$URL"
        #sleep 30s
    fi
done
