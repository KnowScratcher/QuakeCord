from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from fastapi.middleware.gzip import GZipMiddleware
import uvicorn
from router import webpage
from router.api import data, ws
import sys
sys.path.insert(1, '/*')
sys.path.insert(1, '.')

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")
app.add_middleware(GZipMiddleware, minimum_size=500, compresslevel=5)
templates = Jinja2Templates(directory="templates")

app.include_router(data.router)
app.include_router(ws.router)
app.include_router(webpage.router)

if __name__ == "__main__":
    uvicorn.run("main:app",host="0.0.0.0", port=8001, reload=False)
