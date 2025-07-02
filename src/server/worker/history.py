import pandas as pd
import time
import os

def getHistory(dataRatio):
    sendData = {"dt":[], "x":[], "y":[], "z":[]}
    tm_epoch = time.time()
    tm = time.gmtime(tm_epoch)
    p = [str(tm.tm_year), str(tm.tm_mon), str(tm.tm_mday), str(tm.tm_hour) + ".csv"]
    left = 0
    if os.path.exists(os.path.join("data", *p)):
        data = pd.read_csv(os.path.join("data", *p))
        left = 1200 - len(data)
        if left > 0:
            tm = time.gmtime(tm_epoch - 3600)
            p = [str(tm.tm_year), str(tm.tm_mon), str(tm.tm_mday), str(tm.tm_hour) + ".csv"] 
            if os.path.exists(os.path.join("data", *p)):
                data_ = pd.read_csv(os.path.join("data", *p))
                sendData["dt"].extend(data_.iloc[len(data_) - left - 1 if len(data_) - left - 1 >= 0 else 0: len(data_) - 1]["time"])
                sendData["x"].extend(data_.iloc[len(data_) - left - 1 if len(data_) - left - 1 >= 0 else 0: len(data_) - 1]["x"] * dataRatio)
                sendData["y"].extend(data_.iloc[len(data_) - left - 1 if len(data_) - left - 1 >= 0 else 0: len(data_) - 1]["y"] * dataRatio)
                sendData["z"].extend(data_.iloc[len(data_) - left - 1 if len(data_) - left - 1 >= 0 else 0: len(data_) - 1]["z"] * dataRatio)
        sendData["dt"].extend(data.iloc[len(data) - 1200 - 1 if len(data) - 1200 - 1 >= 0 else 0: len(data) - 1]["time"])
        sendData["x"].extend(data.iloc[len(data) - 1200 - 1 if len(data) - 1200 - 1 >= 0 else 0: len(data) - 1]["x"] * dataRatio)
        sendData["y"].extend(data.iloc[len(data) - 1200 - 1 if len(data) - 1200 - 1 >= 0 else 0: len(data) - 1]["y"] * dataRatio)
        sendData["z"].extend(data.iloc[len(data) - 1200 - 1 if len(data) - 1200 - 1 >= 0 else 0: len(data) - 1]["z"] * dataRatio)
        print(sendData)
        if left <= 0:
            sendData["dt"] = [f"{time.localtime(t).tm_hour}:{time.localtime(t).tm_min}:{time.localtime(t).tm_sec}" for t in sendData["dt"]]
            return sendData
    
    sendData["dt"] = [f"{time.localtime(t).tm_hour}:{time.localtime(t).tm_min}:{time.localtime(t).tm_sec}" for t in sendData["dt"]]
    return sendData