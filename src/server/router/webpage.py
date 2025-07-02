from fastapi import APIRouter
from fastapi.requests import Request
from fastapi.templating import Jinja2Templates
import json
import os

router = APIRouter()
templates = Jinja2Templates(directory="templates")

@router.get("/")
async def wavePage(request:Request):
    return templates.TemplateResponse(
        request=request, name="wave.html", context={})