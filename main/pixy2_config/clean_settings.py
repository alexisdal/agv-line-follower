# This Python file uses the following encoding: utf-8

import re

def cleanup(filename_in, filename_out):
    
    f = open(filename_in, "r")
    lines = f.readlines()
    f.close()
    vals = {}
    keys = []
    for line in lines:
        line = line.strip()
        if line.startswith("<data "):
            res = re.search('(key=\")(.*)(\")', line)
            if (res != None):
                key = res.group(2)
                keys.append(key)
                vals[key] = line
            else:
                print("ERROR")
    keys.sort()
    
    #for k in keys:
    #    print(k)
    
    f = open(filename_out, "w")
    f.write("<Pixy_parameters>\n")
    for k in keys:
        f.write(" " + vals[k] + "\n")
    f.write("</Pixy_parameters>\n")    
    f.close()
    


if __name__ == "__main__":
    # execute only if run as a script
    #names = ["parametres_pixy_prod_AGV1.prm","parametres_pixy_prod_AGV2.prm", "parametres_pixy_prod_AGV3.prm"]
    names = ["parametres_pixy_prod_AGV_DEV.prm"]
    for name in names:
        cleanup(name, name+".cleanup")


