from fastapi import APIRouter
from fastapi.responses import PlainTextResponse, JSONResponse
from fastapi.background import BackgroundTasks
from pydantic import BaseModel
from dependencies import *
import logger
import time
import pandas as pd

router = APIRouter()


class Id(BaseModel):
    id: str


@router.post("/register")
async def getTime(id: Id):
    logger.info(f"{id.id} is registering...")
    lastCountTime[id.id] = time.time()
    logger.success(f"{id.id} registered successfully.")
    return True


def appendCSV(data: pd.DataFrame, path: str, name: str):
    """
    `data` is the DataFrame\n
    `path` is the directory\n
    `name` is the file name
    """
    p = os.path.join(path, name)
    if not os.path.exists(path):
        os.makedirs(path)
    if os.path.exists(p):
        df1 = pd.read_csv(p)
        if not df1.empty:
            data = pd.concat([df1, data], ignore_index=True)
        else:
            print(p, "empty")
    data.to_csv(p, index=False)


class Raw(BaseModel):
    dt: float
    x: int
    y: int
    z: int


class Data(BaseModel):
    id: str
    data: list[Raw]


async def process(data: Data):
    buildData = {}
    wsData = {"dt": [], "x": [], "y": [], "z": []}
    last_t = ""
    logger.info(f"processing data from {data.id}...")
    for i in data.data:  # dTime: i (ms)
        if time.time() - lastCountTime[data.id] > 4294900 and i.dt < 2000:  # overflow
            lastCountTime[data.id] += 4294967.296
            print("pass")
        i.dt /= 1000
        i.dt += lastCountTime[data.id]
        tm = time.gmtime(i.dt)
        s = f"{tm.tm_year}.{tm.tm_mon}.{tm.tm_mday}.{tm.tm_hour}"
        lst: list = buildData.get(s, [])
        lst.append([i.dt, i.x, i.y, i.z])
        buildData[s] = lst
        t = time.localtime(i.dt)
        wsData["dt"].append(f"{t.tm_hour}:{t.tm_min}:{t.tm_sec}")
        wsData["x"].append(i.x * dataRatio)
        wsData["y"].append(i.y * dataRatio)
        wsData["z"].append(i.z * dataRatio)
    await manager.broadcast(wsData)
    for index, i in buildData.items():
        dta = pd.DataFrame(i, columns=["time", "x", "y", "z"])
        p = index.split(".")
        path = os.path.join("data", *p[:-1])
        name = p[-1]+".csv"
        appendCSV(dta, path, name)
    buildData.clear()
    logger.success(f"successfully processed data from {data.id}.")


@router.post("/data")
async def dataGateway(data: Data, background: BackgroundTasks):
    if data.id in lastCountTime:  # data.id in registerTime and
        logger.success(f"{data.id} is registered!")
        background.add_task(process, data)
        logger.info(f"Data length: {len(data.data)}.")
        return True
    logger.error(f"{data.id} is not registered, rejecting data.")
    return False
