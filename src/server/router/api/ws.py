from fastapi import WebSocket
from fastapi.routing import APIRouter
import logger
from dependencies import *
from worker import history


router = APIRouter()

@router.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    while True:
        try:
            await websocket.receive_text()
        except:
            await manager.disconnect(websocket)
            break