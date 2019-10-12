#set -x
while true; do
        RSSI=$(grep wlan0 /proc/net/wireless | awk '{print $4}' | tr -d '.')
        CHAN=$(iwlist wlan0 channel | grep Current | rev | tr -d ')' | awk '{print $1}' | rev)
        URL="http://10.155.249.179/cgi-bin/insert.py?n=WIFI_PI&fw=0.1.1&rssi=${RSSI}&channel=${CHAN}"
        wget -qO /dev/null "$URL"
        sleep 30s
done
