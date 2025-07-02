import os
import logger
from fastapi.websockets import WebSocket
from worker import history

basePath = os.path.dirname(__file__)
""" The base dir """
dataPath = os.path.join(basePath, "data")
""" The data directory """
dataRatio = 980.0 / 8028.6
""" The experiment data for data -> gal """

lastCountTime: dict[str, float] = {}
""" Record of the last count time of each station, this value will change about every 49 days """

class ConnectionManager:
    """ Websocket control"""
    def __init__(self):
        self.activate:list[WebSocket] = []
    async def connect(self, ws:WebSocket):
        await ws.accept()
        self.activate.append(ws)
        logger.info("client connected")
        await ws.send_json(history.getHistory(dataRatio))
    async def disconnect(self, ws):
        self.activate.remove(ws)
        logger.info("client disconnected")

    async def broadcast(self, msg:dict):
        for connection in self.activate:
            await connection.send_json(msg)

manager = ConnectionManager()
""" Websocket manager"""