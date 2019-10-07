#!/usr/bin/env python3

import cgi, cgitb, pymysql

import os, sys # to display text about exceptions

print("Content-Type: text/plain\r\n")

arguments = cgi.FieldStorage()



try:

    db = pymysql.connect(host='localhost', user='agv', password='agv2019', database='agvdb', cursorclass=pymysql.cursors.DictCursor)
    c = db.cursor()
    
    # /cgi-bin/insert_agv_test.py?NAME=AGV_DEV&VOLTAGE=23.51&TACHOMETER=155496&DUTYCYCLE=0.25&CURRENT_TICK=822647&SERIAL=6e756e6b776f0ed&FW=0.9.1 HTTP/1.1
    #      + String("NAME=") + String(AGV_NAME)
    #      + "&VOLTAGE=" + String(average_voltage) 
    #      + "&TACHOMETER=" + String(measuredValLeft.tachometer) 
    #      + "&DUTYCYCLE=" + String(average_duty) 
    #      + "&CURRENT_TICK=" + String(current_tick) 
    #      + "&SERIAL=" + arduino_serial 
    #      + "&FW=" + String(VERSION) 
    #      + "&KM=" + String(mydataInEEPROM.km) 
    #      + "&M=" + String(mydataInEEPROM.meters) 

    # defaults values
    #NAME           = None
    #VOLTAGE        = 0.0
    #TACHOMETER     = 0
    #DUTYCYCLE      = 0
    #CURRENT_TICK   = 0
    #SERIAL         = None
    #FW             = None
    #KM             = 0
    #M              = 0
    #num_bumps      = 0
    #num_lidar_crit = 0
    #num_lidar_err  = 0
    #num_line_lost  = 0
    #num_ssid       = 0
    #dur_scan       = 0
    #rssi           = 0
    #channel        = 0
    #queue_size     = 0
    
        
    #NAME           = arguments["NAME"].value            if "NAME"           in arguments.keys() else NAME
    #NAME           = arguments["n"].value               if "n"              in arguments.keys() else NAME
    #VOLTAGE        = arguments["VOLTAGE"].value         if "VOLTAGE"        in arguments.keys() else VOLTAGE
    #VOLTAGE        = arguments["v"].value               if "v"              in arguments.keys() else VOLTAGE
    #TACHOMETER     = arguments["TACHOMETER"].value      if "TACHOMETER"     in arguments.keys() else TACHOMETER
    #TACHOMETER     = arguments["tc"].value              if "tc"             in arguments.keys() else TACHOMETER
    #DUTYCYCLE      = arguments["DUTYCYCLE"].value       if "DUTYCYCLE"      in arguments.keys() else DUTYCYCLE
    #DUTYCYCLE      = arguments["dc"].value              if "dc"             in arguments.keys() else DUTYCYCLE
    #CURRENT_TICK   = arguments["CURRENT_TICK"].value    if "CURRENT_TICK"   in arguments.keys() else CURRENT_TICK
    #CURRENT_TICK   = arguments["ct"].value              if "ct"             in arguments.keys() else CURRENT_TICK
    #SERIAL         = arguments["SERIAL"].value          if "SERIAL"         in arguments.keys() else SERIAL
    #SERIAL         = arguments["sl"].value              if "sl"             in arguments.keys() else SERIAL
    #FW             = arguments["FW"].value              if "FW"             in arguments.keys() else FW
    #FW             = arguments["fw"].value              if "fw"             in arguments.keys() else FW
    #KM             = arguments["KM"].value              if "KM"             in arguments.keys() else KM
    #KM             = arguments["km"].value              if "km"             in arguments.keys() else KM
    #M              = arguments["M"].value               if "M"              in arguments.keys() else M
    #M              = arguments["m"].value               if "m"              in arguments.keys() else M
    #num_bumps      = arguments["num_bumps"].value       if "num_bumps"      in arguments.keys() else num_bumps
    #num_bumps      = arguments["nb"].value              if "nb"             in arguments.keys() else num_bumps
    #num_lidar_crit = arguments["num_lidar_crit"].value  if "num_lidar_crit" in arguments.keys() else num_lidar_crit
    #num_lidar_crit = arguments["nc"].value              if "nc"             in arguments.keys() else num_lidar_crit
    #num_lidar_err  = arguments["num_lidar_err"].value   if "num_lidar_err"  in arguments.keys() else num_lidar_err
    #num_lidar_err  = arguments["ne"].value              if "ne"             in arguments.keys() else num_lidar_err
    #num_line_lost  = arguments["num_line_lost"].value   if "num_line_lost"  in arguments.keys() else num_line_lost
    #num_line_lost  = arguments["ll"].value              if "ll"             in arguments.keys() else num_line_lost
    #
    ## wifi scan monitoring
    ## http://10.155.100.89/cgi-bin/insert.py?NAME=WIFI_MON&fw0.1&num_ssid=7&dur_scan=2189&rssi=-77&channel=6
    #num_ssid       = arguments["num_ssid"].value        if "num_ssid"       in arguments.keys() else 0
    #dur_scan       = arguments["dur_scan"].value        if "dur_scan"       in arguments.keys() else 0
    #rssi           = arguments["rssi"].value            if "rssi"           in arguments.keys() else 0
    #channel        = arguments["channel"].value         if "channel"        in arguments.keys() else 0
    #queue_size     = arguments["queue_size"].value      if "queue_size"     in arguments.keys() else 0

    NAME           = arguments.getvalue("n")
    VOLTAGE        = arguments.getvalue("v")
    TACHOMETER     = arguments.getvalue("tc")
    DUTYCYCLE      = arguments.getvalue("dc")
    CURRENT_TICK   = arguments.getvalue("tc")
    SERIAL         = arguments.getvalue("sl")
    FW             = arguments.getvalue("fw")
    KM             = arguments.getvalue("km")
    M              = arguments.getvalue("m")
    num_bumps      = arguments.getvalue("nb")
    num_lidar_crit = arguments.getvalue("nc")
    num_lidar_err  = arguments.getvalue("ne")
    num_line_lost  = arguments.getvalue("ll")
    num_ssid       = arguments.getvalue("num_ssid")
    dur_scan       = arguments.getvalue("dur_scan")
    rssi           = arguments.getvalue("rssi")
    channel        = arguments.getvalue("channel")
    queue_size     = arguments.getvalue("queue_size")

    # populate default values if missing in provided parameters
    NAME           =  ""    if (NAME           is None) else NAME
    VOLTAGE        =  0     if (VOLTAGE        is None) else VOLTAGE
    TACHOMETER     =  0     if (TACHOMETER     is None) else TACHOMETER
    DUTYCYCLE      =  0     if (DUTYCYCLE      is None) else DUTYCYCLE
    CURRENT_TICK   =  0     if (CURRENT_TICK   is None) else CURRENT_TICK
    SERIAL         =  ""    if (SERIAL         is None) else SERIAL
    FW             =  ""    if (FW             is None) else FW
    KM             =  0     if (KM             is None) else KM
    M              =  0     if (M              is None) else M
    num_bumps      =  0     if (num_bumps      is None) else num_bumps
    num_lidar_crit =  0     if (num_lidar_crit is None) else num_lidar_crit
    num_lidar_err  =  0     if (num_lidar_err  is None) else num_lidar_err
    num_line_lost  =  0     if (num_line_lost  is None) else num_line_lost
    num_ssid       =  0     if (num_ssid       is None) else num_ssid
    dur_scan       =  0     if (dur_scan       is None) else dur_scan
    rssi           =  0     if (rssi           is None) else rssi
    channel        =  0     if (channel        is None) else channel
    queue_size     =  0     if (queue_size     is None) else queue_size


    
    
    #print("NAME           : "+str(NAME          ))# = arguments.getvalue("n")
    #print("VOLTAGE        : "+str(VOLTAGE       ))# = arguments.getvalue("v")
    #print("TACHOMETER     : "+str(TACHOMETER    ))# = arguments.getvalue("tc")
    #print("DUTYCYCLE      : "+str(DUTYCYCLE     ))# = arguments.getvalue("dc")
    #print("CURRENT_TICK   : "+str(CURRENT_TICK  ))# = arguments.getvalue("tc")
    #print("SERIAL         : "+str(SERIAL        ))# = arguments.getvalue("sl")
    #print("FW             : "+str(FW            ))# = arguments.getvalue("fw")
    #print("KM             : "+str(KM            ))# = arguments.getvalue("km")
    #print("M              : "+str(M             ))# = arguments.getvalue("m")
    #print("num_bumps      : "+str(num_bumps     ))# = arguments.getvalue("nb")
    #print("num_lidar_crit : "+str(num_lidar_crit))# = arguments.getvalue("nc")
    #print("num_lidar_err  : "+str(num_lidar_err ))# = arguments.getvalue("ne")
    #print("num_line_lost  : "+str(num_line_lost ))# = arguments.getvalue("ll")
    #print("num_ssid       : "+str(num_ssid      ))# = arguments.getvalue("num_ssid")
    #print("dur_scan       : "+str(dur_scan      ))# = arguments.getvalue("dur_scan")
    #print("rssi           : "+str(rssi          ))# = arguments.getvalue("rssi")
    #print("channel        : "+str(channel       ))# = arguments.getvalue("channel")
    #print("queue_size     : "+str(queue_size    ))# = arguments.getvalue("queue_size")
    
    
    exec_table = """
          INSERT INTO agv(
          NAME, VOLTAGE, TACHOMETER, DUTYCYCLE, CURRENT_TICK, SERIAL, FW  , KM, M  , num_bumps, num_lidar_crit, num_lidar_err, num_line_lost, num_ssid, dur_scan , rssi , channel, queue_size  ) Values(
          '%s', %s     , %s        , %s       , %s          , '%s'  , '%s', %s, %s , %s       , %s            , %s           , %s           , %s      , %s       , %s   , %s     , %s      );
    """ %(NAME, VOLTAGE, TACHOMETER, DUTYCYCLE, CURRENT_TICK, SERIAL, FW  , KM, M  , num_bumps, num_lidar_crit, num_lidar_err, num_line_lost, num_ssid, dur_scan , rssi , channel, queue_size  ) 

    #print(exec_table)
    c.execute(exec_table)

    db.commit()
    db.close()

    print("OK")
except:
    exc_type, exc_obj, exc_tb = sys.exc_info()
    filename = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
    print("ERROR: "+str(exc_obj) + " [@line:" + str(exc_tb.tb_lineno) + "]")
    pass
