from fastapi import FastAPI, Request, status
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from fastapi.middleware.gzip import GZipMiddleware
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
import logger # Using your existing logger
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

@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exc: RequestValidationError):
    # Get the raw body bytes
    body = await request.body()
    
    # Log the exact data that caused the 422 error
    logger.error(f"422 Validation Error!")
    logger.error(f"Path: {request.url.path}")
    logger.error(f"Raw Body: {body.decode()}")
    logger.error(f"Pydantic Errors: {exc.errors()}")

    return JSONResponse(
        status_code=status.HTTP_422_UNPROCESSABLE_ENTITY,
        content={
            "detail": exc.errors(), 
            "received_body": body.decode()
        },
    )

if __name__ == "__main__":
    uvicorn.run("main:app",host="0.0.0.0", port=8001, reload=False)
