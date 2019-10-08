#!/usr/bin/env python

# inspired from 
# https://websockets.readthedocs.io/en/stable/intro.html
# https://github.com/aaugustin/websockets/issues/39


import asyncio
import websockets
from datetime import datetime

async def handler(websocket, path):
    while True:
        if not websocket.open:
            break
        msg = await websocket.recv()
        #print(msg)
        if (msg.startswith("xy")):
            d = datetime.now().isoformat().replace("T", " ")
            # xy:-77|-261
            xy, values = msg.split(":")
            x, y = values.split("|")
            print(d+"\txy\t"+x+"\t"+y)

        else:
            print("unknown msg: "+msg)
    
def main():
    print("starting server on port 3000")
    server = websockets.serve(handler, "0.0.0.0", 3000)
    
    try:
        asyncio.get_event_loop().run_until_complete(server)
        asyncio.get_event_loop().run_forever()
    except KeyboardInterrupt:
        print('\nCtrl-C (SIGINT) caught. Exiting...')
    finally:
        #dont care
        #asyncio.get_event_loop().close()
        #server.close()
        x = 0 
        
if __name__ == "__main__":
    main()