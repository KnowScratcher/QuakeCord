from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from fastapi.background import BackgroundTasks
from pydantic import BaseModel
from dependencies import *
import logger
import time
import pandas as pd
import json
import asyncio
import numpy as np
from anyio import to_thread

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

async def process_numpy(data: dict):
    station_id = data["id"]
    # 1. Convert to NumPy Array (Fast)
    raw_data = np.array([[i["dt"], i["x"], i["y"], i["z"]] for i in data["data"]])
    relative_ms = raw_data[:, 0]

    # clock check and overflow adjustment
    diffs = np.diff(relative_ms, prepend=relative_ms[0])
    overflow_indices = np.where(diffs < -4000000)[0] # big drop
    if len(overflow_indices) > 0:
        for idx in overflow_indices:
            relative_ms[idx:] += 4294967.296
    anchor = lastCountTime[station_id]
    timestamps = anchor + (relative_ms / 1000.0)
    if (time.time() - anchor) > 4294900 and relative_ms[0] < 2000:
        lastCountTime[station_id] += 4294967.296
        logger.warning(f"Overflow detected for {station_id}, adjusting anchor.")
    
    # 3. Determine "Hour Keys" for every sample
    # This creates an array of strings like "2026.1.19.19"
    hour_keys = np.array([time.strftime("%Y.%m.%d.%H", time.gmtime(t)) for t in timestamps])
    
    # 4. Find unique hours in this batch
    unique_hours = np.unique(hour_keys)
    
    for hour in unique_hours:
        # Mask the data that belongs to this specific hour
        mask = (hour_keys == hour)
        hour_data = raw_data[mask]
        hour_timestamps = timestamps[mask]
        
        # Combine timestamps back with x, y, z
        # Result: [timestamp, x, y, z]
        final_table = np.column_stack((hour_timestamps, hour_data[:, 1:]))
        
        # Convert to DataFrame and append to CSV
        df = pd.DataFrame(final_table, columns=["time", "x", "y", "z"])
        
        await manager.broadcast({
            "dt": [time.strftime("%H:%M:%S", time.localtime(t)) for t in hour_timestamps],
            "x": (hour_data[:, 1] * dataRatio).tolist(),
            "y": (hour_data[:, 2] * dataRatio).tolist(),
            "z": (hour_data[:, 3] * dataRatio).tolist()
        })
        logger.success(f"successfully processed data from {data['id']}.")
        p = hour.split(".")
        path = os.path.join("data", station_id, *p[:-1])
        name = f"{p[-1]}.csv"
        await to_thread.run_sync(appendCSV, df, path, name)


# async def process(data: Data):
#     buildData = {}
#     wsData = {"dt": [], "x": [], "y": [], "z": []}
#     last_t = ""
#     logger.info(f"processing data from {data.id}...")
#     for i in data.data:  # dTime: i (ms)
#         if time.time() - lastCountTime[data.id] > 4294900 and i.dt < 2000:  # overflow
#             lastCountTime[data.id] += 4294967.296
#             print("pass")
#         i.dt /= 1000
#         i.dt += lastCountTime[data.id]
#         tm = time.gmtime(i.dt)
#         s = f"{tm.tm_year}.{tm.tm_mon}.{tm.tm_mday}.{tm.tm_hour}"
#         lst: list = buildData.get(s, [])
#         lst.append([i.dt, i.x, i.y, i.z])
#         buildData[s] = lst
#         t = time.localtime(i.dt)
#         wsData["dt"].append(f"{t.tm_hour}:{t.tm_min}:{t.tm_sec}")
#         wsData["x"].append(i.x * dataRatio)
#         wsData["y"].append(i.y * dataRatio)
#         wsData["z"].append(i.z * dataRatio)
#     await manager.broadcast(wsData)
#     for index, i in buildData.items():
#         dta = pd.DataFrame(i, columns=["time", "x", "y", "z"])
#         p = index.split(".")
#         path = os.path.join("data", *p[:-1])
#         name = p[-1]+".csv"
#         appendCSV(dta, path, name)
#     buildData.clear()
#     logger.success(f"successfully processed data from {data.id}.")


@router.websocket("/ws/data/{client_id}")
async def websocket_data_endpoint(websocket: WebSocket, client_id: str):
    await websocket.accept()
    logger.info(f"ESP32 {client_id} connected via WebSocket")
    
    if client_id not in lastCountTime:
        lastCountTime[client_id] = time.time()
        logger.info(f"{client_id} auto-registered via WS")

    try:
        while True:
            # 1. Receive JSON data from ESP32
            data_json = await websocket.receive_text()
            raw_payload = json.loads(data_json)
            
            # 2. Map to your Data model
            # Note: ESP32 sends 'data' array and 'id'
            data_obj = Data(**raw_payload)
            
            # 3. Handle processing
            # Since WebSockets are async, we can await process directly 
            # or wrap it in a task to avoid blocking the socket heartbeats
            asyncio.create_task(process(data_obj))
            
            # 4. Optional: Send acknowledgment back to ESP32
            await websocket.send_text("ack")

    except WebSocketDisconnect:
        logger.warning(f"ESP32 {client_id} disconnected")
    except Exception as e:
        logger.error(f"WS Error: {e}")