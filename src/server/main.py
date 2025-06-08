from fastapi import FastAPI, WebSocket,WebSocketDisconnect,BackgroundTasks
from fastapi.responses import HTMLResponse, PlainTextResponse
from pydantic import BaseModel
import json
import time
import os
import pandas as pd

app = FastAPI()

registerTime = {}
lastCountTime = {}
buildData = {}

class ConnectionManager:
    def __init__(self):
        self.activate:list[WebSocket] = []
    async def connect(self,ws:WebSocket):
        await ws.accept()
        self.activate.append(ws)
    def disconnect(self,ws):
        self.activate.remove(ws)

    async def broadcast(self,msg:str):
        for connection in self.activate:
            await connection.send_text(msg)

manager = ConnectionManager()

class Id(BaseModel):
    id:str

@app.post("/register")
async def getTime(id:Id):
    registerTime[id.id] = time.time()
    lastCountTime[id.id] = time.time()
    return True

def appendCSV(data:pd.DataFrame,path:str,name:str):
    """
    `data` is the DataFrame\n
    `path` is the directory\n
    `name` is the file name
    """
    p = os.path.join(path,name)
    if not os.path.exists(path):
        os.makedirs(path)   
    if os.path.exists(p):
        df1 = pd.read_csv(p)
        if not df1.empty:
            data = pd.concat([df1,data], ignore_index=True)
        else:
            print(p,"empty")
    data.to_csv(p,index=False)

class Raw(BaseModel):
    dt:float
    x:int
    y:int
    z:int
    
class Data(BaseModel):
    id:str
    data:list[Raw]

def process(data:Data):
    global buildData
    print(lastCountTime[data.id])
    for i in data.data:
        if time.time() - lastCountTime[data.id] > 4294900 and i.dt < 2000: # overflow
            lastCountTime[data.id] += 4294967.296
            print("pass")
        i.dt /= 1000
        i.dt += lastCountTime[data.id]
        tm = time.gmtime(i.dt)
        s = f"{tm.tm_year}.{tm.tm_mon}.{tm.tm_mday}.{tm.tm_hour}"
        lst:list = buildData.get(s,[])
        lst.append([i.dt,i.x,i.y,i.z])
        buildData[s] = lst
    for index,i in buildData.items():
        dta = pd.DataFrame(i,columns=["time","x","y","z"])
        p = index.split(".")
        path = os.path.join("data",*p[:-1])
        name = p[-1]+".csv"
        appendCSV(dta,path,name)
    buildData = {}
    
@app.post("/data")
async def websocket_endpoint(data:Data,background:BackgroundTasks):
    if data.id in registerTime and data.id in lastCountTime:
        background.add_task(process,data)
        print(len(data.data))
        return True
    return False
