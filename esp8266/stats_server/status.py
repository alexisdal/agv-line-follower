#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import cgi, pymysql

import os, sys # to display text about exceptions

import datetime  # for diff dates

print("Content-Type: text/html\r\n")

arguments = cgi.FieldStorage()





print("<html>")
print('<meta http-equiv="refresh" content="5"/>')
print("""<style>
th, td {
  border: 1px solid black;
  padding: 5px;
  font-size: xx-large;
}
</style>""")
print("<body>");


now = datetime.datetime.now()


try:

    db = pymysql.connect(host='localhost', user='agv', password='agv2019', database='agvdb', cursorclass=pymysql.cursors.DictCursor)
    c = db.cursor()
    
    print('<table style="border: 1px solid black;"><tr>')
    print('<th style="border:none" />')
    print("<th>Volt</th>")
    print("<th>km&nbsp;&nbsp;&nbsp;</th>")
    print("<th>fw&nbsp;&nbsp;&nbsp;</th>")
    print("<th>qs&nbsp;&nbsp;&nbsp;</th>")
    print("<th>sec&nbsp;&nbsp;&nbsp;</th>")
    print("</tr>");
    
    agv_names = [ "AGV1", "AGV2", "AGV3", "AGV_DEV"]
    for name in agv_names:
    
        sql = """
            select * from agv 
            where NAME='%s'
            order by `DATE` desc
            limit 1    
        """ %( name )
    
        #print(sql)
        res = c.execute(sql) # will return an int => number of rows
        #print(str(type(res)));
        #print(res);
        data = c.fetchone() 
        # typical result would be a tuple:
        # ('AGV1', 25.29, 975844, 0.21, 5387483, '6e756e6b776f0e9', '0.9.1.1', None, 107, 177.22, datetime.datetime(2019, 8, 31, 11, 2, 25))
        # with: cursorclass=pymysql.cursors.DictCursor 
        #{u'COUNT_BARCODE1': None, u'CURRENT_TICK': 5661548, u'NAME': 'AGV1', u'FW': '0.9.1.1', u'M': 377.66, u'KM': 107, u'TACHOMETER': 1035528, u'VOLTAGE': 25.26, u'DATE': datetime.datetime(2019, 8, 31, 11, 7, 4), u'SERIAL': '6e756e6b776f0e9', u'DUTYCYCLE': 0.0}
        #print(str(data))
        
        print("<tr>")
        if (res == 1):
            print("<td>%s</td>" % name )
            print("<td>%s</td>" % (data["VOLTAGE"]) )
            km = "%0.2f" % (float(data["KM"]) + (float(data["M"]) / 1000.0))
            print("<td>%s</td>" % ( km ) )
            print("<td>%s</td>" % (data["FW"]) )
            print("<td>%s</td>" % (data["queue_size"]) )
            duration = (now - data["DATE"]).total_seconds()
            print("<td>%s</td>" % ( str(int(duration))  ) )
            
        print("</tr>")
        
    print("</table>");




    db.commit()
    db.close()

except:
    exc_type, exc_obj, exc_tb = sys.exc_info()
    filename = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
    print("ERROR: "+str(exc_obj) + " [@line:" + str(exc_tb.tb_lineno) + "]")
    #pass


print("</body></html>");
