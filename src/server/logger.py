def info(msg):
    print(f"INFO:     {msg}")

def success(msg):
    print(f"\033[32mSUCCESS:  {msg}\033[0m")

def warning(msg):
    print(f"\033[33mWARNING:  {msg}\033[0m")

def error(msg):
    print(f"\033[31mERROR:    {msg}\033[0m")